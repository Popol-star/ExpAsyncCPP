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
public:
    TaskThread(size_t nb_threads) :_queue(), _threads(), _mtx() {
        for (int i = 0; i < nb_threads; i++) {
            _threads.emplace_back([this]() {
                while (true) {
                    std::unique_lock lck(_mtx);
                    _cond.wait(lck, [this]() {return !_queue.empty(); });
                    auto fn = std::move(_queue.front());
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
        for (std::thread& t : _threads)
        {
            t.join();
        }
    }
};

static async::AsyncCoroutine<void> coroutine() {
    using namespace std::chrono_literals;
    std::cout <<" wait to the end"<< std::endl;
    std::cout <<"sleep 5s"<< std::endl;
    co_await async::Timer(5000);
    
    std::cout << "finished\n";
}

int main()
{
    using namespace std::chrono_literals;
    async::BlockingExecutor()
        .block_on(coroutine());
}