/**
  ******************************************************************************
  * @file           : iomanager.cpp
  * @author         : 18483
  * @brief          : I/O 协程调度模块实现
  * @attention      : 包含 triggerEvent,addEvent,delEvent,cancleEvent,cancleAll,
  *                 :     idle, stopping, tickle
  * @date           : 2025/2/14
  ******************************************************************************
  */

#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>


namespace sylar{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

enum EpollCtlOp{
};

/// 根据事件类型返回对应事件的上下文
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read; // 返回读事件上下文
        case IOManager::WRITE:
            return write; // 返回写事件上下文
        default: //未知事件触发断言
            SYLAR_ASSERT2(false, "getContext");
    }
}

/// 重置事件上下文信息 ，清空调度器，协程，回调函数
/// 为了确保没有残留的状态影响后续的事件处理
void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

/// 触发事件，执行该事件关联的回调函数或协程
void IOManager::FdContext::triggerEvent(sylar::IOManager::Event event) {
    // 确保当前事件已在 events 中注册
    SYLAR_ASSERT(events & event);  // 逻辑与
    // 从当前事件集合中移除该事件 使其不再被监听
    events = (Event)(events & ~event);
    // 获取当前事件上下文
    EventContext& ctx = getContext(event);

    // 执行事件回调函数 或 协程
    if(ctx.cb){
        // 将回调函数添加到调度器中，等待调度执行
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        // 将协程添加到调度器中，等待调度执行
        ctx.scheduler->schedule(&ctx.fiber);
    }
    // 事件触发后清空调度器字段，避免重复调度
    ctx.scheduler = nullptr;
    return;
}

/// 构造函数
IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
    // 创建epoll句柄, 参数为epoll监听的fd的数量
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0); // 检查epoll_creat是否成功

    // 创建管道 用于唤醒 epoll_wait
    /*
     * pipe的作用是用于唤醒epoll_wait，因为epoll_wait是阻塞的，
     * 如果没有事件发生，epoll_wait会一直阻塞，
     * 所以需要一个pipe来唤醒epoll_wait
     */
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    // 初始化 epoll 事件结构
    epoll_event event;
    // 初始化epoll事件，清空event结构内存
    memset(&event, 0, sizeof(epoll_event));
    // 设置事件类型为：读事件，边缘触发
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0]; //设置为管道pipe 的读端描述符

    // 设置管道读端为 非阻塞模式
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

    // 添加管道的读事件到epoll监听中
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    // 初始化 fd 上下文数组大小为32
    contextResize(32);

    //启动调度器，开始事件循环
    start();
}

