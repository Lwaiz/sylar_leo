/**
  ******************************************************************************
  * @file           : hook.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/21
  ******************************************************************************
  */

#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
        sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)             \
    XX(connect)             \
    XX(accept)             \
    XX(read)             \
    XX(readv)             \
    XX(recv)             \
    XX(recvfrom)             \
    XX(recvmsg)             \
    XX(write)             \
    XX(writev)             \
    XX(send)             \
    XX(sendto)             \
    XX(sendmsg)             \
    XX(close)             \
    XX(fcntl)             \
    XX(ioctl)             \
    XX(getsockopt)             \
    XX(setsockopt)

/// 获取接口原始地址 (在main函数运行前完成)
void hook_init() {
    static bool is_inited = false;
    if(is_inited){
        return;
    }
// dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
// RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
// 取出原函数，赋值给新函数
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

/**
 * @brief 将hook_init()封装到一个结构体的构造函数中，
 *   并创建静态对象，能够在main函数运行之前就能将地址保存到函数指针变量当中。
 */
static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) -> void {
            SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                     << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};
static _HookIniter s_hook_initer;


/// 获取是否 hook
bool is_hook_enable() {
    return t_hook_enable;
}

/// 控制是否开启 hook
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}


}


struct timer_info {
    int cancelled = 0;
};


/// hook的核心函数  I/O操作
/// 以写同步的方式实现异步的效果
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                     uint32_t event, int timeout_so, Args&&... args){
    /**
     * 1.先进行一系列判断 是否按原函数执行
     * 2.执行原函数 若errno = EINTR，则为系统中断，应该不断重新尝试操作
     * 3.若errno = EAGIN，系统已经隐式的将socket设置为非阻塞模式，此时资源咱不可用
     * 4.若设置超时时间 则设置一个条件定时器，若超时时间前来数据，则tinfo已销毁，不会执行回调
     * 5.在条件定时器的回调函数中设置错误为ETIMEDOUT超时，使用cancelEvent强制执行该任务，继续回到该协程执行
     * 6.addEvent添加事件，成功则让出协程执行权
     * 7.只有两种情况协程会被拉起： - 超时了，通过定时器回调函数 cancelEvent唤醒回来 - addEvent数据回来了会唤醒回来
     * 8.取消定时器 超时则返回-1
     * 9.若数据来了 则 retry 重新操作
     */
    // 如果不需要hook 直接返回原始接口
    if(!sylar::t_hook_enable){
        // forward完美转发，可以将传入的可变参数args以原始类型的方式传递给函数fun
        // 可以避免不必要的类型转换和拷贝，提高代码的效率和性能
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取 fd 对应的 FdCtx
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    // 没有文件 直接返回原始接口
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 检查句柄是否关闭
    if(ctx->isClose()){
        errno = EBADF;
        return -1;
    }

    // 如果不是socket或者用户设置了非阻塞 直接调用原始函数
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    // 获取超时时间 设置超时条件
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    // 先调用原始函数读/写 数据 若函数返回值有效 则直接返回
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    // 中断 则重试
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    //若为阻塞状态
    if(n == -1 && errno == EAGAIN) {
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;
        // tinfo 的弱指针 可以用来判断tinfo是否已经销毁
        std::weak_ptr<timer_info> winfo(tinfo);

        // 若超时时间不为-1， 则设置定时器
        if(to != (uint64_t)-1) {
            // 添加条件定时器  ---to时间消息还没来就触发回调
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                auto t = winfo.lock();
                // tinfo失效 || 设了错误   定时器失效了
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT; // 没错误的话设置为超时而失败
                // 取消事件进行强制唤醒
                iom->cancleEvent(fd, (sylar::IOManager::Event)(event));
            }, winfo);
        }
        // 添加事件
        int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
        // 添加失败
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) { timer->cancle(); }  // 取消定时器
            return -1;
        } else {
            /* addEvent 成功，把执行时间让出来
             *   只有两种情况会回来:
             *       1) 超时了， timer cancelEvent triggerEvent 强制唤醒
             *       2) addEvent 数据回来了就会唤醒回来
             */
            //SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
            sylar::Fiber::YiledToHold();
            //SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
            if(timer) { timer->cancle(); }
            // 从定时任务唤醒 超时失败
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            // 数据来了就直接重新操作
            goto retry;
        }
    }
    return n;
}



/// 声明变量
extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX


// sleep
/// 创建一个定时器，并让出执行权，等到定时器超时 再返回来
unsigned int sleep(unsigned int seconds){
    if(!sylar::t_hook_enable) {
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    // bind() 需要先确定 schedule 函数的类型
    iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)
                        (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule,
                  iom, fiber, -1));
    sylar::Fiber::YiledToHold();
    return 0;
}


int usleep(useconds_t usec){
    if(!sylar::t_hook_enable) {
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(sylar::Scheduler::*)
                        (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule,
                                            iom, fiber, -1));
    sylar::Fiber::YiledToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!sylar::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec /1000 /1000;
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(sylar::Scheduler::*)
                                                 (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule,
                                         iom, fiber, -1));
    sylar::Fiber::YiledToHold();
    return 0;
}


// socket

