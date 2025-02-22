/**
  ******************************************************************************
  * @file           : fd_manager.h
  * @author         : 18483
  * @brief          : 文件句柄管理类
  * @attention      : None
  * @date           : 2025/2/21
  ******************************************************************************
  */


#ifndef SYLAR_FD_MANAGER_H
#define SYLAR_FD_MANAGER_H

#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"

namespace sylar {

/*
 * FdCtx 存储每一个 fd 相关的信息
 * FdManager(单例类) 管理每一个 FdCtx
 */


/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型 (是否 socket)
 *          是否阻塞，是否关闭，读写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    /**
     * @brief 通过文件句柄构造 FdCtx
     * @param fd
     */
    FdCtx(int fd);

    /**
     * @brief 析构函数
     */
    ~FdCtx();

    /**
     * @brief 是否完成初始化
     */
    bool isInit() const {return m_isInit;}

    /**
     * @brief 是否 socket
     */
    bool isSocket() const { return m_isSocket; }

    /**
     * @brief 是否已关闭
     */
    bool isClose() const {return m_isClosed;}

    /**
     * @brief 设置系用户主动设置非阻塞
     * @param v 是否阻塞
     */
    void setUserNonblock(bool v) { m_userNonblock = v; }

    /**
     * @brief 获取是否用户主动设置的非阻塞
     */
    bool getUserNonblock() const { return m_userNonblock; }

    /**
     * @brief 设置系统非阻塞
     * @param v 是否阻塞
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    /**
     * @brief 获取系统非阻塞
     */
    bool getSysNonblock() const { return m_sysNonblock; }

    /**
     * @brief 设置超时时间
     * @param type 类型 SO_RCVTIMEO(读超时)，SO_SNDTIMEO(写超时)
     * @param v 时间毫秒
     */
    void setTimeout(int type, uint64_t v);

    /**
     * @brief 获取超时时间 毫秒
     */
    uint64_t getTimeout(int type);

private:
    /**
     * @brief 初始化
     */
    bool init();

private:
    /// 是否初始化
    bool m_isInit : 1;
    /// 是否 socket
    bool m_isSocket : 1;
    /// 是否hook 非阻塞
    bool m_sysNonblock : 1;
    /// 是否用户主动设置非阻塞
    bool m_userNonblock : 1;
    /// 是否关闭
    bool m_isClosed : 1;
    /// 文件句柄
    int m_fd;
    /// 读超时时间毫秒
    uint64_t m_recvTimeout;
    /// 写超时时间毫秒
    uint64_t m_sendTimeout;
};


/**
 * @brief 文件句柄管理类
 */
class FdManager {
public:
    typedef RWMutex RWMutexType;

    /**
     * @brief 无参构造函数
     */
    FdManager();

    /**
     * @brief 获取/创建文件句柄类 FdCtx
     * @param fd 文件句柄
     * @param auto_create 是否自动创建
     * @return返回对应文件句柄类 FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    /**
     * @brief 删除文件句柄类
     * @param fd 文件句柄
     */
    void del(int fd);

private:
    /// 读写锁
    RWMutexType m_mutex;
    /// 文件句柄集合
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}

#endif //SYLAR_FD_MANAGER_H
