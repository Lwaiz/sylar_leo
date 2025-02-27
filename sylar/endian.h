/**
  ******************************************************************************
  * @file           : endian.h
  * @author         : 18483
  * @brief          : 字节序转换 操作函数 (大端 | 小端)
  * @attention      : None
  * @date           : 2025/2/23
  ******************************************************************************
  */


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

#if BYTE_ORDER == BIG_ENDIAN    // 检测当前机器的字节序如果是大端序
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN  // 定义项目字节序为大端字节序
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

/// 如果是当前机器是大端字节序
#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN

/**
 * @brief 大端数据直接返回
 */
    template<class T>
    T byteswapOnLittleEndian(T t){
        return byteswap(t);
    }

/**
 * @brief 小端数据转换为大端
 */
    template<class T>
    T byteswapOnBigEndian(T t){
        return t;
    }


/// 如果是小端字节序
#else

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


#endif

}

#endif //SYLAR_ENDIAN_H
