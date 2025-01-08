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
 * @brief 日志事件
 */
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char* file, int32_t line, uint32_t elapse,
             uint32_t thread_id, uint32_t fiber_id, uint64_t time,
             const std::string& thread_name);

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
    const std::string getContent() const {return m_content;}

    /**
     * @brief  返回线程名称
     */
    const std::string& getThreadName() const {return m_threadName;}

private:
    const char* m_file = nullptr;  //文件名
    int32_t m_line = 0;            //行号
    uint32_t m_elapse = 0;         //程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;       //线程ID
    uint32_t m_fiberId = 0;        //协程ID
    uint64_t m_time = 0;           //时间戳
    std::string m_content;
};


// 日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter( const std::string& pattern);

    // %t   %thread_id  %m%n
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    /**
     * @brief 初始化 解析日志模板
     */
    void init();
private:
    ///日志格式模板
    std::string m_pattern;
    ///日志格式解析后格式
    std::vector<FormatItem::ptr> m_items;
    ///是否有错误标志
    bool m_error = false;
};

/**
 * @brief  日志输出目标: 控制台 / 文件
 */
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) = 0;
    void setFormatter(LogFormatter::ptr val) {m_formatter = val;}
    LogFormatter::ptr getFormatter() const {return m_formatter;}
protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};


//日志器
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, const LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const {return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val;}

    const std::string& getName() const {return m_name;}

private:
    std::string m_name;                         // 日志名称
    LogLevel::Level m_level;                    // 满足日志级别的才能输出
    std::list<LogAppender::ptr> m_appenders;    //Appender集合
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
private:

};

//输出到文件的Appender
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    //重新打开文件， 文件打开成功返回true
    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;

};


}


#endif //SYLAR_LOG_H
