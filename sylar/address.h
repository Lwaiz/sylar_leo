/**
  ******************************************************************************
  * @file           : address.h
  * @author         : 18483
  * @brief          : 网络地址相关的封装 (IPv4, IPv6, Unix)
  * @attention      : Address 是基类，IPAddress 是派生类，IPv4Address 和 IPv6Address 是具体实现
  * @date           : 2025/2/23
  ******************************************************************************
  */


#ifndef SYLAR_ADDRESS_H
#define SYLAR_ADDRESS_H

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>

namespace sylar {

class IPAddress;

/**
 * @brief 网络地址的基类   通用网络地址  抽象基类
 * @details 提供通用接口 ，获取地址族  sockaddr指针 地址长度 等
 */
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过 sockaddr 指针创建具体的 Address 地址对象
     * @param addr sockaddr指针
     * @param addrlen sockaddr的长度
     * @return 返回和 sockaddr 相匹配的 Address 失败返回 nullptr
     */
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);


    /**
     * @brief 通过 host 地址返回对应条件的所有 Address
     * @param result 保存满足条件的Address
     * @param host 域名 服务器名等          举例: www.sylar.top[:80] (方括号为可选内容)
     * @param family 协议族                (AF_INT, AF_INT6, AF_UNIX)
     * @param type socketl 类型            SOCK_STREAM、SOCK_DGRAM 等
     * @param protocol  协议               IPPROTO_TCP、IPPROTO_UDP 等
     * @return 返回转换是否成功
     */
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                       int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 地址返回对应条件的任意 Address
     * @return 返回满足条件的任意Address 失败返回nullptr
     */
    static Address::ptr LookupAny(const std::string& host,
                       int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 通过 host 地址返回对应条件的任意 IPAddress
     * @return 返回满足条件的任意IPAddress 失败返回nullptr
     */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                       int family = AF_INET, int type = 0, int protocol = 0);

    /**
     * @brief 返回本机所有网卡的 <网卡名，地址，子网掩码位数>
     * @param result 保存本机所有地址
     * @param family 协议族
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::multimap<std::string,
                                      std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

    /**
     * @brief 返回本机指定网卡的 地址，子网掩码位数
     * @param result 保存指定网卡的所有地址
     * @param iface 网卡名称
     * @param family 协议族
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result,
                                      const std::string& iface,int family = AF_INET);

    /**
     * @brief 虚析构函数
     */
    virtual ~Address(){}

    /**
     * @brief 返回协议族
     * @details AF_INET 或 AF_INET6
     */
    int getFamily() const;

    /**
     * @brief 返回 sockaddr 指针 (只读)
     */
    virtual const sockaddr* getAddr() const = 0;

    /**
     * @brief 返回 sockaddr 指针 (读写)
     */
    virtual sockaddr* getAddr() = 0;

    /**
     * @brief 返回 sockaddr 的长度
     */
    virtual socklen_t getAddrLen() const = 0;

    /**
     * @brief 将地址格式化为可读性字符串输出地址到 流
     * @param os 输出流
     */
    virtual std::ostream &insert(std::ostream& os) const = 0;

    /**
     * @brief 返回可读性字符串表示
     */
    std::string toString() const;

    /**
     * @brief 重载 小于 等于 不等于 地址比较函数
     * @param rhs
     */
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

};


/**
 * @brief IP地址基类 (是Address的派生类 专门表示IP地址)
 * @details 提供IP地址相关接口 如获取广播地址 网段地址 子网掩码 端口号等
 */
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief 获取该地址的广播地址
     * @param prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress 失败返回nullptr
     */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    /**
     * @brief 通过 域名 IP 服务器名 创建IPAddress
     * @param address 域名 IP 服务器名
     * @param port 端口号
     * @return 成功返回IPAddress 失败返回nullptr
     */
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    /**
     * @brief 获取该地址的网段地址
     * @param prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress 失败返回nullptr
     */
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

    /**
     * @brief 获取子网掩码地址
     * @param prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress 失败返回nullptr
     */
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    /**
     * @brief 返回端口号
     */
    virtual uint32_t getPort() const = 0;

    /**
     * @brief 设置端口号
     * @param v
     */
    virtual void setPort(uint16_t v) = 0;

};


/**
 * @brief IPv4地址类  (IPAddress的派生类 用于表示IPv4地址)
 * @details 封装 sockaddr_in结构体，提供IPv4地址相关操作
 */
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief 使用点分十进制地址创建 IPv4Address 对象
     * @param address 点分十进制地址， 如 192.168.1.1
     * @param port 端口号
     * @return 返回IPv4Address 失败返回 nullptr
     */
     static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    /**
     * @brief 通过 sockaddr_in 构造 IPv4Address
     * @param address sockaddr_in 结构体
     */
    IPv4Address(const sockaddr_in& address);

    /**
     * @brief 通过二进制地址构造 IPv4Address
     * @param address 二进制地址 address
     * @param port 端口号
     */
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    /// 将IPv4地址格式化为可读性字符串输出地址到 流
    std::ostream & insert(std::ostream& os) const override;

    /**
     * @brief 获取该地址广播地址
     */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    /**
     * @brief 获取该地址的网段
     */
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;

    /**
     * @brief 获取子网掩码地址
     */
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    /// 获取和设置端口号
    uint32_t getPort() const override;
    void setPort(uint16_t) override;

private:
    sockaddr_in m_addr;
};


/**
 * @brief IPv6地址类  (IPAddress的派生类 用于表示IPv6地址)
 * @details 封装 sockaddr_in6 结构体，提供IPv6地址相关操作
 */
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;


    /**
     * @brief 通过IPv6地址字符串创建 IPv6Address
     * @param address IPv6 地址字符串
     * @param port 端口号
     * @return 返回IPv6Address 失败返回 nullptr
     */
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    /**
     * @brief 无参构造函数
     */
    IPv6Address();
    /**
     * @brief 通过 sockaddr_in6 构造 IPv6Address
     * @param address sockaddr_in6 结构体
     */
    IPv6Address(const sockaddr_in6& address);

    /**
     * @brief 通过二进制地址构造 IPv6Address
     * @param address IPv6二进制地址 address
     * @param port 端口号
     */
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    /// 返回 sockaddr_in6 结构体指针
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    /// 将IPv6地址格式化为可读性字符串输出地址到 流
    std::ostream & insert(std::ostream& os) const override;

    /**
     * @brief 获取该地址广播地址
     */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    /**
     * @brief 获取该地址的网段
     */
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;

    /**
     * @brief 获取子网掩码地址
     */
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};


/**
 * @brief UnixSocket地址类 (是 Address 的派生类 用于表示 Unix 域套接字地址)
 * @details 封装了 sockaddr_un 结构体，提供了 Unix 域套接字地址的相关操作
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    /**
     * @brief 无参构造函数
     */
    UnixAddress();

    /**
     * @brief 通过路径构造UnixAddress
     * @param path  UnixSocket路径
     */
    UnixAddress(const std::string& path);

    /// 返回sockaddr_un结构体指针
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    /// 返回Unix域套接字的路径
    std::string getPath() const;
    std::ostream & insert(std::ostream& os) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};


/**
 * @brief 未知地址类
 */
class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;

    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream & insert(std::ostream& os) const override;

private:
    sockaddr m_addr;
};


/**
 * @brief 流式输出 Address
 */
std::ostream& operator<<(std::ostream& os, const Address& addr);

}



#endif //SYLAR_ADDRESS_H
