/**
  ******************************************************************************
  * @file           : hook.h
  * @author         : 18483
  * @brief          : Hook模块    (使用我们写的同名异步操作的函数 替换系统调用)
  * @attention      : 通过 Hook 机制拦截同步的系统调用（如 connect、read、write 等），
  *                   将其转换为异步操作，同时利用协程或事件循环来模拟异步行为
  * @date           : 2025/2/21
  ******************************************************************************
  */


#ifndef SYLAR_HOOK_H
#define SYLAR_HOOK_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

namespace sylar {

    /**
     * @brief 当前线程是否 hook
     */
    bool is_hook_enable();

    /**
     * @brief 设置当前线程的hook状态
     * @param flag
     */
    void set_hook_enable(bool flag);

}

extern "C" {

/// sleep 用于让进程休眠指定的时间
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

/// socket
// 创建一个套接字，并返回一个描述符，该描述符可以用来访问该套接字
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

// 让客户程序通过在一个未命名套接字和服务器监听套接字之间建立连接的方法来连接到服务器
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

// 等待客户建立对该套接字的连接
typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

/// read
// 从文件描述符（包括TCP Socket）中读取数据，并将读取的数据存储到指定的缓冲区中
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

// 文件I/O操作，从文件描述符读取数据到多个缓冲区中
typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

// 从套接字接收数据，并提供了额外的Flags参数来控制特殊行为
typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

// 接收来自套接字（socket）的数据，并且适用于面向无连接的协议，如 UDP（用户数据报协议）。
// recvfrom 允许程序接收数据同时获取数据发送方的地址信息
typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

// 提供了更为复杂和灵活的功能，支持分散/聚集I/O和更多的控制选项
typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

/// write
// 向任意文件描述符中写入(读取)数据，用作socket发送数据时，只能向已经建立连接的文件描述符中写入(读取)数据
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

// 向任意文件描述符中写入多个缓冲区的数据，readv用于从任意描述符中向多个缓冲区读取数据，
// 用作socket发送数据时，只能向已经建立连接的文件描述符中写入(读取)数据
typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

// 向socket中写入(读取)数据，只能用于已经建立连接的socket上，udp也可以调用connect建立连接
typedef ssize_t (*send_fun)(int s,const void *msg, size_t len, int flags);
extern send_fun send_f;

//用于向socket中写入(读取)数据，如果用在已经建立连接的socket上，需要忽略其地址和地址长度参数，
//即地址指针设置为NULL，地址长度设置为0；如udp，如果不调用connec建立连接，则需要指定地址参数，
//如果调用connect建立了连接，则省略地址参数
typedef ssize_t (*sendto_fun)(int s,const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_fun sendto_f;

//用于向socket文件描述符中写入多个缓冲区的数据，
//用于向多个缓冲区读取socket文件描述符中的数据，发送(接收)前需要构造msghdr消息头
typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

/// close
//一个套接字的默认行为是把套接字标记为已关闭，然后立即返回到调用进程，该套接字描述符不能再由调用进程使用，
//也就是说它不能再作为read或write的第一个参数，然而TCP将尝试发送已排队等待发送到对端，
//发送完毕后发生的是正常的TCP连接终止序列
typedef int (*close_fun)(int fd);
extern close_fun close_f;

/// io控制相关

// 对已打开的文件描述符进行各种控制操作，包括复制、设置标志、非阻塞操作等
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
extern  fcntl_fun fcntl_f;

// 对设备进行操作和控制
typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

/// sockopt 相关

// 获取任意类型、任意状态套接口的选项当前值，并把结果存入optval
typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

// 用于任意类型、任意状态套接口的设置选项值
typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

/// 自定义
// 连接超时设置
extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

}

#endif //SYLAR_HOOK_H
