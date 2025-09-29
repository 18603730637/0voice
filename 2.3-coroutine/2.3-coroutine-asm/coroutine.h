#ifndef COROUTINE_H
#define COROUTINE_H

#include <cstdint>
#include <functional>
#include <queue>

// 协程状态
enum class CoStatus {
    READY,    // 就绪
    RUNNING,  // 运行中
    SUSPENDED,// 挂起
    FINISHED  // 完成
};

// 协程上下文，保存寄存器状态
struct CoContext {
    uint64_t rbx;  // 通用寄存器
    uint64_t rbp;  // 栈基址寄存器
    uint64_t r12;  // 通用寄存器
    uint64_t r13;  // 通用寄存器
    uint64_t r14;  // 通用寄存器
    uint64_t r15;  // 通用寄存器
    uint64_t rsp;  // 栈指针寄存器
    uint64_t rip;  // 指令指针寄存器
};

// 协程结构体
struct Coroutine {
    using Func = std::function<void()>;
    
    uint64_t id;          // 协程ID
    CoStatus status;      // 协程状态
    CoContext ctx;        // 协程上下文
    void* stack;          // 协程栈
    size_t stack_size;    // 栈大小
    Func func;            // 协程执行函数
    
    Coroutine(Func f, size_t stack_size = 1024 * 1024);
    ~Coroutine();
};

// 协程调度器
class Scheduler {
private:
    std::queue<Coroutine*> ready_queue;  // 就绪队列
    Coroutine* running;                  // 当前运行的协程
    uint64_t next_id;                    // 下一个协程ID
    
public:
    Scheduler();
    ~Scheduler();
    
    // 创建协程
    Coroutine* create_coroutine(Coroutine::Func func);
    
    // 切换到下一个协程
    void yield();
    
    // 运行调度器
    void run();
    
    // 获取当前调度器
    static Scheduler* get_current();
};

// 协程创建函数
Coroutine* co_create(Coroutine::Func func);

// 协程让出CPU
void co_yield();

// 运行协程调度器
void co_run();

#endif // COROUTINE_H
    