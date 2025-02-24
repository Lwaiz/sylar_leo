/**
  ******************************************************************************
  * @file           : test_iomanager.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/16
  ******************************************************************************
  */


#include "../sylar/sylar.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber(){
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // 设置为非阻塞模式
    fcntl(sock, F_SETFL, O_NONBLOCK);
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

    // 建立连接
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ,[](){
            SYLAR_LOG_INFO(g_logger) << "read callback";
        });
        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE,[](){
            SYLAR_LOG_INFO(g_logger) << "write callback";

            sylar::IOManager::GetThis()->cancleEvent(sock, sylar::IOManager::READ);
            close(sock);
        });
    }
    else {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << strerror(errno);
     }

}

void test1(){
    sylar::IOManager iom(4, false,"thread");
    iom.schedule(&test_fiber);
}

sylar::Timer::ptr s_timer;
void test_timer() {
    sylar::IOManager iom(2, false, "timer");
    s_timer = iom.addTimer(1000, []()->void{
        static int i = 0;
        SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            //s_timer->reset(2000, true);
            s_timer->cancle();
        }
    }, true);
}

int main(int argc, char** argv) {
    //test1();

    test_timer();

    return 0;
}
