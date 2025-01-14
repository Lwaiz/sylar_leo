//
// Created by 18483 on 2025/1/6.
//


#ifndef SYLAR_LOG_H
#define SYLAR_LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"

///宏定义

/**
 * @brief 使用流方式将日志级别 level 的日志写到 logger
 *
 * 说明：
 * - 如果 logger 的日志级别大于指定的 level，则不会输出日志。
 * - 通过 `sylar::LogEventWrap` 创建一个日志事件，并通过流方式将日志信息追加到日志流中。
 * - 主要目的是为不同日志级别提供统一的日志输出接口。
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level)    \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent( \
                                logger, level, __FILE__, __LINE__, 0,  \
                                sylar::GetThreadId(),          \
                                sylar::GetFiberId(),           \
                                time(0),                       \
                                "main_thread"))).getSS()
/**
 * @brief 使用流方式将日志级别 debug，info，warn，error，fatal 的日志写到 logger
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

/**
 * @brief 使用 格式化 方式将日志级别 level 的日志写入到 logger
 *
 * 说明：
 * - 如果 logger 的日志级别大于指定的 level，则不会输出日志。
 * - 通过 `sylar::LogEventWrap` 创建一个日志事件，并调用日志事件的 `format` 方法进行格式化。

 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level)                  \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent( \
        logger, level, __FILE__, __LINE__,           \
        0, sylar::GetThreadId(),                     \
        sylar::GetFiberId(),                         \
        time(0),                                     \
        "main_thread"))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用 格式化 方式将日志级别 debug，info，warn，error，fatal 的日志写到 logger
 */
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)


namespace sylar {

class Logger;
class LoggerManager;

/**
 * @brief  日志级别工具类
 */
class LogLevel{
public:
    // enum 枚举类型 用于定义一组具有命名常数的类型
    enum Level{
        UNKNOW = 0,  ///未知级别  通常用于初始化或错误状态
        DEBUG = 1,   ///调试级别  用于开发和调试程序时输出详细信息
        INFO = 2,    ///信息级别  用于记录正常运行时的重要信息
        WARN = 3,    ///警告级别  用于提示潜在问题
        ERROR = 4,   ///错误级别  表示程序运行中遇到了问题
        FATAL = 5    ///致命错误级别  系统级别的严重错误
    };

    /**
     * @brief 将日志级别转成文本输出
     * @param[in] level 日志级别
     * @example 输入 LogLevel::DEBUG 时返回 "DEBUG"
     */
    static const char* ToString(LogLevel::Level level);

    /**
     * @brief 将文本转换成日志级别
     * @param[in] str 日志级别文本
     * @example 输入 "DEBUG" 或 "debug" 时 返回 LogLevel::DEBUG
     */
     static LogLevel::Level FromString(const std::string& str);
};

/**
 * @brief 日志事件类
 * @attention 表示一条日志记录的信息，
 *            通常包含日志时间、线程信息、日志级别、内容、代码位置等内容
 */
class LogEvent{
public:
    // 用于管理日志事件对象的动态分配
    typedef std::shared_ptr<LogEvent> ptr;
    /**
     * @brief 构造函数
     *        初始化日志事件对象的所有属性
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] file 文件名
     * @param[in] line 文件行号
     * @param[in] elapse 程序启动依赖的耗时(毫秒)
     * @param[in] thread_id 线程id
     * @param[in] fiber_id 协程id
     * @param[in] time 日志事件(秒)
     * @param[in] thread_name 线程名称
     */
    LogEvent(std::shared_ptr<Logger> logger,
             LogLevel::Level level,
             const char* file,
             int32_t line,
             uint32_t elapse,
             uint32_t thread_id,
             uint32_t fiber_id,
             uint64_t time,
             const std::string& thread_name
             );

    /// 成员函数：返回日志信息
     /**
      * @brief 返回文件名
      */
    const char* getFile() const {return m_file;}

    /**
     * @brief 返回行号
     */
    int32_t getLine() const {return m_line;}

    /**
     * @brief  返回耗时
     */
    uint32_t getElapse() const {return m_elapse;}

    /**
     * @brief  返回线程ID
     */
    uint32_t getThreadId() const {return m_threadId;}

    /**
     * @brief  返回协程ID
     */
    uint32_t getFiberId() const {return m_fiberId;}

    /**
     * @brief  返回时间
     */
    uint64_t getTime() const {return m_time;}

