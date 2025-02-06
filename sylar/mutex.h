/**
  ******************************************************************************
  * @file           : mutex.h
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/5
  ******************************************************************************
  */


#ifndef SYLAR_MUTEX_H
#define SYLAR_MUTEX_H

#include <string>
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>

#include "noncopyable.h"
//#include "fiber.h"

namespace sylar{

/**
 * @brief 信号量
 */
class Semaphore : Noncopyable{
public:
    /**
     * @brief 构造函数
     * @param count 信号量值的大小
     */
    Semaphore(uint32_t count = 0);

    /**
     * @brief 析构函数
     */
    ~Semaphore();

    /**
     * @brief 获取信号量
     */
    void wait();

    /**
     * @brief 释放信号量
     */
    void notify();
private:
    sem_t m_semaphore;

};

/**
 * @brief 局部锁的模板实现
 * @tparam T
 */
template<class T>
struct ScopedLockImpl{
public:
    /**
     * @brief 构造函数
     * @param mutex
     */
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex){
        m_mutex.lock();
        m_locked = true;
    }

    /**
     * @brief 析构函数 自动释放锁
     */
    ~ScopedLockImpl(){
        unlock();
    }

    /**
     * @brief 加锁
     */
    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    ///mutex
    T& m_mutex;
    ///是否已上锁
    bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 */
template<class T>
struct ReadScopedLockImpl{
public:
    /**
     * @brief 构造函数
     * @param mutex  读写锁
     */
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex){
        m_mutex.rdlock();
        m_locked = true;
    }

    /**
     * @brief 析构函数 自动释放锁
     */
    ~ReadScopedLockImpl(){
        unlock();
    }

    /**
     * @brief 加读锁
     */
    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    ///mutex
    T& m_mutex;
    ///是否已上锁
    bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 */
template<class T>
struct WriteScopedLockImpl{
public:
    /**
     * @brief 构造函数
     * @param mutex  读写锁
     */
    WriteScopedLockImpl(T& mutex)
            :m_mutex(mutex){
        m_mutex.wrlock();
        m_locked = true;
    }

    /**
     * @brief 析构函数 自动释放锁
     */
    ~WriteScopedLockImpl(){
        unlock();
    }

    /**
     * @brief 加读锁
     */
    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    ///mutex
    T& m_mutex;
    ///是否已上锁
    bool m_locked;
};

/**
 * @brief 互斥量
 */
class Mutex : Noncopyable {
public:
    /// 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    /**
     * @brief 构造函数
     */
    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }

    /**
     * @brief 析构函数
     */
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    /**
     * @brief 加锁
     */
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }

    /**
     * @brief 解锁
     */
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    /// mutex
    pthread_mutex_t m_mutex;
};

/**
 * @brief 空锁（用于调试）
 */
class NullMutex : Noncopyable {
public:
    /// 局部锁
    typedef ScopedLockImpl<NullMutex> Lock;

    /**
     * @brief 构造函数
     */
    NullMutex() {}

    /**
     * @brief 析构函数
     */
    ~NullMutex() {}

    /**
     * @brief 加锁
     */
    void lock() {}

    /**
     * @brief 解锁
     */
    void unlock() {}
};

/**
 * @brief 读写互斥量
 */
class RWMutex : Noncopyable {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    ///读写锁
    pthread_rwlock_t m_lock;
};

}

#endif //SYLAR_MUTEX_H
