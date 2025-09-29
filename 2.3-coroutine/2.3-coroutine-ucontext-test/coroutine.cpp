#include "coroutine.h"

static void Coroutine_entry(void *arg) {
    Coroutine *co = static_cast<Coroutine *>(arg);
    co->run();
}

Coroutine::Coroutine(std::function<void()> func, int stackSize) {
    m_func = std::move(func);
    m_stackSize = stackSize;
    m_stack = new char[stackSize];
    m_status = CoroutineStatus::READY;
    m_schedulePtr = nullptr;

    getcontext(&m_ctx);
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_flags = 0;
    m_ctx.uc_stack.ss_size = m_stackSize;
    m_ctx.uc_stack.ss_sp = m_stack;

    makecontext(&m_ctx, reinterpret_cast<void (*)()>(Coroutine_entry), 1, this);
}

Coroutine::~Coroutine() { delete[] m_stack; }

Coroutine::Coroutine(Coroutine &&other) {

    m_ctx = other.m_ctx;
    m_schedulePtr = other.m_schedulePtr;
    m_func = std::move(other.m_func);
    m_stackSize = other.m_stackSize;
    m_stack = other.m_stack;
    m_status = other.m_status;

    other.m_schedulePtr = nullptr;
    other.m_stack = nullptr;
}

Coroutine &Coroutine::operator=(Coroutine &&other) {
    if (&other != this) {
        delete[] m_stack;

        m_ctx = other.m_ctx;
        m_schedulePtr = other.m_schedulePtr;
        m_func = std::move(other.m_func);
        m_stackSize = other.m_stackSize;
        m_stack = other.m_stack;
        m_status = other.m_status;

        other.m_schedulePtr = nullptr;
        other.m_stack = nullptr;
    }
    return *this;
}

void Coroutine::run() {
    m_func();
    m_status = CoroutineStatus::FINISH;
    m_schedulePtr->yield();
}

Schedule::Schedule() { m_curcoroutine = nullptr; }

Schedule::~Schedule() {}

void Schedule::CreateCoroutine(std::function<void()> func) {
    auto co = std::make_unique<Coroutine>(std::move(func));
    co->SetSchedule(this);
    m_coroutines.push_back(std::move(co));
}

void Schedule::run() {
    getcontext(&m_mainctx);
    while (true) {
        bool hasReady = false;
        for (int i = 0; i < m_coroutines.size(); i++) {
            auto &co = m_coroutines[i];
            if (co->GetCoroutineStatus() == CoroutineStatus::READY) {
                hasReady = true;
                m_curcoroutine = co.get();
                m_curcoroutine->SetCoroutineStatus(CoroutineStatus::RUNING);
                swapcontext(&m_mainctx, &(co->GetContext()));
                break;
            }
        }
        if (!hasReady) {
            return;
        }
    }
}

void Schedule::yield() {
    if (m_curcoroutine->GetCoroutineStatus() == CoroutineStatus::RUNING) {
        m_curcoroutine->SetCoroutineStatus(CoroutineStatus::SUSPEND);
    }
    swapcontext(&m_curcoroutine->GetContext(), &m_mainctx);
}