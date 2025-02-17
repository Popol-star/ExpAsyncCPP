#pragma once
#include "Async.h"
#include <mutex>
#include <list>
#include <vector>
#include <condition_variable>
#include <atomic>
#include "SharedPoolAllocator.h"
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
        std::list<Timer*> _timers;
        size_t _minimum_remaining_timer;
        bool _awake;
        // Inherited via Executor
        void awake() override;

        //
        void add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority) override;
        void update_coroutines();
        void update_timers(const std::chrono::time_point<std::chrono::steady_clock> &last_call);
    public:
        BlockingExecutor();
        void register_timer(Timer* timer) override;
        void block_on(std::initializer_list<AsyncCoroutine<void>> coroutine);
        
    };

    class NBlockingExecutor :private Executor {
    private:
        AsyncCoroutine<void> _coroutine;
        struct WaitingRoom {
            std::coroutine_handle<> handle;
            Pollable* pollable;
        };
        std::list<WaitingRoom> _waitings;
        std::list<Timer*> _timers;
        std::atomic_bool _awake;
        std::chrono::time_point<std::chrono::steady_clock> _last_call;
 
        void awake() override;

        //
        void add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority) override;
    public:
        /*
            
        */
        void update();
        /*
            Set the coroutine.
        */
        void start(AsyncCoroutine<void> coroutine);
        /*
            Returns true if the given coroutine is done
        */
        bool is_finished() const noexcept;
        /*
        */
        void register_timer(Timer* timer) override;
    };
}