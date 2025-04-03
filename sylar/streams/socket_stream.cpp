/**
  ******************************************************************************
  * @file           : socket_stream.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */

#include "socket_stream.h"
#include "sylar/util.h"

namespace sylar {

SocketStream::SocketStream(Socket::ptr sock, bool owner)
    :m_socket(sock), m_owner(owner){}

SocketStream::~SocketStream(){
    if(m_owner && m_socket){
        m_socket->close();
    }
}

bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}
/// 从 Socket 读取 length 字节到 buffer
int SocketStream::read(void *buffer, size_t length) {
    if(!isConnected()) return -1;
    return m_socket->recv(buffer, length);  // 调用 m_socket->recv()接收数据
}

int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) return -1;
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length); // 获取 可写入的 iovec 数组
    int rt = m_socket->recv(&iovs[0], iovs.size()); // 读取数据
    if(rt > 0){
        ba->setPosition(ba->getPosition() + rt);  // 更新读取位置
    }
    return rt;
}

int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_socket->send(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

void SocketStream::close() {
    if(m_socket) {
        m_socket->close();
    }
}

Address::ptr SocketStream::getRemoteAddress() {
    if(m_socket) {
        return m_socket->getRemoteAddress();
    }
    return nullptr;
}

Address::ptr SocketStream::getLocalAddress() {
    if(m_socket) {
        return m_socket->getLocalAddress();
    }
    return nullptr;
}

std::string SocketStream::getRemoteAddressString() {
    auto addr = getRemoteAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}

std::string SocketStream::getLocalAddressString() {
    auto addr = getLocalAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}


}

