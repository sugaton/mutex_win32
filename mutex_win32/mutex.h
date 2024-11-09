#pragma once

#include <windows.h>
#include <synchapi.h>
#include <chrono>

namespace win32_mutex{

    class CriticalSection {
    private:
        CRITICAL_SECTION cs;
    public:
        CriticalSection() : cs() 
        {
            InitializeCriticalSection(&cs);
        }
        ~CriticalSection()
        {
            DeleteCriticalSection(&cs);
        }

        CriticalSection(const CriticalSection&) = delete;
        CriticalSection& operator=(const CriticalSection&) = delete;
        
        void lock() 
        {
            EnterCriticalSection(&cs);
        }
        void release()
        {
            LeaveCriticalSection(&cs);
        }

        PCRITICAL_SECTION get() 
        {
            return &cs;
        }
    };
    
    class lock_guard {
    private:
        CriticalSection& m_cs;
    public:
        lock_guard(CriticalSection& cs) 
        : m_cs(cs)
        {
            m_cs.lock();
        }
        ~lock_guard() {
            m_cs.release();
        }
    };

class ConditionVariable {
private:
    CONDITION_VARIABLE cond_var;

public:
    ConditionVariable() : cond_var{} {
        InitializeConditionVariable(&cond_var);
    }

    // Destructor: Wake up all waiting threads
    ~ConditionVariable() {
        notify_all();
    }

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    // Notifies one waiting thread
    void notify_one() {
        WakeConditionVariable(&cond_var);
    }

    // Notifies all waiting threads
    void notify_all() {
        WakeAllConditionVariable(&cond_var);
    }

    // Waits until notified
    void wait(CriticalSection& cs) {
        SleepConditionVariableCS(&cond_var, cs.get(), INFINITE);
    }

    // Waits until notified and condition becomes true
    template <class Predicate>
    void wait(CriticalSection& cs, Predicate pred) {
        while(!pred()){
            SleepConditionVariableCS(&cond_var, cs.get(), INFINITE);
        }
    }
};

}