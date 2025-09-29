#include <functional>
#include <iostream>
#include <memory>
#include <ucontext.h>
#include <vector>

enum class CoroutineStatus {
    READY,
    RUNING,
    SUSPEND,
    FINISH,
};

class Schedule;

class Coroutine {
private:
    ucontext_t m_ctx;
    Schedule *m_schedulePtr;
    std::function<void()> m_func;
    int m_stackSize;
    char *m_stack;
    CoroutineStatus m_status;

public:
    Coroutine(std::function<void()> func, int stackSize = 1024 * 128);
    ~Coroutine();
    Coroutine(const Coroutine& other) = delete;
    Coroutine& operator=(const Coroutine& other) = delete;
    Coroutine(Coroutine&& other);
    Coroutine& operator=(Coroutine&& other);

    Schedule* GetShcedule() { return m_schedulePtr; }
    void run();
    CoroutineStatus GetCoroutineStatus() { return m_status; }
    ucontext_t& GetContext() { return m_ctx; }
    void SetCoroutineStatus(CoroutineStatus status) {m_status = status;}
    void SetSchedule(Schedule* schedule) {m_schedulePtr = schedule;}
};

class Schedule {
  private:
    std::vector<std::unique_ptr<Coroutine>> m_coroutines;
    Coroutine *m_curcoroutine;
    ucontext_t m_mainctx;

  public:
    Schedule();
    ~Schedule();
    void CreateCoroutine(std::function<void()> func);
    void yield();
    void run();
};