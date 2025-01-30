# ExpAsyncCPP

## A simple C++20 experimental async library 

The repository aim to show a possible way to implement a asynchronous environment using coroutines.

Async.h contains:\
-AsyncCoroutine: coroutines used in this library.\
-Executor: Interface defining an coroutines executors.\
-Awaitable: Template abstract class defining a waitable premitive.

AsyncExecutor.h/cpp contains:\
Executor implementations.\
-A blocking executor (block the thread).\
-A nonblocking executor (Can be usefull for video games).

SingleShot.h contains:\
-Single producer, single consumer messaging premitive. Only one element can be sended.\
This premitive is used to send a message threadsafely.

UnboundedSPSC.h contains:\
-Single producer, single consumer messaging premitive. Multiple elements can be sended.\
This premitive is used to send messages threadsafely.

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

### Waiting for multiple Task

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

## Optimizations

It may be slow... i dunno.
