# ExpAsyncCPP

## A simple C++20 experimental async library 

The repository aim to show a possible way to implement a asynchronous environment using coroutines.
Inspired by the rust async environment.

The design may change in the future.

### Basic syntax

```cpp
#include <iostream>
#include <Async.h>
#include <AsyncExecutor.h>
static async::AsyncCoroutine<int> subcoroutine() {
    co_return 200;
}
static async::AsyncCoroutine<void> coroutine() {
    std::cout << co_await subcoroutine() << std::endl;
}
int main()
{   //block the thread until coroutine is finished.
    async::BlockingExecutor().block_on(coroutine());
}
```

Use co_return instead of return.\
use co_await to wait an other AsyncCoroutine (or other premitive).\

an async::AsyncCoroutine can be returned by a function, method or a virtual method.

### Waiting for multiple Tasks

The function async::join use used to wait multiple waitable.\
Waiting async::join produce a tuple. 

```cpp
static async::AsyncCoroutine<int> subcoroutine() {
    co_return 200;
}
static async::AsyncCoroutine<double> subcoroutine2() {
    co_return 19.2;
}
static async::AsyncCoroutine<void> coroutine() {
    auto result = co_await async::join(subcoroutine(), subcoroutine2());
    std::cout << std::get<0>(result) << std::endl;
    std::cout << std::get<1>(result) << std::endl;
}
int main()
{
    async::BlockingExecutor().block_on(coroutine());
}
```

If the result isn't needed, async::join_task have the same behavior but waiting it produce void.  

### Single shot premitive

SingleShots are used to send only one message between 2 threads (SPSC).

A SingleShot contains a not copiable writer and a not copiable reader.\
The reader can be awaited.\
An empty optional as a result means the writer has been deleted without sending a message.

```cpp
#include <SingleShot.h>
static async::AsyncCoroutine<void> coroutine(async::SingleShotReader<std::string> reader) {
    std::optional<std::string> result = co_await reader;
    if (result) {
        std::cout << "result \"" << *result <<"\"" << std::endl;
    }
    else {
        //in this case, the writer is deleted without sending a message.
        std::cout << "writer deleted\n";
    }
}

int main()
{
    using namespace std::chrono_literals;

    async::SingleShot<std::string> signal;
    std::thread t([writer=signal.getWriter()]() {
        std::this_thread::sleep_for(5s);
        writer.set_value("Message from thread");
    });
    async::BlockingExecutor().block_on(coroutine(signal.getReader()));
    t.join();
}
```

## Optimizations

It may be slow... i dunno.

