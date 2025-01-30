#include <iostream>
#include <Async.h>
#include <AsyncExecutor.h>
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