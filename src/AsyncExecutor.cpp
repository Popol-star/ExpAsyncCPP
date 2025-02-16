#include "AsyncExecutor.h"
#include <algorithm>
using namespace async;
BlockingExecutor::BlockingExecutor():
    _mtx(),
    _awake(false),
    _var(),
    _waitings(),
    _timers()
{
}

void async::BlockingExecutor::register_timer(Timer *timer)
{
    _timers.push_back(timer);
}

void BlockingExecutor::block_on(AsyncCoroutine<void> coroutine)
{
    std::chrono::time_point<std::chrono::steady_clock> last_call;
    
    coroutine.set_executor(this);
    _waitings.push_back({ coroutine.handle,nullptr });
    _awake = true;
    size_t slp=0;
    while (!coroutine.done())
    {

        if(_timers.empty())
        {
            std::unique_lock lck(_mtx);
            _var.wait(lck, [this]() {return _awake; });
            _awake = false;
        }
        else{
            std::unique_lock lck(_mtx);
            _var.wait_for(lck,std::chrono::milliseconds(slp), [this]() {return _awake; });
            _awake = false;
        }
        update_timers(last_call);
        update_coroutines();
        if(!_timers.empty()){
            slp= (*std::min_element(_timers.begin(),_timers.end(),[](Timer* t1,Timer* t2){
                return std::min(t1->remaining,t2->remaining);
            }))->remaining;
        }
        last_call=std::chrono::steady_clock::now();
    }
   
}

void async::BlockingExecutor::update_coroutines()
{
    auto it = _waitings.begin();
    while (it != _waitings.end())
    {
        if (!it->pollable || it->pollable->is_ready())
        {
            it->handle.resume();
            it = _waitings.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void async::BlockingExecutor::update_timers(const std::chrono::_V2::steady_clock::time_point &last_call)
{
    auto timespan = std::chrono::steady_clock::now() - last_call;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timespan).count();
    auto it_timer = _timers.begin();
    while (it_timer != _timers.end())
    {
        auto timer = (*it_timer);
        if (timer->remaining > ms)
        {
            timer->remaining -= ms;
          
            ++it_timer;
        }
        else
        {
            timer->remaining = 0;
            it_timer = _timers.erase(it_timer);
        }
    }
}

void BlockingExecutor::awake()
{
    std::unique_lock lck(_mtx);
    if (!_awake) {
        _awake = true;
        lck.unlock();
        _var.notify_one();
    }
}

void BlockingExecutor::add_task(std::coroutine_handle<> handle, Pollable* pollable,TaskPriority priority)
{
    switch (priority) {
    case TaskPriority::Direct:
        _waitings.push_back({ handle,pollable });
        break;
    case TaskPriority::Indirect:
        _waitings.push_front({ handle,pollable });
        break;
    }
}
/*

Non blocking Executor

*/
void async::NBlockingExecutor::awake()
{
    _awake.store(true, std::memory_order_release);
}

void async::NBlockingExecutor::add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority)
{
    switch (priority) {
    case TaskPriority::Direct:
        _waitings.push_back({ handle,pollable });
        break;
    case TaskPriority::Indirect:
        _waitings.push_back({ handle,pollable });
        break;
    }
}

void async::NBlockingExecutor::update()
{
    while (_awake.exchange(false, std::memory_order_consume)) {
        auto it = _waitings.begin();
        while (it != _waitings.end()) {
            if (!it->pollable || it->pollable->is_ready()) {
                it->handle.resume();
                it = _waitings.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void async::NBlockingExecutor::start(AsyncCoroutine<void> coroutine)
{
    add_task(coroutine.getHandle(), nullptr, TaskPriority::Direct);
    _coroutine=std::move(coroutine);
    _coroutine.set_executor(this);
    awake();
}

bool async::NBlockingExecutor::is_finished() const noexcept
{
    return !_coroutine||_coroutine.done();
}

void async::NBlockingExecutor::register_timer(Timer *timer)
{
}
