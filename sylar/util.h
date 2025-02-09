/**
  ******************************************************************************
  * @file           : util.h
  * @author         : 18483
  * @brief          : 常用的工具函数
  * @attention      : None
  * @date           : 2025/1/14
  ******************************************************************************
  */


#ifndef SYLAR_UTIL_H
#define SYLAR_UTIL_H

#include <cxxabi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iomanip>
//#include <json/json.h>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
//#include <google/protobuf/message.h>

namespace sylar{

/**
 * @brief 返回当前线程 ID
 */
pid_t GetThreadId();

/**
 * @brief 返回当前协程 ID
 */
uint32_t GetFiberId();

/**
 * @brief 获取当前的 调用栈
 * @param bt 保存调用栈
 * @param size 最多返回层数
 * @param skip 跳过栈顶的层数
 */
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param size 栈的最大层数
 * @param skip 跳过栈顶层数
 * @param prefix 栈信息前输出的内容
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");














template<class T>
const char* TypeToName(){
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}
















};

#endif //SYLAR_UTIL_H
