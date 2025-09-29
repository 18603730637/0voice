#include "coroutine.h"
#include <iostream>
#include <thread>
#include <chrono>

// 测试协程1
void test_coroutine1() {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Coroutine 1 - iteration " << i << std::endl;
        co_yield();  // 让出CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Coroutine 1 finished" << std::endl;
}

// 测试协程2
void test_coroutine2() {
    for (int i = 0; i < 2; ++i) {
        std::cout << "Coroutine 2 - iteration " << i << std::endl;
        co_yield();  // 让出CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    std::cout << "Coroutine 2 finished" << std::endl;
}

// 测试协程3
void test_coroutine3() {
    std::cout << "Coroutine 3 started" << std::endl;
    co_yield();  // 让出CPU
    std::cout << "Coroutine 3 continuing" << std::endl;
    co_yield();  // 让出CPU
    std::cout << "Coroutine 3 finished" << std::endl;
}

int main() {
    std::cout << "Main start" << std::endl;

    // 创建三个测试协程
    auto co1 = co_create(test_coroutine1);
    auto co2 = co_create(test_coroutine2);
    auto co3 = co_create(test_coroutine3);

    // 运行协程调度
    co_run();

    std::cout << "Main finished" << std::endl;
    return 0;
}
    