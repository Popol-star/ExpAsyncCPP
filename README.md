# ExpAsyncCPP

## A simple C++20 experimental async library 

The repository aims to show a possible way to implement an asynchronous environment using coroutines.
Inspired by the rust async environment.

The design is imperfect and not optimized, it may change in the future.

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

Use `co_return` instead of `return`.\
use `co_await` to wait another AsyncCoroutine (or other premitive).\

An async::AsyncCoroutine can be returned by a function, method or a virtual method.

### Waiting for multiple Tasks

The function `async::join` use used to wait multiple waitables.\
Waiting `async::join` produce a tuple. 

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

SingleShots are used to send single message between two threads (SPSC).

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

async::util::collect is able to collect all the elements from the queue in a std::vector.

### Single producer/consumer queue

SingleShots are used to send multiple messages between two threads (SPSC).
A SingleShot contains a not copiable writer and a not copiable reader.\
Elements can be retrieved from the queue with `popAsync`.
An empty optional as a result means the writer has been deleted and the queue is empty.

```cpp
#include <UnBoundedSPSC.h>
#include <string>
static async::AsyncCoroutine<void> coroutine(async::SPSCReader<std::string> reader) {
    std::optional<std::string> result;
    do {
        result = co_await reader.popAsync();
        if (result) {
            std::cout << "result \"" << *result << "\"" << std::endl;
        }
        else {
            //in this case, the writer is deleted without sending a message.
            std::cout << "writer deleted\n";
        }
    } while (result); 
}

int main()
{
    using namespace std::chrono_literals;
    async::UnboundedSPSC<std::string> signal;
    std::thread t([writer=signal.getWriter()]() {
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(5s);
            writer.push(std::string("message ").append(std::to_string(i)));
        }
    });
    async::BlockingExecutor().block_on(coroutine(signal.getReader()));
    t.join();
}
```
async::utils::collect can be used to retrieve all elements in the queue in a std::vector.
The function wait until the writer is deleted.

```
static async::AsyncCoroutine<void> coroutine(async::SPSCReader<std::string> reader) {
    std::vector<std::string> vec = co_await async::utils::collect(reader);
    for (std::string& str : vec) {
        std::cout << str << std::endl;
    }
}

int main()
{
    using namespace std::chrono_literals;
    async::UnboundedSPSC<std::string> signal;
    std::thread t([writer=signal.getWriter()]() {
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(5s);
            writer.push(std::string("message ").append(std::to_string(i)));
        }
    });
    async::BlockingExecutor().block_on(coroutine(signal.getReader()));
    t.join();
}

```
## Optimizations

It may be slow... i dunno.

