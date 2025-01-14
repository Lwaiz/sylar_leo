#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"


int main(int argc,char** argv){

    //创建日志器实例
    sylar::Logger::ptr logger(new sylar::Logger);

    //创建标准输出日志目标 并 添加到日志器中
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    logger->addAppender(file_appender);

    //配置文件流日志格式器
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d{%Y-%m-%d %H:%M:%S} [%p] (%f:%l) %m %n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);
    file_appender->setLevel(sylar::LogLevel::INFO);

    //创建日志事件
    sylar::LogEvent::ptr event(new sylar::LogEvent(logger,
                                                   sylar::LogLevel::DEBUG,
                                                   __FILE__,
                                                   __LINE__,
                                                   0,
                                                   sylar::GetThreadId(),
                                                   sylar::GetFiberId(),
                                                   time(0),
                                                   "main_thread")
                                                   );
    event->getSS() << "hello sylar log";

    logger->log(sylar::LogLevel::DEBUG, event);
    std::cout << "hello sylar log" << std::endl;

    SYLAR_LOG_INFO(logger) << " test macro" ;
    SYLAR_LOG_ERROR(logger) << " test macro error";
    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}