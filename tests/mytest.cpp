/**
  ******************************************************************************
  * @file           : mytest.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/23
  ******************************************************************************
  */


/*
 *
 */
#include <iostream>
#include <vector>

using namespace std;

#ifndef SYLAR_ENDIAN_H
#define SYLAR_ENDIAN_H

// 小端字节序
#define SYLAR_LITTLE_ENDIAN 1
// 大端字节序
#define SYLAR_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

namespace sylar {

/**
 * @brief 8字节类型的字节序转化
 * @tparam T
 */
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value){
        return (T) bswap_64((uint64_t) value);
    }

/**
* @brief 4字节类型的字节序转化
* @tparam T
*/
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value){
        return (T) bswap_32((uint32_t) value);
    }

/**
* @brief 2字节类型的字节序转化
* @tparam T
*/
    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value){
        return (T) bswap_16((uint16_t) value);
    }


/// 条件编译 确定系统字节序

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

/// 如果是大端字节序
#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN

    /**
 * @brief 只在小端机器上执行 byteswap 在大端机器上什么也不做
 */
    template<class T>
    T byteswapOnLittleEndian(T t){
        return t;
    }

/**
 * @brief 只在大端机器上执行 byteswap 在小端机器上什么也不做
 */
    template<class T>
    T byteswapOnBigEndian(T t){
        return byteswap(t);
    }


/// 如果是小端字节序
#else

/**
 * @brief 只在小端机器上执行 byteswap 在大端机器上什么也不做
 */
    template<class T>
    T byteswapOnLittleEndian(T t){
        return byteswap(t);
    }

/**
 * @brief 只在大端机器上执行 byteswap 在小端机器上什么也不做
 */
    template<class T>
    T byteswapOnBigEndian(T t){
        return t;
    }


#endif

}

#endif //SYLAR_ENDIAN_H


template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;  // 高位掩码
}

void mytest(){
    uint32_t mask = CreateMask<uint32_t> (24);
    cout << hex << mask << endl;

    cout << hex << sylar::byteswapOnLittleEndian(CreateMask<uint32_t> (24));
}

int main() {
    mytest();

    return 0;
}
