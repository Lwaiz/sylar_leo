/**
  ******************************************************************************
  * @file           : timer.cpp
  * @author         : 18483
  * @brief          : 定时器模块实现   包含 Timer类  和 TimerManager类
  * @attention      : None
  * @date           : 2025/2/16
  ******************************************************************************
  */

#include "timer.h"
#include "util.h"

namespace sylar {

/// Timer类的比较器 比较两个定时器对象的优先级
bool Timer::Comparator::operator()(const Timer::ptr &lhs,
            const Timer::ptr &rhs) const {
    if(!lhs && !rhs){ return false;}
    if(!lhs){ return true;}
    if(!rhs){ return false;}
    if(lhs->m_next < rhs->m_next){ return true;}
    if(lhs->m_next > rhs->m_next){ return false;}

    return lhs.get() < rhs.get();   // 如果两个定时器时间戳相同，根据内存地址决定顺序
}

/// 定时器构造函数 初始化循环定时器
Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager *manager)
     :m_recurring(recurring)
     ,m_ms(ms)   // m_ms ：定时器周期
     ,m_cb(cb)
     ,m_manager(manager) {
    m_next = sylar::GetCurrentMS() + m_ms;  // 定时器下次触发时间
}

/// 初始化非循环定时器
Timer::Timer(uint64_t next)
    :m_next(next){
}

bool Timer::cancle() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    /// 将回调函数置空 从定时器集合中删除该定时器
    if(m_cb){
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

/// 刷新定时器 触发时间
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    // 没有回调函数 不能刷新
    if(!m_cb) {
        return false;
    }
    // 删除原始的定时器
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    // 更新定时器时间，重新插入
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

/// 重置定时器的间隔时间
bool Timer::reset(uint64_t ms, bool from_now) {
    // 间隔时间没变 且不用从当前开始，无需重置
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    // 删除原始定时器
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    // 初始化定时器开始时间
    uint64_t start = 0;
    if(from_now) {
        start = sylar::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    // 重置定时器
    m_ms = ms;
    m_next = start +m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}


// ----------------------- TimerManager -------------------------- //


TimerManager::TimerManager() {
    m_previouseTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {
}

/// 创建定时器并 添加到管理器中
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock) {
    // 插入定时器并返回位置
    auto it = m_timers.insert(val).first;
    // 判断是否插入到集合的最前面
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;  // 标记已被触发
    }
    lock.unlock();
    // 插入到最前面 触发相关操作
    if(at_front) {
        onTimerInsertedAtFront();
    }
}

/// 通过 std::weak_ptr 来检查条件是否依然有效，如果有效则执行回调函数
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

/// 创建有条件 的 定时器
Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

/// 返回下一个定时器的时间间隔
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    // 如果没有定时器 返回一个特殊值
    if(m_timers.empty()) {
        return ~0ull;
    }

    // 获取下一个定时器
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();
    // 如果当前时间已经超过或等于下一个定时器的时间 则已经到期(立即执行) 返回0
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        // 否则返回下一个定时器到期的时间差
        return next->m_next - now_ms;
    }
}

/// 检查定时器是否到期 并执行到期的定时器回调函数
void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
    uint64_t now_ms = sylar::GetCurrentMS();  // 获取当前时间戳
    std::vector<Timer::ptr> expired;  // 存储已过期的定时器
    /// 提高并发性能，先使用读锁检查 在使用写锁检查
    {
        RWMutexType::ReadLock lock(m_mutex); // 读锁锁定
        if(m_timers.empty()){
            return;  // 没有定时器
        }
    }
    RWMutexType::WriteLock lock(m_mutex);    // 写锁锁定
    if(m_timers.empty()) {
        return;
    }

    // 创建一个当前时间戳的定时器 用来检查已过期的定时器
    Timer::ptr now_timer(new Timer(now_ms));
    // 查找时间 >= 当前时间的第一个定时器
    auto it = m_timers.lower_bound(now_timer);
    // 遍历定时器集合 直到找到第一个不再过期的定时器
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    // 将已过期的定时器插入到 expired 列表
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it); // 移除过期定时器
    // 为回调函数分配足够空间
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        // 将其回调函数加入到 cbs
        cbs.push_back(timer->m_cb);
        // 循环定时器，重新加入到 m_timers
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr; // 非循环定时器 清空
        }
    }

}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}
