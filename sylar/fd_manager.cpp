/**
  ******************************************************************************
  * @file           : fd_manager.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/21
  ******************************************************************************
  */


#include "fd_manager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sylar {

FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx(){
}

/// 初始化文件描述符上下文
bool FdCtx::init() {
    // 初始化检查
    if(m_isInit) {
        return true;
    }
    // 设置超时时间 -1表示未设置
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    // 检查文件描述符类型
    struct stat fd_stat;
    /// fstat 获取文件描述符信息
    // -1 表示获取失败 ； 0 获取成功
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        // 判断是否为 socket
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    // 如果是socket，设置非阻塞模式
    if(m_isSocket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        // 如果用户没有设置非阻塞 设置为非阻塞
        if(!(flags & O_NONBLOCK)) {
            fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }
    // 初始化其他成员变量
    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}


/**
 * @brief 设置超时时间
 * @param type 类型 SO_RCVTIMEO(读超时)，SO_SNDTIMEO(写超时)
 * @param v 时间毫秒
 */
void FdCtx::setTimeout(int type, uint64_t v){
    if(type == SO_RCVTIMEO) {  // 读超时
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

/**
 * @brief 获取超时时间 毫秒
 */
uint64_t FdCtx::getTimeout(int type){
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;  // 读超时
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

/// 获取/创建文件句柄类 FdCtx
FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) { return nullptr; }

    // 集合中没有，并且不自动创建，返回nullptr
    RWMutexType ::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        if(auto_create == false) {
            return nullptr;
        }
    } else {
        // 集合中有，直接返回
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

    // 创建新的FdCtx
    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));  // 扩容
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    // 将新的FdCtx放入集合中
    m_datas[fd] = ctx;
    return ctx;
}

/// 删除 fd 上下文
void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset(); // 重置 fd
}

}