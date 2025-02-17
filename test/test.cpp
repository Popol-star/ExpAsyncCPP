#include <iostream>
#include <AsyncExecutor.h>
#include <string>
#include <Signal.h>
#include <SingleShot.h>
#include <queue>
#include "MoveOnlyFunction.h"
class TaskThread {
private:
    std::queue<std::move_only_function<void(void)>> _queue;
    std::mutex _mtx;
    std::condition_variable _cond;
    std::vector<std::thread> _threads;
    bool stopped = false;
public:
    TaskThread(size_t nb_threads) :_queue(), _threads(), _mtx() {
        for (int i = 0; i < nb_threads; i++) {
            _threads.emplace_back([this]() {
                while (true) {
                    std::unique_lock lck(_mtx);
                    _cond.wait(lck, [this]() {return !_queue.empty(); });
                    std::move_only_function<void()> fn =std::move(_queue.front());
                    _queue.pop();
                    lck.unlock();
                    fn();
                }
            });
        }
    }
    template <class FUNC>
    auto addTask(FUNC fnc) {
        std::unique_lock lck(_mtx);

        // Ensure SingleShot takes the value type, not a reference
        using ReturnType = std::decay_t<decltype(fnc())>; // Decay to value type

        async::SingleShot<ReturnType> ss;
        async::SingleShotWriter<ReturnType> writer = ss.getWriter();

        _queue.push([func = std::move(fnc), w = std::move(writer)]() mutable {
            // Call the function and set the value
            w.set_value(func());
            });

        if (_queue.size() == 1) {
            lck.unlock();
            _cond.notify_one();
        }
        return ss.getReader();
    }

    ~TaskThread() {
        _mtx.lock();
        stopped = true;
        _mtx.unlock();
        for (std::thread& t : _threads)
        {
            t.join();
        }
    }
};

static async::AsyncCoroutine<void> blinker() {
    while (true) {
        co_await async::Timer(1000);
        std::cout << "Light on\n";
        co_await async::Timer(1000);
        std::cout << "Light off \n";
    }
}
static async::AsyncCoroutine<void> coroutine() {
        co_await async::Timer(2000);
        std::cout << "Waited 2 seconds\n";
        co_await async::Timer(60000);
        std::cout << "Waited 1 minute\n";
}
int main()
{
    using namespace std::chrono_literals;
    async::BlockingExecutor()
        .block_on({ blinker(),coroutine()});
}