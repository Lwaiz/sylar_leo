/**
  ******************************************************************************
  * @file           : socket.h
  * @author         : 18483
  * @brief          : Socket封装
  * @attention      : None
  * @date           : 2025/2/24
  ******************************************************************************
  */


#ifndef SYLAR_SOCKET_H
#define SYLAR_SOCKET_H

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <boost/noncopyable.hpp>
//#include <openssl/err.h>
//#include <openssl/ssl.h>
#include "address.h"
#include "noncopyable.h"

namespace sylar {


/**
 * @brief Socket 封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, boost::noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief Socket类型
     */
    enum Type {
        /// TCP类型
        TCP = SOCK_STREAM,
        /// UDP类型
        UDP = SOCK_DGRAM
    };

    /**
     * @brief Socket协议族
     */
    enum Family {
        /// IPv4 socket
        IPv4 = AF_INET,
        /// IPv6 socket
        IPv6 = AF_INET6,
        /// UNIX socket
        UNIX = AF_UNIX
    };

    /**
     * @brief 创建 TCP Socket (满足地址类型)
     * @param address 地址
     */
    static Socket::ptr CreateTCP(sylar::Address::ptr address);

    /**
     * @brief 创建 UDP Socket (满足地址类型)
     * @param address 地址
     */
    static Socket::ptr CreateUDP(sylar::Address::ptr address);

    /**
     * @brief 创建IPv4的TCP Socket
     */
    static Socket::ptr CreateTCPSocket();

    /**
     * @brief 创建IPv4的UDP Socket
     */
    static Socket::ptr CreateUDPSocket();

    /**
     * @brief 创建IPv6的TCP Socket
     */
    static Socket::ptr CreateTCPSocket6();

    /**
     * @brief 创建IPv6的UDP Socket
     */
    static Socket::ptr CreateUDPSocket6();

    /**
     * @brief 创建Unix的TCP Socket
     */
    static Socket::ptr CreateUnixTCPSocket();

    /**
     * @brief 创建Unix的UDP Socket
     */
    static Socket::ptr CreateUnixUDPSocket();

    /**
     * @brief Socket 构造函数
     * @param family 协议族
     * @param type 类型
     * @param protocol 协议
     */
    Socket(int family, int type, int protocol = 0);

    /**
     * @brief 虚析构函数
     */
    virtual ~Socket();

    /**
     * @brief 获取和设置 发送超时时间
     */
    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);

    /**
     * @brief  获取和设置 接收超时时间
     */
    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    /**
     * @brief 获取 sockopt (套接字选项)
     */
    bool getOption(int level, int option, void* result, socklen_t* len);

    template<class T>
    bool getOption(int level, int option, T& result){
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    /**
     * @brief 设置 sockopt (套接字选项)
     */
    bool setOption(int level, int option, const void* result, socklen_t len);

    template<class T>
    bool setOption(int level, int option, const T& value){
        return setOption(level, option, &value, sizeof(T));
    }

    /**
     * @brief 接收connect链接
     * @return 成功返回新连接的socket,失败返回nullptr
     * @pre Socket必须 bind , listen  成功
     */
    virtual Socket::ptr accept();

    /**
     * @brief 绑定地址
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 连接地址
     * @param[in] addr 目标地址
     * @param[in] timeout_ms 超时时间(毫秒)
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    /**
     * @brief 监听socket
     * @param[in] backlog 未完成连接队列的最大长度
     * @result 返回监听是否成功
     * @pre 必须先 bind 成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 关闭socket
     */
    virtual bool close();

    /**
     * @brief 发送数据
     * @param buffer 待发送的数据内存
     * @param length 待发送数据的长度
     * @param flags 标志字
     * @return
     *          @retval >0 发送成功对应大小的数据
     *          @retval =0 socket被关闭
     *          @retval <0 socket出错
     */
    virtual int send(const void* buffer, size_t length, int flags = 0);
    /**
     * @param[in] buffers 待发送数据的内存(iovec数组)
     * @param[in] length 待发送数据的长度(iovec长度)
     */
    virtual int send(const iovec* buffers, size_t length, int flags = 0);
    /**
     * @param to 发送的目标地址
     */
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    /**
     * @brief 接收数据
     * @param[out] buffer 接收数据的内存
     * @param[in] length 接收数据的内存大小
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    virtual int recv(void* buffer, size_t length, int flags = 0);
    virtual int recv(iovec* buffers, size_t length, int flags = 0);
    /**
     * @param from 发送端地址
     */
    virtual int recvFrom(void* buffer, size_t length, const Address::ptr from, int flags = 0);
    virtual int recvFrom(iovec* buffers, size_t length, const Address::ptr from, int flags = 0);

    /**
     * @brief 获取远端地址
     */
    Address::ptr getRemoteAddress();

    /**
     * @brief 获取本地地址
     */
    Address::ptr getLocalAddress();

    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }
    bool isConnected() const {return m_isConnected;}

    /**
     * @brief 是否有效(m_sock != -1)
     */
    bool isValid() const;

    /**
     * @brief 返回Socket错误
     */
    int getError();

    /**
     * @brief 输出信息到流中
     */
    virtual std::ostream& dump(std::ostream& os) const;

    virtual std::string toString() const;

    /**
     * @brief 返回 socket 句柄
     */
    int getSocket() const {return m_sock;}

    /**
     * @brief 取消事件
     */
    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

protected:
    /**
     * @brief 初始化Socket
     */
    void initSock();

    /**
     * @brief 创建Socket
     */
    void newSock();
    /**
     * @brief 初始化sock
     */
    virtual bool init(int sock);
    
protected:
    /// socket 句柄
    int m_sock;
    /// 协议族
    int m_family;
    /// 类型
    int m_type;
    /// 协议
    int m_protocol;
    /// 是否连接
    bool m_isConnected;
    /// 本地地址
    Address::ptr m_localAddress;
    /// 远端地址
    Address::ptr m_remoteAddress;

};




}

#endif //SYLAR_SOCKET_H
