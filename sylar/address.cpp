/**
  ******************************************************************************
  * @file           : address.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/23
  ******************************************************************************
  */

#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "endian.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

/// *************************** 工具函数 ************************** ///

template<class T>
static T CreateMask(uint32_t bits) {
    // 生成的掩码是  000000...111
    /*
     * 假设当前地址为 192.168.1.1，prefix_len = 24。
     * 掩码为 0x000000FF（低 8 位为 1）。
     */
    return (1 << (sizeof (T) * 8 - bits)) - 1;
}


template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}


/// *************************** Address ************************** ///

/// 查找主机的地址   如果查找成功，返回查找到的第一个地址
Address::ptr Address::LookupAny(const std::string &host,
                                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

/// 查找主机的 IP 地址    如果找到的地址是 IPAddress 类型，返回第一个匹配的 IPAddress
IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host,
                                 int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto & i : result) {
            // 尝试将查找到的地址转换为 IP地址
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
                return v;
            }
        }
    }
    return nullptr;
}

/// 使用 getaddrinfo 系统调用查找与指定主机名（host）匹配的地址信息，并将结果保存到 result 中
bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family, int type, int protocol) {

    addrinfo hints, *results, *next;
    // 设置addrinfo结构体的默认值
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = nullptr;  // 规范化的主机名
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;       // 链表的下一个元素

    std::string node;
    const char* service = nullptr;

    // 处理ipv6情况   检查 IPv6Address service
    //（格式：[IPv6地址]），解析出主机地址和服务（端口）
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            // TODO check out of range
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2; // 提取端口号
            }
            node = host.substr(1, endipv6 - host.c_str() - 1); // 提取 IPv6 地址部分
        }
    }

    // 处理一般情况    检查 node service
    // 如果主机地址还为空，检查是否有端口号，并将其拆分为主机地址和服务部分
    if(node.empty()) {
        service = (const char*) memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());   // 提取主机名部分
                ++service;      // 服务部分是端口号，指向 ':' 后的位置
            }
        }
    }

    if(node.empty()) {
        node = host;
    }
    // 使用 getaddrinfo 函数查询地址信息
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << error << " errstr="
                << gai_strerror(error);
        return false;
    }

    next = results;   // 结果链表的第一个元素
    while(next) {
        // 将查询到的地址信息转换为 Address 类型的智能指针，并保存到结果中
        result.push_back(Create(next->ai_addr, (socklen_t )next->ai_addrlen));
        next = next->ai_next;  // 跳转到下一个地址信息节点
    }
    freeaddrinfo(results);  // 释放 addrinfo 结构体的内存
    return !result.empty();
}


/// 获取所有网络接口的地址信息，并将结果存储在 result 中
// 结果格式是：接口名称 -> 地址和子网掩码长度 (prefix_len)
bool Address::GetInterfaceAddresses(std::multimap<std::string,
                                       std::pair<Address::ptr, uint32_t>> &result,
                                       int family) {
    struct ifaddrs *next, *results;
    // 获取所有网络接口的地址信息
    if(getifaddrs(&results) != 0) {
        SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                << "err=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    try {
        // 遍历所有接口地址信息
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            // 如果指定了 family 并且当前接口的地址族与之不匹配，跳过该接口
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            // 根据地址族不同，处理 IPv4 和 IPv6 地址
            switch (next->ifa_addr->sa_family) {
                case AF_INET:
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));  // 创建 IPv4 地址
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;  // 获取 IPv4 子网掩码
                    prefix_len = CountBytes(netmask);  // 计算子网掩码的前缀长度
                }
                    break;
                case AF_INET6:
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    for(int i = 0; i < 16; ++i){
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                }
                    break;
                default:
                    break;
            }

            // 如果成功获取到地址，插入到结果中
            if(addr) {
                result.insert(std::make_pair(next->ifa_name,
                                             std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return !result.empty();
    }
    freeifaddrs(results);
    return !result.empty();
}


/// 获取指定接口的地址信息
// 如果接口为空或为 "*"，则返回所有地址
bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                                    const std::string &iface, int family) {
    // 如果接口为空或为 "*"，返回所有地址
    if(iface.empty() || iface == "*"){
        if(family == AF_INET || family == AF_UNSPEC){
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC){
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    // 存储所有接口的地址信息
    std::multimap<std::string,
            std::pair<Address::ptr, uint32_t>> results;

    // 获取接口地址信息
    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }
    // 查找指定接口的地址信息，并将其添加到结果中
    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}


int Address::getFamily() const {
    return getAddr()->sa_family;
}

std::string Address::toString() const {
    std::stringstream ss;  // 创建字符串流
    insert(ss);  // 将地址数据 Address对象 插入到字符串流中
    return ss.str();
}


/// 工厂函数，根据给定的 sockaddr 地址来创建适当类型的 Address 对象
Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
    if(addr == nullptr){
        return nullptr;
    }
    Address::ptr result;
    // 通过 sa_family 判断地址类型 创建对应对象
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

bool Address::operator<(const Address& rhs) const{
    // 取较小的地址长度作为要比较的字节数
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    /**
     * int memcmp(const void* ptr1, const void* ptr2, size_t num);
     *  比较 ptr1 和 ptr2 内存区域中字节的大小 num：要比较的字节数
     */
    // 比较 minlen长度内的字节
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);

    if(result < 0) return true;
    else if(result > 0) return false;
    // 如果minlen内的数据相等，比较地址长度
    else if(getAddrLen() < rhs.getAddrLen()) return true;

    return false;
}

bool Address::operator==(const Address& rhs) const{
    return getAddrLen() == rhs.getAddrLen()    // 长度相等
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;  // 字节数据相同
}

bool Address::operator!=(const Address& rhs) const{
    return !(*this == rhs);   // 使用重载的 == 号
}


