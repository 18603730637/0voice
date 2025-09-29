#include "coroutine.h"
#include <iostream>
#include <thread>
#include <chrono>

// 全局调度器实例
Scheduler scheduler;

// 测试协程1
void test_coroutine1() {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Coroutine 1: " << i << std::endl;
        yield();  // 让出CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 测试协程2
void test_coroutine2() {
    for (int i = 0; i < 2; ++i) {
        std::cout << "Coroutine 2: " << i << std::endl;
        yield();  // 让出CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// 测试协程3
void test_coroutine3() {
    std::cout << "Coroutine 3: Starting" << std::endl;
    yield();  // 让出CPU
    std::cout << "Coroutine 3: Continuing" << std::endl;
    yield();  // 让出CPU
    std::cout << "Coroutine 3: Finishing" << std::endl;
}

int main() {
    std::cout << "Main: Starting scheduler" << std::endl;
    
    // 创建三个协程
    scheduler.create_coroutine(test_coroutine1);
    scheduler.create_coroutine(test_coroutine2);
    scheduler.create_coroutine(test_coroutine3);
    
    // 启动调度器
    scheduler.run();
    
    std::cout << "Main: All coroutines finished" << std::endl;
    return 0;
}
