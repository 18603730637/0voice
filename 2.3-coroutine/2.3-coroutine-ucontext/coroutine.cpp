#include "coroutine.h"
#include <cstdlib>
#include <iostream>

// 协程入口函数，用于启动协程
static void coroutine_entry(void* arg) {
    Coroutine* co = static_cast<Coroutine*>(arg);
    if (co) {
        co->run();  // 执行协程函数
    }
}

// 协程构造函数
Coroutine::Coroutine(std::function<void()> func, size_t stack_size)
    : func_(std::move(func)), status_(CoroutineStatus::READY),
      stack_size_(stack_size), scheduler_(nullptr) {
    // 初始化上下文
    if (getcontext(&ctx_) == -1) {
        std::cerr << "Failed to initialize context" << std::endl;
        std::abort();
    }
    
    // 分配栈空间
    stack_ = new char[stack_size_];
    ctx_.uc_stack.ss_sp = stack_;
    ctx_.uc_stack.ss_size = stack_size_;
    ctx_.uc_stack.ss_flags = 0;
    ctx_.uc_link = nullptr;  // 协程结束后将返回主上下文
    
    // 设置协程入口函数
    makecontext(&ctx_, reinterpret_cast<void (*)()>(coroutine_entry), 1, this);
}

// 协程析构函数
Coroutine::~Coroutine() {
    delete[] stack_;
}

// 协程移动构造函数
Coroutine::Coroutine(Coroutine&& other) noexcept
    : ctx_(other.ctx_), status_(other.status_), func_(std::move(other.func_)),
      stack_(other.stack_), stack_size_(other.stack_size_), scheduler_(other.scheduler_) {
    other.stack_ = nullptr;
    other.scheduler_ = nullptr;
}

// 协程移动赋值运算符
Coroutine& Coroutine::operator=(Coroutine&& other) noexcept {
    if (this != &other) {
        delete[] stack_;
        
        ctx_ = other.ctx_;
        status_ = other.status_;
        func_ = std::move(other.func_);
        stack_ = other.stack_;
        stack_size_ = other.stack_size_;
        scheduler_ = other.scheduler_;
        
        other.stack_ = nullptr;
        other.scheduler_ = nullptr;
    }
    return *this;
}

// 执行协程函数
void Coroutine::run() {
    func_();  // 执行用户提供的函数
    status_ = CoroutineStatus::FINISHED;  // 标记为已完成
    scheduler_->yield();  // 完成后主动让出CPU
}

// 调度器构造函数
Scheduler::Scheduler() : running_coroutine_(nullptr), current_id_(-1) {}

// 创建新协程
int Scheduler::create_coroutine(std::function<void()> func, size_t stack_size) {
    auto coroutine = std::make_unique<Coroutine>(std::move(func), stack_size);
    coroutine->set_scheduler(this);
    int id = coroutines_.size();
    coroutines_.push_back(std::move(coroutine));
    return id;
}

// 启动调度器
void Scheduler::run() {
    while (true) {
        bool has_ready = false;
        
        // 查找下一个就绪的协程
        for (size_t i = 0; i < coroutines_.size(); ++i) {
            auto& co = coroutines_[i];
            if (co->get_status() == CoroutineStatus::READY || 
                co->get_status() == CoroutineStatus::SUSPENDED) {
                
                has_ready = true;
                current_id_ = i;
                running_coroutine_ = co.get();
                running_coroutine_->set_status(CoroutineStatus::RUNNING);
                
                // 切换到该协程
                if (swapcontext(&main_context_, co->get_context()) == -1) {
                    std::cerr << "Failed to swap context" << std::endl;
                    std::abort();
                }
                
                break;
            }
        }
        
        // 如果没有就绪的协程了，退出调度
        if (!has_ready) {
            break;
        }
    }
}

// 切换到下一个协程
void Scheduler::yield() {
    if (!running_coroutine_) return;
    
    // 如果当前协程未完成，则标记为挂起
    if (running_coroutine_->get_status() == CoroutineStatus::RUNNING) {
        running_coroutine_->set_status(CoroutineStatus::SUSPENDED);
    }
    
    // 保存当前协程状态，切换回主上下文
    if (swapcontext(running_coroutine_->get_context(), &main_context_) == -1) {
        std::cerr << "Failed to yield" << std::endl;
        std::abort();
    }
}

// 全局yield函数，供协程内部调用
void yield() {
    // 获取当前线程的调度器和运行中的协程
    // 注意：在实际应用中，这里应该使用线程局部存储来获取当前调度器
    // 为简化实现，这里假设只有一个调度器实例
    // 生产环境中应该改进这部分实现
    extern Scheduler scheduler;
    scheduler.yield();
}
