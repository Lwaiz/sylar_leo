/**
  ******************************************************************************
  * @file           : http_session.cpp
  * @author         : 18483
  * @brief          : HTTP 服务器类封装
  * @attention      : None
  * @date           : 2025/4/2
  ******************************************************************************
  */

#include "http_session.h"
#include "http_parser.h"

namespace sylar {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
        : SocketStream(sock, owner) {
}
/// 解析 HTTP 请求
HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);  // 创建HTTP请求解析器
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize(); // 获取缓冲区大小
    // 使用智能指针管理缓冲区，防止内存泄漏
    std::shared_ptr<char> buffer( new char[buff_size], [](char* ptr){delete[] ptr;});
    char* data = buffer.get();  // 获取data指针
    int offset = 0;
    do {     // 读取HTTP请求数据
        int len = read(data + offset, buff_size - offset); // read读取数据
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len); // execute 解析请求
        if(parser->hasError()) {    // 检查解析是否出错
            close();
            return nullptr;
        }
        // 处理沾包
        offset = len - nparse;
        if(offset == (int)buff_size) {   // 说明缓冲区满了，还未解析完 <HTTP请求超长或格式错误>
            close();                     // 关闭连接
            return nullptr;
        }
        if(parser->isFinished()) {       // 检查解析是否完成
            break;
        }
    } while(true);
    // 处理 HTTP 请求体
    int64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);
        // 如果offset已经包含完整的 Content-Length 数据，直接memcpy()复制
        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        // 如果length>0，调用readFixSize()读取剩余部分
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }  // 设置请求体 & 返回 HttpRequest
    parser->getData()->init();
    return parser->getData();
}


int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();  // 将 HttpResponse 序列化为字符串
    // 调用 writeFixSize() 发送响应数据 <保证完整发送>
    return writeFixSize(data.c_str(), data.size());
}


}
}