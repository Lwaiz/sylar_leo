/**
  ******************************************************************************
  * @file           : config_servlet.h
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/4/3
  ******************************************************************************
  */


#ifndef SYLAR_CONFIG_SERVLET_H
#define SYLAR_CONFIG_SERVLET_H

#include "sylar/http/servlet.h"

namespace sylar {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
            , sylar::http::HttpResponse::ptr response
            , sylar::http::HttpSession::ptr session) override;
};



}
}


#endif //SYLAR_CONFIG_SERVLET_H
