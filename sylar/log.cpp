
#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <time.h>
#include <string.h>

namespace sylar{

/**
 * @param level
 * @return
 */
const char* LogLevel::ToString(LogLevel::Level level){
    switch(level){
#define XX(name) \
        case LogLevel::name : \
            return #name; \
            break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) \
    if(str == #v) {  \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

/// LogEvent 的格式化日志输出功能
/// 接受可变参数的format函数
void LogEvent::format(const char* fmt, ...){
    va_list al;               // 定义可变参数列表
    va_start(al, fmt);        // 初始化可变参数列表
    format(fmt, al);          // 调用重载方法处理格式化
    va_end(al);               // 结束可变参数列表
}

///实际处理字符串格式化的函数
void LogEvent::format(const char* fmt, va_list al){
    char* buf = nullptr;  //定义字符指针 用于存储格式化后的字符串
    // 使用 vasprintf 动态分配缓冲区并格式化字符串
    int len = vasprintf(&buf, fmt, al);

    //检查格式化是否成功
    if(len != -1) {
        //将格式化后的字符串追加到字符串流中
        m_ss << std::string(buf, len);
        //释放动态分配缓冲区
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e){
}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}


class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

/**
 * @brief 将日志事件的时间戳格式化为指定的时间字符串，并输出到日志流
 */
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    ///m_format 存储时间格式字符串
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
            :m_format(format) {
        ///如果传入的 format 为空，则设置默认值
        if(m_format.empty()){
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    /**
     * @brief 输出格式化时间字符串
     */
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        ///创建 tm 结构体 用于存储本地时间
        struct tm tm;
        /// 从 LogEvent 对象中获取日志事件时间戳
        time_t time = event->getTime();
        ///将 time_t 转换为本地时间 tm 结构
        localtime_r(&time, &tm);
        ///用于存储格式化后的时间字符串
        char buf[64];
        ///strftime标准库函数 将 tm 结构按指定的 m_format 格式化为字符串 结果保存在 buf 中
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = ""){}
    void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream & os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override{
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

/// 初始化：成员初始化列表
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char* file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                   const std::string& thread_name)
    :m_file(file),
     m_line(line),
     m_elapse(elapse),
     m_threadId(thread_id),
     m_fiberId(fiber_id),
     m_time(time),
     m_threadName(thread_name),
     m_logger(logger),
     m_level(level){

}


Logger::Logger(const std::string& name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG){
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    for(auto& i : m_appenders){
        if(!i->m_hasFormatter){
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val){
    std::cout << "---" << val << std::endl;
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()){
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_val);
}

void Logger::addAppender(LogAppender::ptr appender){
    ///检查 appender 的格式化器
    if(!appender->getFormatter()){
        appender->m_formatter = m_formatter;
    }
    ///将 appender 添加到 m_appenders 容器中
    m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level,  LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();

        if(!m_appenders.empty()){
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root){
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
    }

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen(){
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;   // !!表示 非0值转为1 , 0值还是0
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        //std::cout << "hello";
        m_formatter->format(std::cout, logger, level, event);
    }
}


/// 格式类 实现

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern) {
    init();
}


std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    std::stringstream ss;
    ///依次遍历 m_items（模板中各个解析出来的格式化项），调用它们的
    /// format 方法，将格式化内容写入到字符串流 ss 中
    for(auto& i : m_items){
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    ///直接将格式化后的日志写入到输出流 ofs 中
    /// 目标流可以是 标准输出流、文件流
    for(auto& i : m_items){
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

/**
 * @brief  init()初始化函数
 *     解析日志格式化字符串模式（m_pattern）并将解析结果存储到一个vec中
 * @example 举例："%d{%Y-%m-%d %H:%M:%S} [%p] %c: %m%n"
 *      %d 表示日期时间，%p 表示日志级别，%c 表示日志名称，%m 表示消息内容，%n 表示换行符
 *      遇到 %d：检测到 {%Y-%m-%d %H:%M:%S} 为参数，解析为 ("d", "{%Y-%m-%d %H:%M:%S}", 1)。
 *      遇到 [: 普通字符，解析为 ("[", "", 0)。
 *      遇到 %p：无参数，解析为 ("p", "", 1)。
 *      遇到 %c：无参数，解析为 ("c", "", 1)。
 *      遇到 %m：无参数，解析为 ("m", "", 1)。
 *      遇到 %n：无参数，解析为 ("n", "", 1)。
 */
void LogFormatter::init() {
    /** 1.初始化 定义容器与变量 */
    //每个元素是一个三元组：(字符串, 格式化参数, 类型) // (str, format, type)
    std::vector<std::tuple<std::string, std::string, int>> vec;  //vec储存解析结果
    std::string nstr;  //临时存储普通字符串

    /** 2.遍历格式化模式 m_pattern */
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        // 处理普通字符  如果字符不是 % ，将其追加到 nstr 中
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        //处理转义字符  '%%'
        if ((i + 1) < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');  //将 '%' 添加到普通字符串
                ++i;  //跳过下一个 '%'
                continue;
            }
        }

        /** 3.格式化解析字符串部分 */
        //当检测到字符是 % ，开始尝试解析格式化内容
        size_t n = i + 1;       //当前解析位置
        // 解析状态机 用于区分键名解析和格式内容解析
        int fmt_status = 0;     // 0 初始状态， 1 正在解析键名， 2 完成解析
        size_t fmt_begin = 0;   //记录格式开始位置

        std::string str;   //存储格式化键名
        std::string fmt;   //存储格式化内容

        //解析格式化字符串
        while (n < m_pattern.size()) {
            // 非字母、非 '{' 或 '}' 时，表示格式化键名解析结束
            if (!fmt_status && (!isalpha(m_pattern[n])
                                && m_pattern[n] != '{' && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1); //提取键名
                break;
            }

            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    //遇到 { 表示进入格式部分
                    str = m_pattern.substr(i + 1, n - i - 1); //提取键名
                    fmt_status = 1;  //切换到状态1 格式解析状态
                    fmt_begin = n;   //记录格式部分起始位置
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    //遇到 } 表示格式部分解析结束
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1); // 提取格式内容
                    fmt_status = 0;  //状态重置为 0
                    ++n;
                    break;
                }
            }
            ++n;
            //如果到达字符串末尾 并且未解析出键名 则直接提取剩余部分作为键名
            if (n == m_pattern.size() && str.empty()) {
                str = m_pattern.substr(i + 1);
            }
        }

        /** 4.根据解析结果更新 vec */
        if (fmt_status == 0) {
            if (!nstr.empty()) {
                //如果有普通字符串未处理， 则先存储
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            //存储格式化字符串及其格式
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1; //更新索引位置 跳过已解析内容
        } else if (fmt_status == 1) {
            //如果格式部分解析出错， 记录错误并输出日志
            std::cout << "pattern parse error : " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true; //标记解析错误
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    /** 5.处理剩余普通字符串 */
    //如果有剩余的普通字符串 存储到 vec 中
    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    /** 6.格式化项创建函数映射 */
    //定义格式化项对应的创建函数映射表
    //s_format_items 是一个字典，存储了各个格式化标记及其对应的处理类构造函数
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
 /**
  * @brief  宏 XX 用于定义一种映射关系，格式化标记和对应的处理类之间的映射。
  * @param[in]  str:格式化标记， C:处理类
  * @return 使用Lambda表达式创建一个FormatItem对象
  */
#define XX(str, C) \
        {#str, [](const std::string& fmt){ return FormatItem::ptr(new C(fmt));}}

        //定义各种格式化标记及其对应的处理类  "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
        XX(m, MessageFormatItem),   // %m -- 消息体
        XX(p, LevelFormatItem),     // %p -- level
        XX(r, ElapseFormatItem),    // %r -- 启动后的时间
        XX(c, NameFormatItem),      // %c -- 日志名称
        XX(t, ThreadIdFormatItem),  // %t -- 线程id
        XX(n, NewLineFormatItem),   // %n -- 换行回车
        XX(d, DateTimeFormatItem),  // %d -- 时间
        XX(f, FileNameFormatItem),  // %f -- 文件名
        XX(l, LineFormatItem),      // %l -- 行号
        XX(T, TabFormatItem),       // T  -- Tab
        XX(F, FiberIdFormatItem),   // F  -- 协程ID
        XX(N, ThreadNameFormatItem),// N -- 线程名称
#undef XX
    };
    /** 7.遍历 vec 生成格式化项 */
    //遍历 vec 容器，里面包含多个tuple (str, format, type)
    //get<0>(i) 获取第i个tuple中的第一个元素 -- 键名  %m、%p等
    //get<1>(i) 获取第i个tuple中的第二个元素 -- 格式化参数  如 %d，表示日期
    //get<2>(i) 获取第i个tuple中的第三个元素 -- 类型 0：普通字符串，1：格式化项
    for(auto& i : vec){
        //普通字符串 创建 StringFormatItem 并加入 m_items
        if(std::get<2>(i) == 0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        //格式化项 查找对应处理类 并创建 加入 m_items
        //找不到对应处理类 标记错误
        else{
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()){
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            }
            else{
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}


LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    //m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string &name) {
    auto it = m_loggers.find(name);
    return it == m_loggers.end() ? m_root : it->second;
//    if(it != m_loggers.end()){
//        return it->second;
//    }

//    Logger::ptr logger(new Logger(name));
//    logger->m_root = m_root;
//    m_loggers[name] = logger;
//    return logger;
}

void LoggerManager::init(){

}









} // namespace 作用域结束



