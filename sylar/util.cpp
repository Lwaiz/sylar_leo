/**
  ******************************************************************************
  * @file           : util.cpp
  * @author         : 18483
  * @brief          : 常用的工具函数实现
  * @attention      : None
  * @date           : 2025/1/14
  ******************************************************************************
  */



#include "util.h"

namespace sylar{

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

uint32_t GetFiberId(){
    return 0;
    //return sylar::Fiber::Get
}












}

