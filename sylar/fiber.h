/**
  ******************************************************************************
  * @file           : fiber.h
  * @author         : 18483
  * @brief          : 协程封装
  * @attention      : None
  * @date           : 2025/2/9
  ******************************************************************************
  */


#ifndef SYLAR_FIBER_H
#define SYLAR_FIBER_H

#include <memory>
#include <functional>
#include <ucontext.h>

namespace sylar{

class Scheduler;

/**
 * @brief 协程类
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     */
    enum State{
        ///初始化状态
        INIT,
        ///暂停状态
        HOLD,
        ///执行中状态
        EXEC,
        ///结束状态
        TERM,
        ///可执行状态
        READY,
        ///异常状态
        EXCEPT
    };

private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个主协程的构造
     *            私有方法，只能通过 GetThis()返回
     */
    Fiber();

public:
    /**
     * @brief 有参构造函数
     * @attention 用于子协程的构造
     * @param cb 协程执行的函数
     * @param stacksize 协程栈的大小
     * @param use_caller 是否在 MainFiber 上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

    /**
     * @brief 析构函数
     */
    ~Fiber();

    /**
     * @brief 重置协程执行函数，并设置状态
     * @param cb
     * @details 重复利用已结束的协程，复用其栈空间，创建新协程
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前协程切换到执行状态
     * @pre   执行 当前线程的主协程
     */
    void call();

    /**
     * @brief 将当前协程切换到后台
     * @pre   执行的为当前子协程
     * @post  返回到线程的主协程
     */
    void back();

    /**
     * @brief 返回协程 id
     */
    uint64_t getId() const {return m_id;}

    /**
     * @brief 返回协程状态
     * @return
     */
    State getState() const {return m_state;}

public:
    /**
     * @brief 设置当前线程的运行协程
     * @param f 运行协程
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在协程
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台 并设置为 READY 状态
     */
    static void YiledToReady();

    /**
     * @brief 当前协程切换到后台 并设置为 HOLD 状态
     */
    static void YiledToHold();

    /**
     * @brief 返回当前协程总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
    static void MainFunc();

    /**
     * @brief 协程执行函数
     * @post  执行完成返回到线程调度协程
     */
    static void CallMainFunc();

    /**
     * @brief 获取当前协程 ID
     */
    static uint64_t GetFiberId();

private:
    ///协程 id
    uint64_t m_id = 0;
    ///协程运行栈大小
    uint32_t m_stacksize = 0;
    ///协程状态
    State m_state = INIT;
    ///协程上下文
    ucontext_t m_ctx;
    ///协程运行栈指针
    void* m_stack = nullptr;
    ///协程运行函数
    std::function<void()> m_cb;

};


}

#endif //SYLAR_FIBER_H