    /**
     * @brief  返回日志内容
     */
    std::string getContent() const {return m_ss.str();}

    /**
     * @brief  返回线程名称
     */
    const std::string& getThreadName() const {return m_threadName;}

    /**
     * @brief 返回日志器
     */
    std::shared_ptr<Logger> getLogger() const {return m_logger;}

    /**
     * @brief 返回日志级别
     */
    LogLevel::Level getLevel() const {return m_level;}

    /**
     * @brief 返回日志内容字符串流
     */
    std::stringstream& getSS() {return m_ss;}

    /**
     * @brief 格式化写入日志内容
     * @details  使用可变参数
     *           支持类似 printf 的格式化方式
     */
    void format(const char* fmt, ...);

    /**
     * @brief 格式化写入日志内容
     * @details 接受 va_list 类型的参数
     *          用于日志内容的动态拼接
     */
    void format(const char* fmt, va_list al);

private:
    const char* m_file = nullptr;  //文件名
    int32_t m_line = 0;            //行号
    uint32_t m_elapse = 0;         //程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;       //线程ID
    uint32_t m_fiberId = 0;        //协程ID
    uint64_t m_time = 0;           //时间戳
    //std::string m_content;         //保存日志的具体内容
    std::string m_threadName;      //线程名称
    std::stringstream m_ss;        //日志内容流  用于动态构建日志内容
    std::shared_ptr<Logger> m_logger; //日志器
    LogLevel::Level m_level;       //日志级别
};

/**
 * @brief 包装日志事件 (LogEvent) 的操作并提供管理其生命周期的功能
 *     用于简化 LogEvent 对象的使用和管理
 */
class LogEventWrap{
public:
    /**
     * @brief 构造函数
     * @param e  日志事件
     * @detail 传入一个智能指针 使用成员初始化列表直接赋值 m_event
     */
    LogEventWrap(LogEvent::ptr e);

    /**
     * @brief 析构函数 析构时自动触发日志提交
     * @function 在 LogEventWrap 对象销毁时，将日志事件提交到关联的 Logger
     * @detail   1.通过 m_event->getLogger() 获取关联的日志器（Logger）。
     *           2.调用日志器的 log 方法，将当前日志事件 m_event 按照其级别 m_event->getLevel() 提交
     */
    ~LogEventWrap();

    /**
     * @brief 获取日志事件
     */
    LogEvent::ptr getEvent() const {return m_event;}

    /**
     * @brief 获取日志内容流
     */
    std::stringstream& getSS();

private:
    ///日志事件
    LogEvent::ptr m_event;
};


/**
 * @brief 日志格式器
 */
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式模板 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter( const std::string& pattern);

    // %t   %thread_id  %m%n
    /**
     * @brief 返回格式化日志文本
     * @param logger  日志器
     * @param level   日志级别
     * @param event   日志事件
     * @details 1.返回格式化后的字符串，便于进一步处理或存储
     *          2.直接将格式化结果写入输出流 ofs，提高效率
     */
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs,std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    /**
     * @brief 日志内容项格式化类
     *   每个 FormatItem 表示一个日志格式化标记的解析结果，例如 %m 或 %p
     *   继承 FormatItem，可以为不同的标记（如 %m 消息）自定义解析和格式化逻辑
     */
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        /**
         * @brief 析构函数
         */
        virtual ~FormatItem() {}
        /**
         * @brief 格式化日志到流
         * @param[in,out] os 日志输出流
         * @param logger
         * @param level
         * @param event
         * @details 继承该类后需要实现 format 方法，将对应的日志信息写入到流 os
         */
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    /**
     * @brief 初始化 解析日志模板 m_pattern
     */
    void init();

    /**
     * @brief 检查解析是否有错误
     */
     bool isError() const {return m_error;}

     /**
      * @brief 返回日志模板
      */
     const std::string getPattern() const {return m_pattern;}

private:
    ///日志格式模板
    std::string m_pattern;
    ///日志格式解析后格式列表
    std::vector<FormatItem::ptr> m_items;
    ///解析是否有错误标志
    bool m_error = false;

};


/**
 * @brief  日志输出目标: 控制台 / 文件
 */
class LogAppender{
    friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    //typedef Spinlock MutexType;

    /**
     * @brief 析构函数
     */
    virtual ~LogAppender() {}

