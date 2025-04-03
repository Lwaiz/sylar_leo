/**
  ******************************************************************************
  * @file           : status_servlet.h
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/3
  ******************************************************************************
  */


#ifndef SYLAR_STATUS_SERVLET_H
#define SYLAR_STATUS_SERVLET_H

#include "sylar/http/servlet.h"

namespace sylar {
namespace http {

class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
            , sylar::http::HttpResponse::ptr response
            , sylar::http::HttpSession::ptr session) override;
};

}
}

#endif //SYLAR_STATUS_SERVLET_H
