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

#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

//#include <google/protobuf/unknown_field_set.h>

#include "log.h"
#include "fiber.h"


namespace sylar{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId(){
    return syscall(SYS_gettid);
}

uint32_t GetFiberId(){
    return sylar::Fiber::GetFiberId();
}

static std::string demangle(const char* str) {
    size_t size = 0;
    int status = 0;
    std::string rt;
    rt.resize(256);
    if(1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
        char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
        if(v) {
            std::string result(v);
            free(v);
            return result;
        }
    }
    if(1 == scanf(str, "%255s", &rt[0])) {
        return rt;
    }
    return str;
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if(strings == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i){
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::ostringstream ss;
    for(size_t i = 0; i < bt.size(); ++i){
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


uint64_t GetCurrentMS(){
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrenUS(){
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}


















}

