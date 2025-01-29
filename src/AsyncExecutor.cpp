#include "AsyncExecutor.h"
using namespace async;
BlockingExecutor::BlockingExecutor():
    _mtx(),
    _awake(false),
    _var(),
    _waitings()
{
}

void BlockingExecutor::block_on(AsyncCoroutine<void> coroutine)
{
    coroutine.set_executor(this);
    _waitings.push_back({ coroutine.handle,nullptr });
    _awake = true;
    while (!coroutine.done())
    {
        {
            std::unique_lock lck(_mtx);
            _var.wait(lck, [this]() {return _awake; });
            _awake = false;
        }
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
        _waitings.push_back({ handle,pollable });
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
