/**
  ******************************************************************************
  * @file           : mutex.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/5
  ******************************************************************************
  */


#include <stdexcept>
#include "mutex.h"

namespace sylar {

Semaphore::Semaphore(uint32_t count) {
    // 初始化信号量
    if(sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    // 销毁信号量
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

/// 释放信号量
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}


}

