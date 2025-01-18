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
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "log.h"
#include "util.h"

namespace sylar {

/**
 * @brief 配置变量 基类
 */
class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    /**
     * @brief 构造函数  初始化配置参数的名称和描述
     * @param name 配置参数名称 [0-9 a-z _ .]
     * @param description   配置参数描述
     */
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name),
        m_description(description){
        ///小写化
        std::transform(m_name.begin(), m_name.end(),m_name.begin(), ::tolower);
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
     * @brief 将配置参数的值转为字符串，用于序列化
     */
    virtual std::string toString() = 0;

    /**
     * @brief 从字符串解析配置参数的值，用于反序列化
     * @param val
     */
    virtual bool fromString(const std::string& val) = 0;

    virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;          ///配置参数名称
    std::string m_description;   ///配置参数描述
};


/**
 * @brief 类型转换模板类 (F 源类型, T 目标类型)
 */
template<class F, class T>
class LexicalCast{
public:
    /**
     * @brief 类型转换
     * @param v 源类型值
     * @return 返回 v 转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F& v){
        return boost::lexical_cast<T>(v);
    }
};

/// 1.vector<T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::vector<T> )
 */
template<class T>
class LexicalCast<std::string, std::vector<T>>{
public:
    /// 重载 () 运算符 仿函数
    std::vector<T> operator()(const std::string& v){
        /// 利用 YAML::Load 将字符串解析为 YAML::Node
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        /// 遍历 YAML::Node 的每一个元素，递归调用转换为目标类型 T 并存入 vec
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类偏特化( std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::vector<T>, std::string>{
public:
    std::string operator()(const std::vector<T>& v){
        /// 创建一个 YAML::Node 类型为 Sequence
        YAML::Node node(YAML::NodeType::Sequence);
        /// 遍历 std::vector<T> 的每一个元素，
        /// 递归调用转换为字符串，并加入到 YAML::Node 中
        for(auto & i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        /// 将 YAML::Node 序列化为字符串返回
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/// 2.list<T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::list<T> )
 */
template<class T>
class LexicalCast<std::string, std::list<T>>{
public:
    std::list<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类偏特化( std::list<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::list<T>, std::string>{
public:
    std::string operator()(const std::list<T>& v){
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto & i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/// 3.set<T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::set<T> )
 */
template<class T>
class LexicalCast<std::string, std::set<T>>{
public:
    std::set<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
* @brief 类型转换模板类偏特化( std::set<T> 转换成 YAML String)
*/
template<class T>
class LexicalCast<std::set<T>, std::string>{
public:
    std::string operator()(const std::set<T>& v){
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto & i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/// 4.unordered_set<T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::unordered_set<T> )
 */
template<class T>
class LexicalCast<std::string, std::unordered_set<T>>{
public:
    std::unordered_set<T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
* @brief 类型转换模板类偏特化( std::unordered_set<T> 转换成 YAML String)
*/
template<class T>
class LexicalCast<std::unordered_set<T>, std::string>{
public:
    std::string operator()(const std::unordered_set<T>& v){
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto & i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/// 5.map<std::string, T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::map<std::string, T> )
 */
template<class T>
class LexicalCast<std::string, std::map<std::string, T>>{
public:
    std::map<std::string, T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
                it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                         LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
* @brief 类型转换模板类偏特化( std::map<std::string, T> 转换成 YAML String)
*/
template<class T>
class LexicalCast<std::map<std::string, T>, std::string>{
public:
    std::string operator()(const std::map<std::string, T>& v){
        YAML::Node node(YAML::NodeType::Map);
        for(auto & i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/// 6.unordered_map<std::string, T>
/**
 * @brief 类型转换模板类偏特化(YAML String 转换成 std::unordered_map<std::string, T> )
 */
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>{
public:
    std::unordered_map<std::string, T> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin();
            it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),
                                         LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

/**
* @brief 类型转换模板类偏特化( std::unordered_map<std::string, T> 转换成 YAML String)
*/
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>{
public:
    std::string operator()(const std::unordered_map<std::string, T>& v){
        YAML::Node node(YAML::NodeType::Map);
        for(auto & i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/**
 * @brief 配置参数模板子类，保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从 string 转换成 T类型的仿函数
 *          ToStr   从 T 转换成 string 的仿函数
 *          std::string  YAML 格式的字符串
 *       使用模板偏特化
 */
template <class T, class FromStr = LexicalCast<std::string, T>
                 , class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    /// 配置事件回调函数（callback -- cb）
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;

    /**
     * @brief 通过 参数名，参数值，描述 构造 ConfigVar
     * @param name  参数名称有效字符为[0-9a-z_.]
     * @param default_value  参数的默认值
     * @param description    参数的描述
     */
    ConfigVar(const std::string& name,
              const T& default_value,
              const std::string& description = "")
          : ConfigVarBase(name, description),   ///调用基类构造函数
          m_val(default_value){
    }

    /**
     * @brief 将参数值转成 YAML String
     * @details 使用 boost::lexical_cast 将值 m_val 转换为字符串。
     *          如果转换失败，捕获异常并记录日志
     */
    std::string toString() override{
        try{
            //return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        } catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    /**
     * @brief 从字符串初始化值
     * @param val
     * @details 使用 boost::lexical_cast 将字符串 val 转换为类型 T。
     *          如果转换失败，捕获异常并记录日志
     */
    bool fromString(const std::string& val) override {
        try {
             // m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception"
                                              << e.what() << " convert: string to " << typeid(m_val).name();
        }
        return false;
    }
    /**
     * @brief 返回配置变量的值
     */
    const T getValue() const {return m_val;}

    /**
     * @brief 设置配置变量的新值
     * @param v
     */
    void setValue(const T& v) {
        if(v == m_val){
            return;
        }
        ///当setValue时 值发生变化，执行回调函数，类似于观察者模式
        for(auto& i : m_cbs) {
            i.second(m_val, v);
        }
        m_val = v;
    }

    /**
     * @brief 返回参数值的类型名称
     */
    std::string getTypeName() const override {return TypeToName<T>();}

    /**
     * @brief 添加变化回调函数
     * @param key 回调函数 唯一 ID
     * @param cb  回调函数
     */
    uint64_t addListener(on_change_cb  cb){
        static uint64_t s_fun_id = 0;
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除回调函数
     * @param key 回调函数 唯一 id
     */
    void delListener(uint64_t key){
        m_cbs.erase(key);
    }

    /**
     * @brief 获取回调函数
     * @return 如果存在返回对应的回调函数,否则返回nullptr
     */
    on_change_cb getListener(uint64_t key){
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 清理所有回调函数
     */
    void clearListener(){
        m_cbs.clear();
    }

private:
    /// 存储配置变量的值，类型由模板参数 T 确定
    T m_val;
    ///变更回调函数组
    /// uint64_t: key 要求唯一；一般可以用hash
    std::map<uint64_t , on_change_cb > m_cbs;
};

/**
 * @brief ConfigVar 的管理类
 * @details 提供便捷的方法创建 / 访问 ConfigVar
 */
class Config{
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    /**
     * @brief 获取/ 创建 对应参数名的配置参数
     * @tparam T 模板类型
     * @param name 配置参数名称
     * @param default_value 参数默认值
     * @param description   参数描述
     * @details 获取参数名为 name 的配置参数 如果存在直接返回
     *          如果不存在，创建参数配置并用 defaul_value赋值
     * @return 返回对应参数配置， 如果参数存在但类型不匹配 返回 nullptr
     * @exception 如果参数包含非法字符 抛出异常
     */
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name,
             const T& default_value, const std::string& description = ""){
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(tmp){
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name= " << name << " exists";
                return tmp;
            } else {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                        << TypeToName<T>() << " real_type=" << it->second->getTypeName()
                        << " " << it->second->toString();
                return nullptr;
            }
        }

        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                    != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    /**
     * @brief 查找配置参数
     * @tparam T
     * @param name
     * @return 返回配置参数名为 name 的配置参数
     */
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()){
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 使用 YAM::Node 初始化配置模块
     * @param root
     */
    static void LoadFromYaml(const YAML::Node& root);

    static ConfigVarBase::ptr LookupBase(const std::string& name);



private:
    //static ConfigVarMap s_datas;
    /**
     * @return 返回所有配置项
     */
    static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
        return s_datas;
    }
};





}

#endif //SYLAR_CONFIG_H
