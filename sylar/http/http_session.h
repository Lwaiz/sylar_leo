/**
  ******************************************************************************
  * @file           : http_session.h
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */


#ifndef SYLAR_HTTP_SESSION_H
#define SYLAR_HTTP_SESSION_H

#include "sylar/streams/socket_stream.h"
#include "http.h"

namespace sylar {
namespace http {

/**
 * @brief HttpSession封装
 */
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);
    /**
     * @brief 接收HTTP请求
     */
    HttpRequest::ptr recvRequest();
    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);

};

}
}

#endif //SYLAR_HTTP_SESSION_H
