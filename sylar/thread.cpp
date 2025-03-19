/**
  ******************************************************************************
  * @file           : thread.cpp
  * @author         : 18483
  * @brief          : 线程模块
  * @attention      : None
  * @date           : 2025/2/5
  ******************************************************************************
  */


#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

// 线程相关的线程局部存储（thread_local）变量
// thread_local 确保 每个线程都有自己独立的副本，互不干扰
static thread_local Thread* t_thread = nullptr;      // 当前线程的 Thread 实例指针
static thread_local std::string t_thread_name = "UNKNOW"; // 当前线程的名称，默认为"UNKNOW"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/// 获取当前线程对应的 Thread 实例
Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName(){
    return t_thread_name;
}

void Thread::SetName(const std::string& name){
    if(name.empty()){
        return;
    }
    // 如果线程实例存在，设置线程的名称
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

/// 构造函数，初始化线程并启动
Thread::Thread(std::function<void()> cb, const std::string &name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    // 创建线程并执行 run 函数
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_creat thread fail, rt=" << rt
                << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}

Thread::~Thread(){
    //确保线程正确分离，避免资源泄漏
    if(m_thread){
        pthread_detach(m_thread);
    }
}

/// 等待线程执行完成
void Thread::join(){
    if(m_thread){
        // 等待线程结束
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                                      << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

/// 线程执行的静态函数，负责启动线程回调函数
void* Thread::run(void* arg){
    Thread* thread = (Thread*)arg;   // 获取传入的 Thread 实例指针
    t_thread = thread;          // 设置当前线程的 Thread 实例
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    // 将回调函数从成员变量 m_cb 交换到 cb 变量中
    cb.swap(thread->m_cb);
    // 在出构造函数之前，确保线程先跑起来，保证能够初始化id
    thread->m_semaphore.notify();

    cb();  // 执行回调函数
    return 0;  // 返回
}


}

