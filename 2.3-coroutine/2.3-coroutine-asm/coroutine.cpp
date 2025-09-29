#include "coroutine.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <mutex>
#include <string.h>
#include <unordered_map>

// 全局线程局部存储的调度器
thread_local Scheduler* tls_scheduler = nullptr;

// 汇编实现的上下文切换函数声明
extern "C" void co_swap(CoContext* from, CoContext* to);

// 协程入口函数包装器
extern "C" void coroutine_entry(void* arg) {
    Coroutine* co = static_cast<Coroutine*>(arg);
    co->func();               // 执行协程函数
    co->status = CoStatus::FINISHED;  // 标记为完成
    Scheduler::get_current()->yield(); // 让出CPU，不会再被调度
}

// 协程构造函数
Coroutine::Coroutine(Func f, size_t stack_size) 
    : func(std::move(f)), stack_size(stack_size), status(CoStatus::READY) {
    // 分配栈空间，栈在x86_64上是向下增长的
    stack = malloc(stack_size);
    if (!stack) {
        throw std::bad_alloc();
    }
    
    // 初始化上下文
    memset(&ctx, 0, sizeof(CoContext));
    
    // 设置栈指针，预留一些空间防止溢出
    uint64_t stack_top = reinterpret_cast<uint64_t>(stack) + stack_size;
    stack_top -= 16;  // 对齐16字节
    ctx.rsp = stack_top;
    
    // 设置入口函数
    ctx.rip = reinterpret_cast<uint64_t>(coroutine_entry);
    
    // 将协程指针作为参数传递给入口函数
    *reinterpret_cast<Coroutine**>(stack_top) = this;
}

// 协程析构函数
Coroutine::~Coroutine() {
    if (stack) {
        free(stack);
        stack = nullptr;
    }
}

// 调度器构造函数
Scheduler::Scheduler() : running(nullptr), next_id(1) {}

// 调度器析构函数
Scheduler::~Scheduler() {
    while (!ready_queue.empty()) {
        delete ready_queue.front();
        ready_queue.pop();
    }
    if (running) {
        delete running;
        running = nullptr;
    }
}

// 创建协程
Coroutine* Scheduler::create_coroutine(Coroutine::Func func) {
    Coroutine* co = new Coroutine(std::move(func));
    co->id = next_id++;
    ready_queue.push(co);
    return co;
}

// 切换到下一个协程
void Scheduler::yield() {
    if (ready_queue.empty()) {
        return; // 没有就绪协程，直接返回
    }
    
    // 如果当前有运行的协程且未完成，则将其加入就绪队列尾部
    if (running && running->status != CoStatus::FINISHED) {
        running->status = CoStatus::READY;
        ready_queue.push(running);
    }
    
    // 从就绪队列获取下一个协程
    Coroutine* next = ready_queue.front();
    ready_queue.pop();
    
    // 切换协程
    Coroutine* prev = running;
    running = next;
    running->status = CoStatus::RUNNING;
    
    // 执行上下文切换
    if (prev) {
        co_swap(&prev->ctx, &running->ctx);
    } else {
        // 首次运行，直接跳转到新协程
        co_swap(nullptr, &running->ctx);
    }
}

// 运行调度器
void Scheduler::run() {
    while (!ready_queue.empty()) {
        yield();
    }
}

// 获取当前调度器
Scheduler* Scheduler::get_current() {
    if (!tls_scheduler) {
        tls_scheduler = new Scheduler();
    }
    return tls_scheduler;
}

// 协程创建函数
Coroutine* co_create(Coroutine::Func func) {
    return Scheduler::get_current()->create_coroutine(std::move(func));
}

// 协程让出CPU
void co_yield() {
    Scheduler::get_current()->yield();
}

// 运行协程调度器
void co_run() {
    Scheduler::get_current()->run();
}
    