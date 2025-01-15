/**
  ******************************************************************************
  * @file           : config.h
  * @author         : 18483
  * @brief          : 配置模块
  * @attention      : None
  * @date           : 2025/1/15
  ******************************************************************************
  */


#ifndef SYLAR_CONFIG_H
#define SYLAR_CONFIG_H

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
//#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "log.h"


namespace sylar{

/**
 * @brief 配置变量的基类
 */
class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    /**
     * @brief 构造函数
     * @param name 配置参数名称 [0-9 a-z _ .]
     * @param description   配置参数描述
     */
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name),
        m_description(description){
    }
    /**
     * @brief 虚析构函数
     */
    virtual ~ConfigVarBase(){}

    /**
     * @brief 返回配置参数名称
     */
     const std::string& getName() const {return m_name;}

    /**
    * @brief 返回配置参数描述
    */
    const std::string& getDescription() const {return m_description;}

    /**
     * @brief 转成字符串
     */
    virtual std::string toString() = 0;

    /**
     * @brief 从字符串初始化值
     * @param val
     */
    virtual bool fromString(const std::string& val) = 0;

protected:
    std::string m_name;          ///配置参数名称
    std::string m_description;   ///配置参数描述

};

/**
 * @brief 配置参数模板子类，保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从 string 转换成 T类型的仿函数
 *          ToStr   从 T 转换成 string 的仿函数
 *          std::string  YAML 格式的字符串
 */
template <class T>
class ConfigVar : public ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    /**
     * @brief 通过 参数名，参数值，描述 构造 ConfigVar
     * @param name  参数名称有效字符为[0-9a-z_.]
     * @param default_value  参数的默认值
     * @param description    参数的描述
     */
    ConfigVar(const std::string& name,
              const T& default_value,
              const std::string& description = "")
          : ConfigVarBase(name, description),
          m_val(default_value){
    }

    /**
     * @brief 将参数值转成 YAML String
     */
    std::string toString() override{
        try{
            return boost::lexical_cast<std::string>(m_val);
        } catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    /**
     * @brief 从字符串初始化值
     * @param val
     */
    bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception"
                                              << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getValue() const {return m_val;}
    void setValue(const T& v) {m_val = v;}
private:
    T m_val;

};

/**
 * @brief ConfigVar 的管理类
 * @details 提供便捷的方法创建 / 访问 ConfigVar
 */
class Config{
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
             const T& default_value, const std::string& description = ""){
        auto tmp = Lookup<T>(name);
        if(tmp){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << "exists";
        } else {

        }
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                    != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        auto it = s_datas.find(name);
        if(it == s_datas.end()){
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }


private:
    static ConfigVarMap s_datas;
};



}

#endif //SYLAR_CONFIG_H
