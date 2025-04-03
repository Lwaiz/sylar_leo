/**
  ******************************************************************************
  * @file           : socket_stream.h
  * @author         : 18483
  * @brief          : Socket流式接口封装
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */


#ifndef SYLAR_SOCKET_STREAM_H
#define SYLAR_SOCKET_STREAM_H


#include "sylar/stream.h"
#include "sylar/socket.h"
#include "sylar/mutex.h"
#include "sylar/iomanager.h"

namespace sylar {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;
    /**
     * @brief 构造函数
     * @param[in] sock Socket类
     * @param[in] owner 是否完全控制
     */
    SocketStream(Socket::ptr sock, bool owner = true);
    /**
     * @brief 析构函数
     * @details 如果m_owner=true,则close
     */
    ~SocketStream();
    /// 核心读写函数
    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;
    /**
     * @brief 获取Socket对象
     */
    Socket::ptr getSocket() const {return m_socket;}
    /**
     * @brief 返回是否连接
     */
    bool isConnected() const;
    /// 获取本地/远程地址
    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    std::string getRemoteAddressString();
    std::string getLocalAddressString();
protected:
    /// Socket 类
    Socket::ptr m_socket;
    /// 是否主控
    bool m_owner;
};

}




#endif //SYLAR_SOCKET_STREAM_H
