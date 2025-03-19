/**
  ******************************************************************************
  * @file           : scheduler.h
  * @author         : 18483
  * @brief          : 协程调度器封装
  * @attention      : None
  * @date           : 2025/2/10
  ******************************************************************************
  */


#ifndef SYLAR_SCHEDULER_H
#define SYLAR_SCHEDULER_H

#include <memory>
#include <vector>
#include <list>
#include <iostream>

#include "fiber.h"
#include "thread.h"

namespace sylar {

/**
 * @brief 协程调度器
 * @attention 封装 N-M 的协程调度器
 *            内部有一个线程池，支持协程在线程池里面切换
 */
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param threads 线程数量
     * @param use_caller 是否使用当前调用线程
     * @param name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /**
     * @brief 虚析构函数
     */
    virtual ~Scheduler();

    /**
     * @brief 返回协程调度器名称
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief 返回当前协程调度器
     */
    static Scheduler* GetThis();

    /**
     * @brief 返回当前协程调度器的调度协程
     */
    static Fiber* GetMainFiber();

    /**
     * @brief 启动协程调度器
     */
    void start();

    /**
     * @brief 停止协程调度器
     */
    void stop();


    /**
     * @brief 单个协程调度
     * @details 将一个协程或回调函数 调度到执行队列中，并通知调度器执行
     * @param fc 协程或者函数
     * @param thread 协程执行的线程id ，-1标识 任意线程
     */
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex); //线程安全
            //将协程添加到 m_fibers 容器中，并返回是否触发调度器
            need_tickle = scheduleNoLock(fc, thread);
        }
        //唤醒调度器
        if(need_tickle){
            tickle();
        }
    }

    /**
     * @brief 批量调度协程
     * @details 将一组协程调度到执行队列中，并通知调度器执行
     * @param begin 协程数组开始迭代器
     * @param end   协程数组结束迭代器
     */
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end){
                //更新 need_tickle 若为 true，说明有协程被调度
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle){
            tickle();
        }
    }

private:
    /**
     * @brief 协程调度启动(无锁)
     * @param fc
     * @param thread
     * @return
     */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        // 是否有协程待执行
        // 队列为空，唤醒调度器并传入协程
        // 队列不为空，已有协程任务，无需立即唤醒调度器
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);  // 保存协程对象和线程信息
        // 有效协程或回调函数 则加入队列
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

protected:
    /**
     * @brief 通知协程调度器有任务了
     */
    virtual void tickle();

    /**
     * @brief 协程调度函数
     */
    void run();
    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();

    /**
     * @brief 协程无任务可调度时执行 idle 协程
     */
    virtual void idle();

    /**
     * @brief 设置当前的协程调度器
     */
    void setThis();

    /**
     * @brief 是否有空闲线程
     */
    bool hasIdleThreads() {return m_idleThreadCount > 0; }

private:
    /**
     * @brief 协程 / 函数 / 线程组
     * @details 存储协程信息
     */
    struct FiberAndThread{
        /// 协程智能指针
        Fiber::ptr fiber;
        /// 协程执行函数
        std::function<void()> cb;
        /// 线程 id
        int thread;

        /**
         * @brief 构造函数
         * @param f 协程
         * @param thr 线程 id
         * @details 直接通过协程对象构造
         */
        FiberAndThread(Fiber::ptr f, int thr)
                :fiber(f), thread(thr){
        }

        /**
         * @brief 构造函数
         * @param f 协程指针
         * @param thr 线程 id
         * @post *f = nullptr
         * @details 从协程指针中获取协程对象 并 转交给 fiber 成员
         */
        FiberAndThread(Fiber::ptr* f, int thr)
                :thread(thr){
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * @param f 协程执行函数
         * @param thr 线程id
         * @details 通过回调函数构造
         */
        FiberAndThread(std::function<void()> f, int thr)
                :cb(f), thread(thr){
        }

        /**
         * @brief 构造函数
         * @param f 协程执行函数指针
         * @param thr 线程id
         * @post *f = nullptr
         * @details 从回调函数指针中获取执行逻辑，并把它转交给 cb 成员
         */
        FiberAndThread(std::function<void()>* f, int thr)
                :thread(thr){
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         * @details 创建空对象 -1 表示未指定线程
         */
        FiberAndThread()
                :thread(-1){
        }

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    ///Mutex
    MutexType m_mutex;
    ///线程池
    std::vector<Thread::ptr> m_threads;
    ///待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    /// use_caller 为 true 时有效，调度协程
    Fiber::ptr m_rootFiber;
    /// 协程调度器名称
    std::string m_name;

protected:
    /// 协程下的线程 id 数组
    std::vector<int> m_threadIds;
    /// 线程数量
    size_t m_threadCount = 0;
    /// 工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    /// 空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    /// 是否正在停止
    bool m_stopping = true;
    /// 是否自动停止
    bool m_autoStop = false;
    /// 主线程 id  (use_caller)
    int m_rootThread = 0;

};



}

#endif //SYLAR_SCHEDULER_H
