/**
  ******************************************************************************
  * @file           : http_parser.h
  * @author         : 18483
  * @brief          : HTTP协议解析封装
  * @attention      : None
  * @date           : 2025/3/19
  ******************************************************************************
  */


#ifndef SYLAR_HTTP_PARSER_H
#define SYLAR_HTTP_PARSER_H

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP请求解析类
 */
class HttpRequestParser{
public:
    /// HTTP解析类智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;
    /**
     * @brief 构造函数
     */
    HttpRequestParser();

    /**
     * @brief 解析协议
     * @param data 协议文本内存
     * @param len 协议文本内存长度
     * @return 返回实际解析的长度 并将已解析的数据移除
     */
    size_t execute(char* data, size_t len);

    /**
     * @brief 是否完成解析
     */
    int isFinished();

    /**
     * @brief 是否有错误
     */
    int hasError();

    /**
     * @brief 返回 HttpRequest 结构体
     */
    HttpRequest::ptr getData() const {return m_data;}

    /**
     * @brief 设置错误
     * @param v 错误值
     */
    void setError(int v) { m_error = v; }

    /**
     * @brief 获取消息体的长度
     */
    uint64_t getContentLength();

    /**
     * @brief 获取 http_parser 结构体
     */
    const http_parser& getParser() const {return m_parser;}

public:
    /**
     * @brief 返回HttpRequest协议解析的缓存大小
     */
    static uint64_t GetHttpRequestBufferSize();
    /**
     * @brief 返回HttpRequest协议的最大消息体大小
     */
    static uint64_t GetHttpRequestMaxBodySize();
private:
    /// http—parser
    http_parser m_parser;
    ///HttpRequest 结构
    HttpRequest::ptr m_data;
    /// 错误码
    /// 1000 : invalid method
    /// 1001 : invalid version
    /// 1002 : invalid field
    int m_error;
};


/**
 * @brief Http响应解析结构体
 */
class HttpResponseParser {
public:
    /// 智能指针
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /// 构造函数
    HttpResponseParser();

    /**
     * @brief 解析HTTP协议
     * @param data 协议文本内存
     * @param len 协议文本内存长度
     * @return 返回实际解析的长度 并将已解析的数据移除
     */
    size_t execute(char* data, size_t len , bool chunck);

    /**
     * @brief 是否完成解析
     */
    int isFinished();

    /**
     * @brief 是否有错误
     */
    int hasError();

    /**
     * @brief 返回 HttpRequest 结构体
     */
    HttpResponse::ptr getData() const {return m_data;}

    /**
     * @brief 设置错误
     * @param v 错误值
     */
    void setError(int v) { m_error = v; }

    /**
     * @brief 获取消息体的长度
     */
    uint64_t getContentLength();

    /**
     * @brief 获取 httpclient_parser 结构体
     */
    const httpclient_parser& getParser() const {return m_parser;}

public:
    /**
     * @brief 返回HttpRequest协议解析的缓存大小
     */
    static uint64_t GetHttpResponseBufferSize();
    /**
     * @brief 返回HttpRequest协议的最大消息体大小
     */
    static uint64_t GetHttpResponseMaxBodySize();
private:
    /// http—parser
    httpclient_parser m_parser;
    ///HttpRequest 结构
    HttpResponse::ptr m_data;
    /// 错误码
    /// 1001 : invalid version
    /// 1002 : invalid field
    int m_error;
};


}
}

#endif //SYLAR_HTTP_PARSER_H
