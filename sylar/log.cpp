#include "log.h"

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


Logger(const std::string& name )
    : m_name(name){

}

void addAppender(LogApperender::ptr appender){
    m_appenders.push_back(appender);
}
void delAppender(LogApperender::ptr appender){
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}
void Logger::log(LogLevel::Level level, const LogEvent::ptr event){
    if(level >= m_level){
        for(auto& i : m_appenders){
            i->log(level, event);
        }
    }
}
void Logger::debug(LogEvent::ptr event){
    debug(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event){
    debug(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event){
    debug(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event){
    debug(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event){
    debug(LogLevel::FATAL, event);
    }

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename) {

}

void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        m_filestream << m_formatter.format(event);
    }
}

bool FileLogAppender::reopen(){
    if(m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;   // !!表示 非0值转为1 , 0值还是0
}

void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        std::cout << m_formatter.format(event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern) {

}

std::string::LogFormatter::format(LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items){
        i->format(ss, event);
    }
    return ss.str();
}


//解析日志格式化字符串模式（m_pattern）并将解析结果存储到一个向量vec中

/* 举例："%d{%Y-%m-%d %H:%M:%S} [%p] %c: %m%n"
 *  %d 表示日期时间，%p 表示日志级别，%c 表示日志名称，%m 表示消息内容，%n 表示换行符
 *    遇到 %d：检测到 {%Y-%m-%d %H:%M:%S} 为参数，解析为 ("d", "{%Y-%m-%d %H:%M:%S}", 1)。
 *    遇到 [: 普通字符，解析为 ("[", "", 0)。
 *    遇到 %p：无参数，解析为 ("p", "", 1)。
 *    遇到 %c：无参数，解析为 ("c", "", 1)。
 *    遇到 %m：无参数，解析为 ("m", "", 1)。
 *    遇到 %n：无参数，解析为 ("n", "", 1)。
 */
LogFormatter::init(){
    // 1.定义容器与变量
    //每个元素是一个三元组：(字符串, 格式化参数, 类型) // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;  //vec储存解析结果
    std::string nstr;  //临时存储普通字符串

    // 2.遍历格式化模式 m_pattern
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        // 如果字符不是 % ，将其追加到 nstr 中
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        //处理 '%%' 即转义字符
        if ((i + 1) < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');  //将 '%' 添加到普通字符串
                ++i;  //跳过下一个 '%'
                continue;
            }
        }

        // 3.格式化解析字符串部分
        //当检测到字符是 % ，开始尝试解析格式化内容
        size_t n = i + 1;       //当前解析位置
        int fmt_status = 0;     //解析状态机 0 初始状态， 1 正在解析键名， 2 完成解析
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

        //根据解析结果更新 vec
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

    //如果有剩余的普通字符串 存储到 vec 中
    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    //定义格式化项对应的创建函数映射表
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
    {#str, [](const std::string& fmt){ return FormatItem::ptr(new C(fmt));}}

            //定义各种格式化标记及其对应的处理类
            

    };



}



} // namespace 作用域结束









