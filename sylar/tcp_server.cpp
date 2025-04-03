/**
  ******************************************************************************
  * @file           : tcp_server.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/1
  ******************************************************************************
  */


#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace sylar {

/// tcp_server 读超时时间
static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
        sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t) (60 * 1000 * 2),
                              "tcp server read timeout");
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

TcpServer::TcpServer(sylar::IOManager *worker,
                     sylar::IOManager *io_worker,
                     sylar::IOManager *accept_worker)
        :m_worker(worker)
        ,m_ioWorker(io_worker)
        ,m_acceptWorker(accept_worker)
        ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
        ,m_name("sylar/1.0.0")
        ,m_isStop(true) {
}

TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

void TcpServer::setConf(const TcpServerConf& v) {
    m_conf.reset(new TcpServerConf(v));
}

bool TcpServer::bind(sylar::Address::ptr addr, bool ssl) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}

/// <addrs:需要绑定的地址列表，fails:绑定、监听失败的地址列表，ssl:是否启用ssl连接>
// 遍历 addrs，为每个地址创建 TCP Socket（支持 SSL）
// 尝试 绑定 (bind)，如果失败，则记录错误日志并存入 fails
// 绑定成功后尝试 监听 (listen)，如果失败，同样记录日志并存入 fails
// 如果有地址都绑定失败，则清空 m_socks，返回 false (防止部分地址绑定失败，不稳定)
// 绑定成功的 Socket 记录日志，并返回 true
bool TcpServer::bind(const std::vector<Address::ptr>& addrs
        ,std::vector<Address::ptr>& fails
        ,bool ssl) {
    m_ssl = ssl;  // 设置ssl模式
    // 遍历传入的所有地址，尝试绑定
    for(auto& addr : addrs) {
        // 根据是否使用 SSL 选择创建普通 TCP Socket 还是 SSL Socket
        Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        // Socket::ptr sock = Socket::CreateTCP(addr);
        // 绑定 socket 到指定地址
        if(!sock->bind(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                                      << errno << " errstr=" << strerror(errno)
                                      << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);  // 绑定失败添加到 fails 列表
            continue;
        }
        // 绑定成功后，开始监听
        if(!sock->listen()) {
            SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                                      << errno << " errstr=" << strerror(errno)
                                      << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);  // 监听失败 添加到 fails 列表
            continue;
        }
        // 绑定并监听成功的 socket 存入服务器的 socket 列表
        m_socks.push_back(sock);
    }
    // 如果存在绑定或监听失败的地址，清空成功绑定的 socket，并返回 false
    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }
    // 绑定成功的 socket 记录日志信息
    for(auto& i : m_socks) {
        SYLAR_LOG_INFO(g_logger) << "type=" << m_type
                                 << " name=" << m_name
                                 << " ssl=" << m_ssl
                                 << " server bind success: " << i;
    }
    return true;
}

// 在循环中不断调用 accept()，等待新的客户端连接
void TcpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop){    // 当服务器未停止时，持续接受连接
        Socket::ptr client = sock->accept();   // 接收客户端连接
        if(client){  // 当有新连接时，将其交给 m_ioWorker 处理
            client->setRecvTimeout(m_recvTimeout); // 设置接收超时时间
            // 分配任务给 IO 线程处理
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                          shared_from_this(), client));
        } else {
            SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
    }
}
// 启动 TcpServer，监听所有 socket 并开启 accept() 进程
bool TcpServer::start() {
    if(!m_isStop){      // 如果已经启动，则直接返回 true
        return true;
    }
    m_isStop = false;   // 设定服务器为运行状态
    for(auto& sock: m_socks){  // 遍历所有监听的 socket
        // 在 accept 线程池中执行 startAccept
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                                           shared_from_this(), sock));
    }
    return true;
}

// 停止服务器，关闭所有 socket，并清理资源
void TcpServer::stop() {
    m_isStop = true;  // 设定服务器为停止状态
    // 获取智能指针，确保 TcpServer 不被释放
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {  // 在 accept 线程池中执行关闭逻辑
        for (auto& sock : m_socks) {  // 遍历所有 socket
            sock->cancelAll();  // 取消所有事件（读写等）
            sock->close();  // 关闭 socket 连接
        }
        m_socks.clear();  // 清空已监听的 socket
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client;
}

bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    for(auto& i : m_socks) {
        auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
        if(ssl_socket) {
            if(!ssl_socket->loadCertificates(cert_file, key_file)) {
                return false;
            }
        }
    }
    return true;
}

std::string TcpServer::toString(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name << " ssl=" << m_ssl
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << i << std::endl;
    }
    return ss.str();
}

}



