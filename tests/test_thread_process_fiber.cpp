/**
  ******************************************************************************
  * @file           : test_thread_process_fiber.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/27
  ******************************************************************************
  */


/**
  ******************************************************************************
  * @file           : test.cpp
  * @author         : 18483
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/26
  ******************************************************************************
  */


#include <iostream>
#include <vector>
#include <thread>
#include <unistd.h>      // sleep, fork
#include <sys/wait.h>    // waitpid
#include <chrono>        // timing
#include <ucontext.h>    // 协程

using namespace std;
using namespace chrono;

// 模拟 I/O 任务（网络请求）
void simulate_network_request() {
    sleep(1);  // 模拟 I/O 阻塞
}

// 进程测试
void test_multiprocessing_io(int n) {
    auto start = high_resolution_clock::now();
    vector<pid_t> pids;

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {  // 子进程
            simulate_network_request();
            exit(0);
        } else {
            pids.push_back(pid);
        }
    }

    // 等待所有子进程结束
    for (pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }

    auto end = high_resolution_clock::now();
    cout << "多进程（I/O 密集型）耗时: "
         << duration<double>(end - start).count() << " 秒" << endl;
}

// 线程测试
void test_multithreading_io(int n) {
    auto start = high_resolution_clock::now();
    vector<thread> threads;

    for (int i = 0; i < n; i++) {
        threads.emplace_back(simulate_network_request);
    }

    for (auto &t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    cout << "多线程（I/O 密集型）耗时: "
         << duration<double>(end - start).count() << " 秒" << endl;
}

// 协程测试
ucontext_t main_context, coroutine_context;
bool coroutine_finished = false;

void coroutine_function() {
    simulate_network_request();
    coroutine_finished = true;
    setcontext(&main_context);
}

void test_ucontext_io(int n) {
    auto start = high_resolution_clock::now();

    getcontext(&coroutine_context);
    coroutine_context.uc_stack.ss_sp = malloc(8192);
    coroutine_context.uc_stack.ss_size = 8192;
    coroutine_context.uc_link = &main_context;
    makecontext(&coroutine_context, coroutine_function, 0);

    for (int i = 0; i < n; i++) {
        coroutine_finished = false;
        swapcontext(&main_context, &coroutine_context);
    }

    auto end = high_resolution_clock::now();
    cout << "多协程（I/O 密集型）耗时: "
         << duration<double>(end - start).count() << " 秒" << endl;
}

int main() {
    vector<int> test_sizes = {10, 100, 1000};

    for (int n : test_sizes) {
        cout << "I/O 密集型任务测试： n = " << n << endl;
        test_multiprocessing_io(n);
        test_multithreading_io(n);
        test_ucontext_io(n);
    }

    return 0;
}
