/**
  ******************************************************************************
  * @file           : iomanager.h
  * @author         : 18483
  * @brief          : 基于 Epoll 的 IO协程调度器
  * @attention      : fd->(File Description)  文件描述符
  * @date           : 2025/2/14
  ******************************************************************************
  */


#ifndef SYLAR_IOMANAGER_H
#define SYLAR_IOMANAGER_H

#include "scheduler.h"
#include "timer.h"

namespace sylar {

class IOManager : public Scheduler , public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief IO 事件
     */
    enum Event {
        NONE  = 0x0,  /// 无事件
        READ  = 0x1,  /// 读事件 (EPOLLIN)
        WRITE = 0x4   /// 写事件 (EPOLLOUT)
    };

private:
    /**
     * @brief Socket事件上下文类
     */
    struct FdContext{
        typedef Mutex MutexType;
        /**
         * @brief 事件上下文类
         */
        struct EventContext {
            /// 事件执行的调度器
            Scheduler* scheduler = nullptr;
            /// 事件协程
            Fiber::ptr fiber;
            /// 事件的回调函数
            std::function<void()> cb;
        };

        /**
         * @brief 获取事件上下文类
         * @param event 事件类型
         * @return 返回对应事件的上下文
         */
        EventContext& getContext(Event event);

        /**
         * @brief 重置事件的上下文
         * @param ctx 待重置的上下文类
         */
        void resetContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * @param event 事件类型
         * @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
         */
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的句柄
        /// fd : File Description
        int fd = 0;
        /// 当前的事件
        /// 该fd添加了哪些事件的回调函数，或者说该fd关心哪些事件
        Event events = NONE;
        MutexType mutex; /// 事件上下文的锁
    };

public:
    /**
     * @brief 构造函数
     * @param threads  线程数量
     * @param use_caller 是否将调用线程包含进去
     * @param name 调度器名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /**
     * @brief 析构函数
     */
    ~IOManager();

    /**
     * @brief 添加事件
     * @param fd socket 句柄
     * @param event 事件类型
     * @param cb 事件回调函数
     * @return 添加成功返回 0， 失败返回 -1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件
     * @param fd socket句柄
     * @param event 事件类型
     * @attention 不会触发事件
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件
     * @param fd
     * @param event
     * @attention 如果事件存在则触发事件
     */
    bool cancleEvent(int fd, Event event);

    /**
     * @brief 取消所有事件
     * @param fd socket 句柄
     */
    bool cancleAll(int fd);

    /**
     * @brief 返回当前的 IOManager
     */
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    /**
     * @brief 重置 socket 句柄上下文的容器大小
     * @param size 容量大小
     */
    void contextResize(size_t size);

    /**
     * @brief 判断是否可以停止
     * @param timeout 最近要触发的定时器事件间隔
     */
    bool stopping(uint64_t& timeout);
private:
    /// epoll 文件句柄
    int m_epfd = 0;
    /// pipe 文件句柄  fd[0]读端，fd[1]写端
    int m_tickleFds[2];
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    /// IOManager 的 Mutex
    RWMutexType m_mutex;
    /// socket事件上下文数组
    std::vector<FdContext*> m_fdContexts;
};


}
#endif //SYLAR_IOMANAGER_H
