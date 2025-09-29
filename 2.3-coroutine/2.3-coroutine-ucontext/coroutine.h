#ifndef COROUTINE_H
#define COROUTINE_H

#include <ucontext.h>
#include <functional>
#include <memory>
#include <vector>

// 协程状态枚举
enum class CoroutineStatus {
    READY,    // 就绪状态，等待调度
    RUNNING,  // 运行中
    SUSPENDED,// 已挂起
    FINISHED  // 已完成
};

// 前置声明
class Scheduler;

// 协程类
class Coroutine {
private:
    ucontext_t ctx_;               // 协程上下文
    CoroutineStatus status_;       // 协程状态
    std::function<void()> func_;   // 协程要执行的函数
    char* stack_;                  // 协程栈空间
    size_t stack_size_;            // 栈大小
    Scheduler* scheduler_;         // 所属调度器

public:
    // 构造函数
    Coroutine(std::function<void()> func, size_t stack_size = 1024 * 128);
    
    // 析构函数
    ~Coroutine();
    
    // 禁止拷贝构造和赋值
    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;
    
    // 移动构造和赋值
    Coroutine(Coroutine&&) noexcept;
    Coroutine& operator=(Coroutine&&) noexcept;

    // 获取上下文
    ucontext_t* get_context() { return &ctx_; }
    
    // 获取状态
    CoroutineStatus get_status() const { return status_; }
    
    // 设置状态
    void set_status(CoroutineStatus status) { status_ = status; }
    
    // 设置调度器
    void set_scheduler(Scheduler* scheduler) { scheduler_ = scheduler; }
    
    // 获取调度器
    Scheduler* get_scheduler() { return scheduler_; }
    
    // 获取栈
    char* get_stack() { return stack_; }
    
    // 获取栈大小
    size_t get_stack_size() { return stack_size_; }
    
    // 执行协程函数
    void run();
};

// 调度器类
class Scheduler {
private:
    std::vector<std::unique_ptr<Coroutine>> coroutines_;  // 所有协程
    Coroutine* running_coroutine_;                        // 当前运行的协程
    ucontext_t main_context_;                             // 主上下文
    int current_id_;                                      // 当前协程ID

public:
    Scheduler();
    ~Scheduler() = default;
    
    // 禁止拷贝构造和赋值
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    
    // 创建新协程
    int create_coroutine(std::function<void()> func, size_t stack_size = 1024 * 128);
    
    // 启动调度器
    void run();
    
    // 切换到下一个协程
    void yield();
    
    // 获取当前运行的协程
    Coroutine* get_running_coroutine() { return running_coroutine_; }
    
    // 获取主上下文
    ucontext_t* get_main_context() { return &main_context_; }
};

// 全局函数：协程让出CPU
void yield();

#endif // COROUTINE_H
