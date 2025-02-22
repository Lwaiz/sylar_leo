/**
  ******************************************************************************
  * @file           : test_hook.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/21
  ******************************************************************************
  */


#include "../sylar/hook.h"
#include "../sylar/log.h"
#include "../sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_sleep() {
    sylar::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        SYLAR_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        SYLAR_LOG_INFO(g_logger) << "sleep 3";
    });
    SYLAR_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock(){
    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // 声明接口地址
    sockaddr_in addr;
    // 分配空间
    memset(&addr, 0, sizeof(addr));
    // 设置 组族
    addr.sin_family = AF_INET;
    // 设置端口
    addr.sin_port = htons(80);
    // 设置IP IP地址转化函数
    inet_pton(AF_INET, "36.155.132.76", &addr.sin_addr.s_addr);

    SYLAR_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    SYLAR_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) { return;}

    // 发送数据
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
    if(rt <= 0) {return;}

    // 接受数据
    std::string buff;
    buff.resize(4096);
    rt = recv(sock, &buff[0], buff.size(), 0);
    SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;
    if(rt <= 0) {return;}

    buff.resize(rt);
    //SYLAR_LOG_INFO(g_logger) << buff;

}


int main(int argc, char** argv) {
    //test_sleep();

    sylar::IOManager iom;
    iom.schedule(test_sock);

    return 0;
}