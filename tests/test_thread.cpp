/**
  ******************************************************************************
  * @file           : test_thread.cpp
  * @author         : 18483
  * @brief          : 测试 线程
  * @attention      : None
  * @date           : 2025/2/5
  ******************************************************************************
  */


/*
 *
 */
#include "../sylar/sylar.h"
#include <unistd.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void func1(){
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << "  this.name: " << sylar::Thread::GetThis()->getName()
                             << "  id: " << sylar::GetThreadId()
                             << "  this.id: " << sylar::Thread::GetThis()->getId();

}

void func2(){

}



int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i){
        sylar::Thread::ptr thr(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(size_t i = 0; i < thrs.size(); ++i){
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=";

    return 0;
}
