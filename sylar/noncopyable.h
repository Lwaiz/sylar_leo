/**
  ******************************************************************************
  * @file           : noncopyable.h
  * @author         : 18483
  * @brief          : 不可拷贝对象封装
  * @attention      : None
  * @date           : 2025/2/5
  ******************************************************************************
  */


#ifndef SYLAR_NONCOPYABLE_H
#define SYLAR_NONCOPYABLE_H

namespace sylar {

/**
 * @brief 不可拷贝对象 不能拷贝 赋值
 */
class Noncopyable{
public:
    /**
     * @brief 默认构造函数
     */
    Noncopyable() = default;
    /**
     * @brief 默认析构函数
     */
    ~Noncopyable() = default;
    /**
     * @brief 禁用 拷贝构造函数
     */
    Noncopyable(const Noncopyable&) = delete;
    /**
     * @brief 禁用 赋值函数
     * @return
     */
    Noncopyable& operator=(const Noncopyable&) = delete;

};
}

#endif //SYLAR_NONCOPYABLE_H