/// ************************ IPAddress ************************ ///


IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;  // 只解析数字主机地址
    hints.ai_family = AF_UNSPEC;      // 不指定地址族（IPv4 或 IPv6 都可以）

    // getaddrinfo 解析主机地址
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address << ", "
                                  << port << ") error=" << error << " errno=" << errno
                                  << " errstr=" << strerror(errno);
        return nullptr;
    }

    try{
        // 解析成功 调用 Address::Create 创建对应地址
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);  // 释放地址信息
        return result;
    } catch(...) {
        freeaddrinfo(results);
        return nullptr;
    }
}


/// *************************** IPv4 ************************** ///

/// 专门用于创建 IPv4 地址的工厂函数
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port); //设置端口号
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr); // 解析IP地址
    if(result <= 0) {
        SYLAR_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                                  << port << ") rt=" << result << " errno=" << errno
                                  << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt; // 返回创建的 IPv4 地址对象
}


/// 通过 sockaddr_in 构造
IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr = address;
}

/// 通过 IP 地址 和 端口号构造
IPv4Address::IPv4Address(uint32_t address, uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    // 将地址从网络字节序（大端）转换为主机字节序（小端）
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const{
    return (sockaddr*) &m_addr;
}

sockaddr* IPv4Address::getAddr() {
    return (sockaddr*) &m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}

/// 将IPv4地址格式化为可读性字符串输出地址到 流
std::ostream & IPv4Address::insert(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    // IPv4 地址是 32 位（4 字节）
    os << ((addr >> 24) & 0xff) << "."    // 右移 24 位，获取地址的第一个字节
       << ((addr >> 16) & 0xff) << "."    // 右移 16 位，获取第二个字节
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);   // 掩码操作提取最低 8 位
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);  // 输出端口号 2字节
    return os;
}

/**
 * IPv4 地址：192.168.1.10   (11000000.10101000.00000001.00001010)
 * CreateMask               (00000000.00000000.00000000.11111111)
 *
 * 子网掩码：255.255.255.0    (11111111.11111111.11111111.00000000)
 *     -------->
 * 广播地址：192.168.1.255，表示网络内的所有主机。
 * 网络地址：192.168.1.0，表示该网络的起始地址。
 * 网段地址：0.0.0.10
 * 子网掩码：255.255.255.0，表示该网络的子网掩码，用于区分网络部分和主机部分。
 */

/// 获取该地址广播地址
// prefix_len 为子网掩码的位数
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr); // 创建一个新的 sockaddr_in 结构，复制当前地址
    // 按位或 (|=) 操作，将地址中的主机部分全部设置为 1，从而计算出广播地址
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
            CreateMask<uint32_t> (prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

/// 获取该地址的网段
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    // 按位与 (&=) 操作，保留 IP 地址的网络部分，将主机部分清零，从而计算出网络地址
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t> (prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

/// 获取子网掩码地址
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    // 取反 是因为子网掩码的网络部分全是 1，主机部分全是 0
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

/// 获取和设置端口号
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}


/// *************************** IPv6 ************************** ///

/// 专门用于创建 IPv6 地址的工厂函数
IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0) {
        SYLAR_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                                  << port << ") rt=" << result << " errno=" << errno
                                  << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address) {
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t *address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    // 将地址从网络字节序（大端）转换为主机字节序（小端）
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const{
    return (sockaddr*) &m_addr;
}

sockaddr* IPv6Address::getAddr() {
    return (sockaddr*) &m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

/// 将IPv6地址格式化为可读性字符串输出地址到 流
// IPv6   0x0:0x0:0x0:0x0:0x0:0x0:0x0:0x1 ---> [::1]:80
std::ostream & IPv6Address::insert(std::ostream& os) const {
    os << "[";
    // 每个 uint16_t 代表IPv6地址中的一个16位部分，IPv6地址总共有8个16位部分
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;

    /// 连续的 0使用一对冒号(::)表示 , 但是一个 IPv6 地址中只能使用一次
    // 是否使用压缩表示
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int) byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

/// 获取该地址广播地址
// 将给定的 IPv6 地址与子网掩码的反码进行按位或（OR）操作，得到广播地址
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
            CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

/// 获取该地址的网段
// 将给定的 IPv6 地址与子网掩码进行按位与（AND）操作，得到网络地址。网络地址的主机部分设置为全 0。
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
            CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

/// 获取子网掩码地址
// 子网掩码就是一个有前缀长度为 1 的连续 1，然后是剩下的为 0
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] =
            ~CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

/// 获取和设置端口号
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}


/// *************************** Unix ************************** ///

// Unix 域套接字路径的最大长度    sun_path 存储 Unix 域套接字的路径
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;    // 标识 Unix 域套接字
    // 为 Unix 地址分配足够的空间   offsetof 计算偏移量
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

/**
 * @brief 通过路径构造UnixAddress
 * @param path  UnixSocket路径
 */
UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr, 0 , sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;  // 加上一个 \0 终止符

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }
    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }

    // 将路径拷贝到 m_addr.sun_path 中
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);  // 更新 m_length
}

/// 返回sockaddr_un结构体指针
const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

sockaddr* UnixAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

void UnixAddress::setAddrLen(uint32_t v) {
    m_length = v;
}

/// 返回Unix域套接字的路径
std::string UnixAddress::getPath() const{
    std::stringstream  ss;
    // 超长 则输出 \0 和路径的其余部分
    if(m_length > offsetof(sockaddr_un, sun_path)
    && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                                   m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

/// 将 Unix 地址的路径输出到流中
std::ostream & UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
                && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                                          m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}


/// *************************** Unknow ************************** ///

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}

const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

sockaddr* UnknownAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream & UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Address& addr){
    return addr.insert(os);
}


}


