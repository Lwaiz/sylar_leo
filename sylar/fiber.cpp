/**
  ******************************************************************************
  * @file           : fiber.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/9
  ******************************************************************************
  */


#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar{

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 全局静态变量 生成协程 id
static std::atomic<uint64_t> s_fiber_id{0};
// 全局静态变量 统计当前协程数
static std::atomic<uint64_t> s_fiber_count{0};

///每个线程都有以下线程局部变量用于保存上下文信息
//线程局部变量 当前线程正在运行的协程
//程序通过 t_fiber 来追踪当前运行的是哪个协程
static thread_local Fiber* t_fiber = nullptr;
//线程局部变量 当前线程的主协程，相当于切换主线程运行 (智能指针)
static thread_local Fiber::ptr t_threadFiber = nullptr;

//静态配置变量 存储协程栈的大小 默认 128KB
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");


/**
 * @brief 协程 内存分配器
 */
class MallocStackAllocator{
public:
    /**
     * @brief 分配指定大小的内存
     * @return 返回指向分配内存的指针
     */
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    /**
     * @brief 释放内存
     * @param vp 指向需要释放的内存指针
     * @param size
     */
    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;


uint64_t Fiber::GetFiberId() {
    // 当前运行协程的局部指针
    if(t_fiber){
        //返回其协程 ID
        return t_fiber->getId();
    }
    //若当前没有运行中的协程，返回0
    return 0;
}

///无参构造 构造主协程对象
Fiber::Fiber() {
    /*
     * - 主协程没有独立栈，与线程栈共享
     * - 主协程上下文为线程上下文，协程切换时不会涉及上下文切换
     */
    m_state = EXEC;   //协程状态设置未执行中
    // 设置当前协程为当前线程的主协程
    SetThis(this);  // this指针指向当前协程对象

    //获取当前协程上下文信息保存到 m_ctx 中
    if(getcontext(&m_ctx)) {
        //成功返回0，失败返回-1 触发断言失败
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count; //增加协程计数

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

///有参构造 构造子协程对象
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb){

    ++s_fiber_count;
    //若给定初始化值用给定值，若没有用约定值 128KB
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    // 分配指定大小的协程栈空间
    m_stack = StackAllocator::Alloc(m_stacksize);
    //获取当前协程上下文信息保存到 m_ctx 中
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false, "getcontext");
    }

    //uc_link 置空，执行完当前context之后退出
    m_ctx.uc_link = nullptr;
    // 初始化栈指针
    m_ctx.uc_stack.ss_sp = m_stack;
    // 初始化栈大小
    m_ctx.uc_stack.ss_size = m_stacksize;

    // 指明 context 入口函数 创建上下文
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallMainFunc, 0);
    }
    // 输出调试信息，协程构造完成
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

///释放协程 栈空间
Fiber::~Fiber(){
    --s_fiber_count;
    // 根据内存是否为空，进行不同的释放操作
    if(m_stack){
        // 有栈，子协程，确保子协程未执行
        SYLAR_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT)
        // 释放运行栈
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // 没有栈 （主协程）
        SYLAR_ASSERT(!m_cb); //确保没有执行函数
        SYLAR_ASSERT(m_state == EXEC); //确保协程处于执行中状态

        Fiber* cur = t_fiber;
        // 若当前协程为主协程，将当前协程 置空
        if(cur == this){
            SetThis(nullptr);
        }
    }
    // 输出调试信息，协程析构完成
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
}

///重置协程函数， 并重置状态
void Fiber::reset(std::function<void()> cb) {
    /*
     * 协程处于 初始化INIT, 终止TERM, 异常EXCEPT 才允许重置
     * 重复利用已结束的协程，复用其栈空间，创建新协程
     */
    //确保栈存在
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM
                 || m_state == EXCEPT
                 || m_state == INIT)
    // 设置新的回调函数
    m_cb = cb;
    // 获取当前上下文
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    //初始化
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    // 通过makecontext 指定协程的入口函数
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    //设置为 INIT状态，表示协程已完成初始化，可以执行
    m_state = INIT;
}

void Fiber::call(){
    // 启动目标协程的执行
    SetThis(this);
    m_state = EXEC;
    //切换上下文到目标协程上下文，执行协程
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    // 从当前协程返回到主协程或者线程协程
    SetThis(t_threadFiber.get());
    // 切换回主协程的上下文
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

///切换到当前 子协程执行
void Fiber::swapIn() {
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    //从主协程切换到子协程执行
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

///切换到主协程 （子协程切换到后台）
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

/// 设置当前协程
void Fiber::SetThis(sylar::Fiber *f) {
    t_fiber = f;
}

/// 返回当前正在执行的协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    // 如果当前协程还未创建 则创建第一个协程 并作为主协程
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();

}

///协程切换到后台，并设置为 Ready 状态
void Fiber::YiledToReady() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state ==EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

///协程切换到后台，并设置为 Hold 状态
void Fiber::YiledToHold() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state ==EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

/// 总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

/// 协程的入口函数，用于执行协程的回调函数（对协程入口函数进行了封装）
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try{
        cur->m_cb();          // 执行回调函数
        cur->m_cb = nullptr;  // 执行完成，置空
        cur->m_state = TERM;  // 状态设置为终止
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << "fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
    } catch(...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                << "fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
    }

    auto raw_ptr= cur.get();
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach fiber id=" + std::to_string(raw_ptr->getId()));

}

void Fiber::CallMainFunc() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;  // 终止
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                      << "fiber_id=" << cur->getId()
                      << std::endl
                      << sylar::BacktraceToString();
    } catch(...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                      << "fiber_id=" << cur->getId()
                      << std::endl
                      << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

    SYLAR_ASSERT2(false, "never reach fiber id=" + std::to_string(raw_ptr->getId()));

}


}

