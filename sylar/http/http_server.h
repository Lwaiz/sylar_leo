/**
  ******************************************************************************
  * @file           : http_server.h
  * @author         : 18483
  * @brief          : HTTP服务器封装
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */


#ifndef SYLAR_HTTP_SERVER_H
#define SYLAR_HTTP_SERVER_H

#include "sylar/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP 服务器类
 */
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    /**
     * @brief 构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = false
                ,sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());
    /**
     * @brief 获取ServletDispatch
     */
    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}
    /**
     * @brief 设置ServletDispatch
     */
    void setServletDispatch(ServletDispatch::ptr v) {m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepalive;  // 支持长连接
    ServletDispatch::ptr m_dispatch; /// Servlet分发器
};

}
}


#endif //SYLAR_HTTP_SERVER_H
