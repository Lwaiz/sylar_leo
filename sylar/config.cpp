/**
  ******************************************************************************
  * @file           : config.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/1/15
  ******************************************************************************
  */


#include <iostream>
#include <vector>

#include "config.h"

namespace sylar{

/// 全局配置变量 存储所有已注册的配置变量
//Config::ConfigVarMap Config::s_datas;

/// 根据名称查找配置变量
ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

/**
 * @brief 遍历 YAML 节点的键值对
 * @param prefix 当前键的前缀 用于处理嵌套结构 (system.port)
 * @param node 当前 YAML 节点
 * @param output 输出键值对列表
 */
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node>>& output){
    /// 命名合法性检查
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
            != std::string::npos){
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name" << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    /// 如果当前节点是一个 Map， 递归处理子节点
    if(node.IsMap()){
        for(auto it = node.begin();
                it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar()
                    : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

/**
 * @brief 加载 YAML 配置
 *        将 YAML 文件配置数据与程序内配置变量同步
 */
void Config::LoadFromYaml(const YAML::Node &root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    /// 遍历 root 节点，提取所有键值对到 all_nodes 列表
    ListAllMember("", root, all_nodes);
    /// 遍历 all_nodes
    for(auto& i : all_nodes){
        std::string key = i.first;
        /// 跳过空键
        if(key.empty()){
            continue;
        }
        /// 大小写不敏感 转换为小写
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        /// 查找是否已注册对应配置变量
        ConfigVarBase::ptr var = LookupBase(key);
        /// 存在已注册变量
        if(var){
            /// 标量 则直接更新 值
            if(i.second.IsScalar()){
                var->fromString(i.second.Scalar());
            } else{
                ///复杂类型，首先序列化为字符串，再更新值
                std::stringstream  ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

static std::map<std::string, uint64_t> s_file2modifytime;
static sylar::Mutex s_mutex;

//void Config::LoadFromConfDir(const std::string& path, bool force){
//    std::string absoulte_path = sylar::EnvMgr::GetInstance()
//}


void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap & m = GetDatas();
    for(auto it = m.begin(); it != m.end(); ++it){
        cb(it->second);
    }
}

}
