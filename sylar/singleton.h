/**
  ******************************************************************************
  * @file           : singleton.h
  * @author         : 18483
  * @brief          : 单例模式封装
  * @attention      : None
  * @date           : 2025/1/14
  ******************************************************************************
  */


#ifndef SYLAR_SINGLETON_H
#define SYLAR_SINGLETON_H

#include <memory>

namespace sylar{

/**
 * @brief    单例模式封装类
 * @tparam T    类型
 * @tparam X    为了创造多个实例对应的 Tag
 * @tparam N    同一个 Tag 创造了多个实例索引
 */
template <class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     *      延迟初始化 只有在第一次调用 GetInstance 时创建实例
     */
    static T* GetInstance(){
        static T v;    ///static 确保全局只有一个实例
        return &v;
    }
};
/**
 * @brief    单例模式 智能指针封装类
 * @tparam T    类型
 * @tparam X    为了创造多个实例对应的 Tag
 * @tparam N    同一个 Tag 创造了多个实例索引
 */
    template <class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        /**
         * @brief 返回单例智能指针
         * @return
         */
        static std::shared_ptr<T> GetInstance(){
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };

}

#endif //SYLAR_SINGLETON_H
