#pragma once
#include "Async.h"
#include <mutex>
#include <list>
namespace async {
    class BlockingExecutor :private Executor {
    private:
        struct WaitingRoom {
            std::coroutine_handle<> handle;
            Pollable* pollable;
        };
        std::mutex _mtx;
        std::condition_variable _var;
        std::list<WaitingRoom> _waitings;
        bool _awake;
        // Inherited via Executor
        void awake() override;

        //
        void add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority) override;
    public:
        BlockingExecutor();

        void block_on(AsyncCoroutine<void> coroutine);
    };
}