/// 析构函数
IOManager::~IOManager(){
    // 停止调度器
    stop();
    close(m_epfd); // 关闭epoll句柄
    close(m_tickleFds[0]); // 关闭 pipe 读端
    close(m_tickleFds[1]); // 关闭 pipe 写端

    // 释放 fd 上下文数组中分配的内存
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

/// 向指定的文件描述符 (fd) 添加事件，并注册回调函数或协程
int IOManager::addEvent(int fd, sylar::IOManager::Event event, std::function<void()> cb) {
    /**
     * 1.根据文件描述符获取对应的文件描述符上下文对象
     * 2.使用epoll_ctl注册事件
     * 3.初始化文件描述符上下文对象
     */
    /// 1.获取文件描述符上下文
    // 初始化一个 FdContext (事件上下文)
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    // 从 m_fdContexts 中拿到对应的上下文
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {  // 如果 fd 对应的FdContext 不存在 则扩容
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    /// 2.防止重复添加事件，注册事件
    // 一个句柄一般不会重复添加同一个事件，可能是两个不同线程在操控同一个句柄
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event) {  // 检查文件描述符是否已注册了相同事件
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    // 若已经有注册的事件则为修改操作 若没有则添加操作
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    // 设置 epoll 事件，使用边缘触发 并保留保留原始事件
    epevent.events = EPOLLIN | fd_ctx->events | event;
    // 将fd_ctx 保存到data指针中
    epevent.data.ptr = fd_ctx;

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    /// 3.更新事件上下文
    // 增加待处理事件数量
    ++m_pendingEventCount;
    // 更新 fd_ctx 的事件状态
    fd_ctx->events = (Event)(fd_ctx->events | event);
    // 获取事件上下文
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    // 断言事件上下文的调度器 协程 回调函数都为空
    SYLAR_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    // 获取当前调度器
    event_ctx.scheduler = Scheduler::GetThis();
    // 若提供了回调函数，则存储到事件上下文中
    if(cb){
        event_ctx.cb.swap(cb); // 交换回调函数
    } else {
        // 获取当前协程作为回调
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                        , "state=" << event_ctx.fiber->getState());
    }
    return 0;
}


bool IOManager::delEvent(int fd, sylar::IOManager::Event event) {
    /**
     * 1.根据文件描述符获取对应的文件描述符上下文对象
	 * 2.使用epoll_ctl删除事件
	 * 3.重置文件描述符上下文对象
     */

    /// 1.获取文件描述符上下文

    RWMutexType::ReadLock lock(m_mutex);
    // 如果文件描述符无效 直接返回
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 如果该文件描述符没有注册该事件 直接返回
    if((!(fd_ctx->events & event))){
        return false;
    }

    /// 2.删除事件

    // 删除指定事件，创建新的不包含删除事件的事件集
    Event new_events = (Event)(fd_ctx->events & ~event);  // 逻辑与一个 非event
    // 如果还有其他事件，那么就是修改已注册事件，否则就是删除事件
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    // 创建 epoll_event 结构体
    epoll_event epevent;
    // 设置epoll事件，使用边缘触发模式 新的注册事件(只有在事件从无到有变化时，epoll返回该事件)
    epevent.events = EPOLLET | new_events;
    // 将 fd_ctx 保存到data指针中
    epevent.data.ptr = fd_ctx;

    // 注册事件
    // 调用epoll_ctl删除事件 将更新的事件集 epevent 注册到 m_epfd 中
    /**
     * @brief 从用户空间将epoll_event结构copy到内核空间
     * @parm m_epfd   epoll文件描述符
     * @parm op       决定是修改还是删除事件
     * @parm fd       要操作的文件描述符
     * @parm epevent  告诉内核需要监听的事件
     */
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    /// 3.重置事件上下文

    // 减少待处理事件数量
    --m_pendingEventCount;
    // 更新 fd_ctx 事件状态
    fd_ctx->events = new_events;
    // 获取该事件上下文
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    // 重置事件上下文
    fd_ctx->resetContext(event_ctx);
    return true;
}


/// 取消事件会触发事件   取消某个fd的事件 则会从调度器的 epoll_wait 中删除
bool IOManager::cancleEvent(int fd, sylar::IOManager::Event event) {
    /**
     * 1.根据文件描述符获取对应的文件描述符上下文对象
	 * 2.使用epoll_ctl删除对应事件
     * 3.直接触发对应事件
     */

    /// 1.获取 fd 对应的上下文

    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if((!(fd_ctx->events & event))){
        return false;
    }

    /// 2. 清除指定事件 表示不关心这个事件了
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    // 将fd_ctx 保存到data指针中
    epevent.data.ptr = fd_ctx;

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    /// 3.触发事件

    // 取消事件需要触发事件
    fd_ctx->triggerEvent(event);
    // 减少待处理事件数量
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancleAll(int fd) {
    /**
     * 1.根据文件描述符获取对应的文件描述符上下文对象
     * 2.使用epoll_ctl删除所有读写事件
     * 3.直接触发所有读写事件
     */

    /// 1.获取文件描述符上下文
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if((!(fd_ctx->events))){
        return false;
    }

    /// 2.删除所有事件

    // 删除操作
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    // 删除所有事件
    epevent.events = 0;
    // 将fd_ctx 保存到data指针中
    epevent.data.ptr = fd_ctx;

    // 注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                  << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    /// 3.触发所有事件
    // 触发所有读事件
    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    // 触发所有写事件
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    // 断言没有剩余事件
    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

/// 获取当前 IO调度器
IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

/// 通知调度协程从 idle 中退出
void IOManager::tickle(){
    /**
     * 1.判断是否有空闲线程 (如果没有则直接返回)
     * 2.向队列中的第一个文件描述符做一个写操作来唤醒它
     */
    if(!hasIdleThreads()) {
     return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping() {
    return Scheduler::stopping()      // 自动停止 / 正在停止 / 子协程为空 / 无活跃线程
        && m_pendingEventCount == 0;  // 当前等待执行的事件数量为0
}

/**
 * @brief 调度器无调度任务时会阻塞idle协程上
 * idle状态应该关注两件事：
 * 1.有没有新的调度任务，对应Schduler::schedule()，
 *     如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务
 * 2.当前注册的所有IO事件有没有触发，
 *     如果有触发，那么应该执行IO事件对应的回调函数
 */
void IOManager::idle(){
    /**
     * 1.调用 epoll_wait 等待事件。
     * 2.遍历每个事件并处理。
     * 3.根据事件类型更新 epoll 的事件注册，并触发协程执行。
     * 4.切换到待处理的协程。
     */

    SYLAR_LOG_DEBUG(g_logger) << "idle";
    // 一次epoll_wait最多检测256个就绪事件，超过则在下一轮检测
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    // 创建shared_ptr时，包括原始指针和自定义删除器，这样在shared_ptr析构时会调用自定义删除器，释放原始指针
    std::shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr)->void{
        delete[] ptr;
    });

    /// 1.循环等待事件
    while(true) {
        uint64_t next_timeout = 0;
        // 判断调度器是否停止
        if(stopping(next_timeout)){
            SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        // 阻塞在epoll_wait上，等待事件发生
        int rt = 0;
        do {
            // 默认超时时间为5秒
            static const int MAX_TIMEOUT = 3000;

            if(next_timeout != ~0ull) {
                next_timeout = (int) next_timeout > MAX_TIMEOUT
                        ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }

            // 等待事件发生，返回发生事件数量，-1 出错， 0 超时
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);

            // 如果是中断，继续等待
            if(rt < 0 && errno == EINTR){
            } else { // 否则退出循环
                break;
            }
        } while(true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        /// 2.处理所有发生的事件

        // 根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for(int i = 0; i < rt; ++i){
            epoll_event& event = events[i];
            // 如果是管道读端事件，则读取数据
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                // 读取管道中的数据，直到读完（read返回的字节数为0）
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            // 获取 fd 对应上下文
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            /**
             * EPOLLERR:出错，比如写读端已经关闭的pipe
             * EPOLLHUP:套接字对端关闭
             */
            // 出现这两种事件，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            // 实际发生的事件
            int real_events = NONE;
            // 读/写事件 则设置实际发生的事件为读/写事件
            if(event.events & EPOLLIN ) { real_events |= READ; }
            if(event.events & EPOLLOUT) { real_events |= WRITE; }
            //不是读写事件 则跳过
            if((fd_ctx->events & real_events) == NONE){ continue; }

            /// 3.更新epoll事件

            // 剔除已经发生的事件 将剩余事件重新加入 epoll_wait
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events; // 更新事件

            // // 对文件描述符 `fd_ctx -> fd` 执行操作 `op`，并将结果存储在 `rt2` 中
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                        << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            /// 4.触发事件和更新挂起事件计数

            // 触发 读/写 事件
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        /**
         * 5.切换协程
         * 一旦处理完所有事件， idle协程yield
         * 让调度协程(Scheduler::run)重新检查是否有新任务要调度
         */
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset(); // 重置当前协程
        // 让出执行权
        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}
