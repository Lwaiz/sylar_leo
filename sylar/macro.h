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


/**
 * @brief 优化条件编译的性能
 * @details  __builtin_expect 内建函数来告诉编译器，该条件大概率为真，从而优化代码生成
 */
#if defined __GNUC__ || defined __llvm__   // 针对GCC 或 LLVM 编译器
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
# define SYLAR_LIKELY(x)    __builtin_expect(!!(x), 1)
/// UNLIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
# define SYLAR_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x)   (x)
#define SYLAR_UNLIKELY(x)   (x)
#endif



///断言宏封装
#define SYLAR_ASSERT(x) \
    if(SYLAR_UNLIKELY(!(x))){             \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n"                           \
                << sylar::BacktraceToString(100, 2, "   ");   \
        assert(x);      \
    }

///断言宏封装
#define SYLAR_ASSERT2(x, w) \
    if(SYLAR_UNLIKELY(!(x))){             \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w           \
                << "\nbacktrace:\n"                           \
                << sylar::BacktraceToString(100, 2, "   ");   \
        assert(x);      \
    }

#endif //SYLAR_MACRO_H