    /**
     * @brief  写入日志  纯虚函数
     *         将日志事件写入到具体的输出目标
     * @param logger
     * @param level
     * @param event
     */
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) = 0;

    /**
     * @brief 将日志输出目标配置转换成 YAML String
     */
    //virtual std::string toYamlString() = 0;
    /**
     * @brief 更改日志格式器
     */
    void setFormatter(LogFormatter::ptr val) {m_formatter = val;}

    /**
     * @brief 获取日志格式器
     */
    LogFormatter::ptr getFormatter() const {return m_formatter;}

    /**
     * @brief 获取日志级别
     */
    LogLevel::Level getLevel() const {return m_level;}

    /**
     * @brief 设置日志级别
     */
    void setLevel(LogLevel::Level val) {m_level = val;}

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;   ///日志级别
    bool m_hasFormatter = false;     ///是否有自己的日志格式器
    //MutexType m_mutex;
    LogFormatter::ptr m_formatter;   ///日志格式器
};


/**
 * @brief  日志器
 */
class Logger : public std::enable_shared_from_this<Logger>{
    //friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    //typedef Spinlock MutexType;

    /**
     * @brief 构造函数
     * @param name 日志器名称
     */
    Logger(const std::string& name = "root");

    /**
     * @brief 写日志
     */
    void log(LogLevel::Level level, LogEvent::ptr event);

    /**
     * @brief 写 各 级日志
     * @param event
     */
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    /**
     * @brief 添加日志目标
     * @param appender  日志目标
     */
    void addAppender(LogAppender::ptr appender);
    /**
     * @brief 删除日志目标
     * @param appender
     */
    void delAppender(LogAppender::ptr appender);

    /**
     * @brief 清空日志目标
     */
    void clearAppenders();

    /**
     * @brief 返回日志级别
     */
    LogLevel::Level getLevel() const {return m_level;}

    /**
     * @brief 设置日志级别
     * @param val
     */
    void setLevel(LogLevel::Level val) {m_level = val;}

    /**
     * @brief 返回日志名称
     */
    const std::string& getName() const {return m_name;}

    /**
     * @brief 设置日志格式器
     * @param val
     */
    void setFormatter(LogFormatter::ptr val);

    /**
     * @brief 设置日志格式模板
     * @param val
     */
    void setFormatter(const std::string& val);

    /**
     * @brief 获取日志格式器
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief 将日志器的配置转换成 YAML String
     */
    //std::string toYamlString();

private:
    /// 日志名称
    std::string m_name;
    ///日志级别
    LogLevel::Level m_level;  // 满足日志级别的才能输出
    /// Appender 日志目标集合
    std::list<LogAppender::ptr> m_appenders;

    //MutexType m_mutex;
    ///日志格式器
    LogFormatter::ptr m_formatter;
    ///主日志器
    Logger::ptr m_root;
};


/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    // std::string toYamlString() override;
};

/**
 * @brief 输出到文件的Appender
 */
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    /**
     * @brief 重新打开文件， 文件打开成功返回true
     * @return 成功返回 true
     */
    bool reopen();
private:
    std::string m_filename;     /// 文件路径
    std::ofstream m_filestream; /// 文件流
    uint64_t m_lastTime = 0;    ///上次重新打开的时间
};


/**
 * @brief 日志器管理类
 *        负责管理所有的 logger
 */
class LoggerManager{
public:
    /**
     * @brief 构造函数
     * @details 初始化主日志器 m_root
     *          将主日志器添加到日志器容器中
     *          调用 init 函数 加载日志配置
     */
    LoggerManager();

    /**
     * @brief 获取日志器
     *          如果日志器不存在 创建新的日志器并存储到 m_loggers 容器中
     * @param[in] name 日志器名称
     */
    Logger::ptr getLogger(const std::string& name);

    /**
     * @brief 初始化日志器管理器
     */
    void init();

    /**
     * @brief 返回主日志器
     *         主日志器通常作为默认日志器处理全局日志输出
     */
    Logger::ptr getRoot() const {return m_root;}

private:
    //MutexType m_mutex;
    ///日志器容器  以名称为键
    std::map<std::string, Logger::ptr> m_loggers;
    ///主日志器
    Logger::ptr m_root;
};

///日志管理类单例模式
typedef sylar::Singleton<LoggerManager> LoggerMgr;

}
#endif //SYLAR_LOG_H