int socket(int domain, int type, int protocol) {
    if(!sylar::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    // 将 fd 放入到文件管理中
    sylar::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

// socket 连接

/**
 * 等待超时或套接字可写，如果先超时，则条件变量winfo仍然有效，通过winfo来设置超时标志并触发WRITE事件，
 * 协程从yield点返回，返回之后通过超时标志设置errno并返回-1；如果在未超时之前套接字就可写了，
 * 那么直接取消定时器并返回成功。取消定时器会导致定时器回调被强制执行一次，但这并不会导致问题，
 * 因为只有当前协程结束后，定时器回调才会在接下来被调度，由于定时器回调被执行时connect_with_timeout协程已经执行完了，
 * 所以理所当然地条件变量也被释放了，所以实际上定时器回调函数什么也没做。
 */
int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!sylar::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    /// 获取文件描述符上下文
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    // 上下文无效或已关闭
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    // 不是套接字
    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    // 用户态设置了非阻塞模式 直接调用系统函数
    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    // 调用原始 connect_f 尝试连接
    int n = connect_f(fd, addr, addrlen);
    if(n == 0) {  // 连接成功 直接返回
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n; // 连接失败  且不是EINPROGRESS 直接返回错误
    }

    // 定义定时器和超市信息
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    // 若超时时间不为-1， 则设置定时器
    if(timeout_ms != (uint64_t)-1) {
        // 添加条件定时器  ---to时间消息还没来就触发回调
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() ->void {
            auto t = winfo.lock();
            // tinfo失效 || 设了错误   定时器失效了
            if(!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT; // 没错误的话设置为超时而失败
            // 取消事件进行强制唤醒
            iom->cancleEvent(fd, sylar::IOManager::WRITE);
        }, winfo);
    }
    // 添加写事件， 等待连接完成
    int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
    // 添加成功 让出执行权 等待事件触发
    if(rt == 0){
        sylar::Fiber::YiledToHold();
        if(timer) {
            timer->cancle();  // 取消定时器
        }
        if(tinfo->cancelled){  // 连接超时 返回错误
            errno = tinfo->cancelled;
            return -1;
        }
    } else {   // 事件添加失败
        if(timer) {
            timer->cancle();
        }
        SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    // 检查套接字错误状态
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else { // 设置错误码 并返回失败
        errno = error;
        return -1;
    }
}


int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
}


int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        sylar::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
}


ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}


ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}


ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}


ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
}


/// write
ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}


ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}


ssize_t send(int s, const void *msg, size_t len, int flags){
    return do_io(s, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}


ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
    return do_io(s, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}


ssize_t sendmsg(int s, const struct msghdr *msg, int flags){
    return do_io(s, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}


int close(int fd) {
    if(!sylar::t_hook_enable){
        return close_f(fd);
    }
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(ctx){
        auto iom = sylar::IOManager::GetThis();
        if(iom){
            // 取消所有事件
            iom->cancleAll(fd);
        }
        // 在文件管理中删除fd
        sylar::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}


/// 对用户反馈是否是用户设置的非阻塞模式
int fcntl(int fd, int cmd, ... /* arg */ ){
    // 可变参数列表
    va_list va;
    va_start(va, cmd); // 初始化可变参数列表
    // 根据 cmd 不同 执行不同的逻辑
    switch(cmd) {
        case F_SETFL:   // F_SETFL: 设置文件描述符的标志
        {
            int arg = va_arg(va, int); // 获取一个整数参数标志值
            va_end(va); // 结束可变参数列表的使用

            // 获取文件描述符上下文
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            // 上下文无效、文件描述符已关闭或不是套接字
            if(!ctx || ctx->isClose() || !ctx->isSocket()){
                return fcntl_f(fd, cmd, arg);  // 调用系统函数
            }
            /// 更新用户态的非阻塞标志
            ctx->setUserNonblock(arg & O_NONBLOCK);
            /// 根据系统态的非阻塞标志调整参数
            if(ctx->getSysNonblock()) {
                arg |= O_NONBLOCK;  // 如果系统态是非阻塞，设置 O_NONBLOCK 标志
            } else {
                arg &= ~O_NONBLOCK; // 否则 清除标志
            }
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFL:  // F_GETFL: 获取文件描述符的标志
        {
            va_end(va);
            // 调用底层 fcntl_f 获取文件描述符的标志
            int arg = fcntl_f(fd, cmd);
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()){
                return arg;
            }
            /// 根据用户态的非阻塞标志调整返回值
            if(ctx->getUserNonblock()) {
                 return arg | O_NONBLOCK;  // 如果用户态是非阻塞，设置 O_NONBLOCK 标志
            } else {
                return arg & ~O_NONBLOCK; // 否则 清除标志
            }
        }
            break;
        /// 其他命令直接调用底层 fcntl_f 不做额外处理
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            // 这些命令需要传递一个整数参数
            int arg = va_arg(va, int);
            va_end(va);
            // 直接调用底层 fcntl_f 并返回结果
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            // 这些命令不需要额外参数
            va_end(va);
            return fcntl_f(fd, cmd);
        }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            // 这些命令需要传递一个 flock 结构体指针
            struct flock* arg = va_arg(va, struct flock*); // 从可变参数列表中获取一个 flock 结构体指针
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            // 这些命令需要传递一个 f_owner_exlock 结构体指针
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        default:
            // 未知命令 直接调用底层返回
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

/// 对设备进行控制操作
int ioctl(int d, unsigned long int request, ...){
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    ///	FIONBIO用于设置文件描述符的非阻塞模式
    // 检查request 是否为 FIONBIO
    if(FIONBIO == request) {
        // 将 arg 转换为 int 指针，并解引用获取其值
        // !! 用于将值转换为布尔类型（0 或 1）
        bool user_nonblock = !!*(int*)arg;
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(d);
        // 直接调用底层函数
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        // 更新用户态 非阻塞标志
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}


int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}


int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
    if(!sylar::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
            if(ctx){
                const timeval* v = (const timeval*) optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
