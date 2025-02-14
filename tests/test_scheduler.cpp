/**
  ******************************************************************************
  * @file           : test_scheduler.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/2/13
  ******************************************************************************
  */


# include "../sylar/sylar.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_scheduler(){
    static int s_count = 5;
    SYLAR_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(--s_count >= 0) {
        sylar::Scheduler::GetThis()->schedule(&test_scheduler,sylar::GetThreadId());
    }
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "main";
    // 创建调度器
    sylar::Scheduler sc(2, false, "test");
    // 启动调度器 初始化调度线程池
    sc.start();
    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_scheduler);
    // 停止调度器
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}
