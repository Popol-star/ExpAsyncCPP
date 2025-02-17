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

static async::AsyncCoroutine<void> coroutine() {
    using namespace std::chrono_literals;
    bool light_state = false;
    while (true) {
        co_await async::Timer(500);
        if (light_state) {
            std::cout << "light on\n";
        }
        else {
            std::cout << "light off\n";
        }
        light_state = !light_state;
   }
}
static async::AsyncCoroutine<void> coroutine2() {
    using namespace std::chrono_literals;
    bool light_state = false;
    while (true) {

        co_await async::Timer(5000);
       
        if (light_state) {
            std::cout << "ZGEG on\n";
        }
        else {
            std::cout << "ZGEG off\n";
        }
        light_state = !light_state;
    }
}
int main()
{
    using namespace std::chrono_literals;
    async::BlockingExecutor()
        .block_on({ coroutine(),coroutine2()});
}