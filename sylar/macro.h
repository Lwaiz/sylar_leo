/**
  ******************************************************************************
  * @file           : macro.h
  * @author         : 18483
  * @brief          : 常用的宏 封装
  * @attention      : None
  * @date           : 2025/2/9
  ******************************************************************************
  */


#ifndef SYLAR_MACRO_H
#define SYLAR_MACRO_H

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

///断言宏封装
#define SYLAR_ASSERT(x) \
    if(!(x)){             \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n"                           \
                << sylar::BacktraceToString(100, 2, "   ");   \
        assert(x);      \
    }

///断言宏封装
#define SYLAR_ASSERT2(x, w) \
    if(!(x)){             \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w           \
                << "\nbacktrace:\n"                           \
                << sylar::BacktraceToString(100, 2, "   ");   \
        assert(x);      \
    }

#endif //SYLAR_MACRO_H
