#pragma once
#include <atomic>
#include <list>
#include "Screen.h"
#include "Async.h"
#include <array>
class AsyncScreen :public Screen
{
private:
    class Exe :public Executor {
        std::atomic_bool _flg;
        struct WaitingRoom {
            std::coroutine_handle<> handle;
            Pollable* pollable;
        };
        std::list<WaitingRoom> _waitings;
    public:
        Exe();
        void update();

        void clear();
        // Inherited via Waker
        void awake() override;
        // Inherited via Executor
        void add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority=TaskPriority::Indirect) override;
    };

    Exe _enter_executor;
    Exe _update_executor;
    AsyncCoroutine<void> _enterCoro;
    AsyncCoroutine<void> _updateCoro;
protected:
    virtual AsyncCoroutine<void> enter_coroutine() = 0;
    virtual AsyncCoroutine<void> update_coroutine() = 0;
public:
     AsyncScreen();
     void onEnter() override;
     void onUpdate() override;
     void onQuit() override;
     std::array<Executor*, 2> get_executors();
     ~AsyncScreen() override{}
};

