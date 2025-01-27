#include "AsyncScreen.h"
#include "timer.h"
AsyncScreen::Exe::Exe() :_waitings(), _flg()
{
}

void AsyncScreen::Exe::update()
{
    while (_flg.exchange(false, std::memory_order_consume)) {
        auto it = _waitings.begin();
        while (it != _waitings.end()) {
            if (!it->pollable || it->pollable->is_valid()) {
                it->handle.resume();
                it = _waitings.erase(it);
            }
            else {
                it++;
            }
        }
    }
}

void AsyncScreen::Exe::clear()
{
    _waitings.clear();
}

void AsyncScreen::Exe::awake()
{
    _flg.store(true, std::memory_order_release);
}

void AsyncScreen::Exe::add_task(std::coroutine_handle<> handle, Pollable* pollable, TaskPriority priority)
{
    switch (priority)
    {
    case Executor::TaskPriority::Direct:
        _waitings.push_back({ handle,pollable });
        break;
    case Executor::TaskPriority::Indirect:
        _waitings.push_front({ handle,pollable });
        break;
    default:
        break;
    }
}

AsyncScreen::AsyncScreen():
    _updateCoro(),
    _enterCoro()
{
}

void AsyncScreen::onEnter()
{
    _enterCoro = enter_coroutine();
    TimerEngine::instance().subscribe_executor(&_enter_executor);
    TimerEngine::instance().subscribe_executor(&_update_executor);
    _enterCoro.set_executor(&_enter_executor);
    _enter_executor.add_task(_enterCoro.getHandle(), nullptr);
    _enter_executor.awake();
    _enter_executor.update();
} 

void AsyncScreen::onUpdate()
{
    if (_enterCoro&&!_enterCoro.done()) {
        _enter_executor.update();
    }
    else {
        if (_enterCoro) {
            TimerEngine::instance().unsubscribe_executor(&_enter_executor);
            _enterCoro = AsyncCoroutine<void>(nullptr);
        }
        if (!_updateCoro|| _updateCoro.done()) {
            _updateCoro = update_coroutine();
            _updateCoro.set_executor(&_update_executor);
            _update_executor.add_task(_updateCoro.getHandle(), nullptr);
            _update_executor.awake();
        }
        _update_executor.update();
    }
}
std::array<Executor*, 2> AsyncScreen::get_executors() {
    return { &_enter_executor,&_update_executor };
}
void AsyncScreen::onQuit()
{
    TimerEngine::instance().unsubscribe_executor(&_enter_executor);
    TimerEngine::instance().unsubscribe_executor(&_update_executor);
    _enter_executor.clear();
    _enterCoro = AsyncCoroutine<void>(nullptr);
}
