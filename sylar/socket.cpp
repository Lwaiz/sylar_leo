/**
  ******************************************************************************
  * @file           : socket.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/24
  ******************************************************************************
  */

#include "socket.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <limits.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


/// ************************* 创建各类型套接字 *********************** ///

Socket::ptr Socket::CreateTCP(sylar::Address::ptr address){
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(sylar::Address::ptr address){
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();   // 调用 newSock() 初始化套接字
    sock->m_isConnected = true;  // 设置 m_isConnected 为 true
    return sock;
}

Socket::ptr Socket::CreateTCPSocket(){
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket(){
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6(){
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6(){
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket(){
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket(){
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}


/**
 * @brief Socket 构造函数   协议族 类型 协议 是否连接
 */
Socket::Socket(int family, int type, int protocol)
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false){
}

/**
 * @brief 虚析构函数
 */
Socket::~Socket(){
    close();
}


/// ************************ 获取和设置套接字选项 *************************** ///

/**
 * @brief 获取和设置 发送超时时间
 */
int64_t Socket::getSendTimeout(){
    //获取套接字的文件描述符的 上下文
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        // 获取发送超时
        return ctx->getTimeout(SO_SNDTIMEO);  // SO_SNDTIMEO 发送超时   (定义在 POSIX 标准的头文件 <sys/socket.h> 中)
    }
    return -1;
}

void Socket::setSendTimeout(int64_t v){
    // 将超时时间转化为 timeval 结构，单位为秒和微秒
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    // 设置发送超时选项
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);   // SOL_SOCKET：套接字选项的级别（level）
}

/**
 * @brief  获取和设置 接收超时时间
 */
int64_t Socket::getRecvTimeout(){
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);   // SO_RCVTIMEO 接收超时
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v){
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

/**
 * @brief 获取 sockopt
 */
bool Socket::getOption(int level, int option, void* result, socklen_t* len){
    // 调用系统调用获取套接字选项
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                    << " level=" << level << " option=" << option
                    << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 设置 sockopt
 */
bool Socket::setOption(int level, int option, const void* result, socklen_t len){
    // 调用系统调用设置套接字选项
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)){
        SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                                  << " level=" << level << " option=" << option
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}


/// ************************ 连接与绑定 *************************** ///

/**
 * @brief 接收connect链接
 * @return 成功返回新连接的socket,失败返回nullptr
 * @pre Socket必须 bind , listen  成功
 */
Socket::ptr Socket::accept(){
    // 创建新的 Socket 连接对象
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    // 系统调用 ::accept 来接受传入的连接请求
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
                    << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    // 初始化套接字
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClose()){
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

/**
 * @brief 绑定地址
 * @param[in] addr 地址
 * @return 是否绑定成功
 */
bool Socket::bind(const Address::ptr addr){
    if(!isValid()) {  // 无效 重新创建
        newSock();
        if(SYLAR_UNLIKELY(!isValid())){ return false; }
    }
    // 检查协议是否匹配
    if(SYLAR_UNLIKELY(addr->getFamily() != m_family)) {
        SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }
    // 处理 Unix 套接字
    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if(sock->connect(uaddr)) {
            return false;
        } else {
            //sylar::
        }
    }

    // 系统调用进行绑定操作
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        SYLAR_LOG_ERROR(g_logger) << "bind error errno=" << errno
                << " errstr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

/**
 * @brief 连接到远程地址
 * @param[in] addr 目标地址
 * @param[in] timeout_ms 超时时间(毫秒)
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms){
    m_remoteAddress = addr;
    if(!isValid()) {
        newSock();
        if(SYLAR_UNLIKELY(!isValid())) { return false; }
    }

    if(SYLAR_UNLIKELY(addr->getFamily() != m_family)) {
        SYLAR_LOG_ERROR(g_logger) << "connect sock.family("
                                  << m_family << ") addr.family(" << addr->getFamily()
                                  << ") not equal, addr=" << addr->toString();
        return false;
    }

    // timeout_ms 设为 -1，则使用默认的 connect() 方式进行阻塞连接
    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") error errno=" << errno << "errstr=" << strerror(errno);
            close();
            return false;
        }
    } else { // 指定了超时时间 timeout_ms，则使用 connect_with_timeout() 进行超时控制
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                                      << ") timeout=" << timeout_ms << ") error errno="
                                      << errno << "errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}


bool Socket::reconnect(uint64_t timeout_ms){
    if(!m_remoteAddress) {
        SYLAR_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null.";
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}

/**
 * @brief 监听socket
 * @param[in] backlog 未完成连接队列的最大长度
 * @result 返回监听是否成功
 * @pre 必须先 bind 成功
 */
bool Socket::listen(int backlog){
    if(!isValid()) {
        SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    // 监听套接字，使其能够接受传入的连接请求
    if(::listen(m_sock, backlog)){
        SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno
                    << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

/**
 * @brief 关闭socket
 */
bool Socket::close(){
    if(!m_isConnected && m_sock == -1) {
        return true;    // 已经关闭，直接返回 true
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);   // 关闭文件描述符，释放资源
        m_sock = -1;
    }
    return false;
}


/// ************************ 数据发送和接收 *************************** ///

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
int Socket::send(const void* buffer, size_t length, int flags){
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}
/**
 * @param[in] buffers 待发送数据的内存(iovec数组)
 * @param[in] length 待发送数据的长度(iovec长度)
 */
int Socket::send(const iovec* buffers, size_t length, int flags){
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags){
    /// iovec 是一个结构体，用于描述一个数据缓冲区。每个 iovec 包含一个指向数据的指针和数据的长度
    if(isConnected()) {
        /// msghdr 结构体是发送消息所需的数据结构。它包含了指向 iovec 缓冲区的指针和目标地址的信息
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();   // 目标地址
        msg.msg_namelen = to->getAddrLen();  // 目标地址长度
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

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
int Socket::recv(void* buffer, size_t length, int flags){
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}
int Socket::recv(iovec* buffers, size_t length, int flags){
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, const Address::ptr from, int flags){
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, const Address::ptr from, int flags){
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief 获取远端地址
 */
Address::ptr Socket::getRemoteAddress(){
    if(m_remoteAddress){
        return m_remoteAddress;
    }
    // 创建一个适当类型的 Address 对象
    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    // 调用 getpeername 获取远程地址
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

/**
 * @brief 获取本地地址
 */
Address::ptr Socket::getLocalAddress(){
    if(m_localAddress){
        return m_localAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                        << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}


/**
 * @brief 是否有效(m_sock != -1)
 */
bool Socket::isValid() const{
    return m_sock != -1;
}

/**
 * @brief 返回Socket错误
 */
int Socket::getError(){
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

/**
 * @brief 输出信息到流中
 */
std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress){
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress){
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


/// ************************ 取消事件 *************************** ///

bool Socket::cancelRead(){
    return IOManager::GetThis()->cancleEvent(m_sock, sylar::IOManager::READ);
}

bool Socket::cancelWrite(){
    return IOManager::GetThis()->cancleEvent(m_sock, sylar::IOManager::WRITE);
}
bool Socket::cancelAccept(){
    return IOManager::GetThis()->cancleEvent(m_sock, sylar::IOManager::READ);
}

bool Socket::cancelAll(){
    return IOManager::GetThis()->cancleAll(m_sock);
}



void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}


void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(SYLAR_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "socke(=" << m_family << ", "
                                  << m_type << ", "  << m_protocol << ") errno="
                                  << errno << " errstr=" << strerror(errno);
    }
}


}
