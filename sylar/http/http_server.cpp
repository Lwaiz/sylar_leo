/**
  ******************************************************************************
  * @file           : http_server.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */


#include "http_server.h"
#include "sylar/log.h"
#include "sylar/http/servlets/config_servlet.h"
#include "sylar/http/servlets/status_servlet.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive
        ,sylar::IOManager* worker
        ,sylar::IOManager* io_worker
        ,sylar::IOManager* accept_worker)
        :TcpServer(worker, io_worker, accept_worker)
        ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    m_type = "http";
    m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

/// 处理请求
void HttpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
    // 创建 HttpSession 来处理 HTTP 请求
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest(); // 读取 HTTP 请求
        if(!req) {
            SYLAR_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                                      << errno << " errstr=" << strerror(errno)
                                      << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }
        // 创建 HttpResponse 并设置 Server 头部信息
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        // m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);  // 发送 HTTP 响应
        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close(); // 关闭会话
}



}
}