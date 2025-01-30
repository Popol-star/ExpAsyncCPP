#include <iostream>
#include <Async.h>
#include <AsyncExecutor.h>
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
       // writer.set_value("Message from thread");
    });
    async::BlockingExecutor().block_on(coroutine(signal.getReader()));
    t.join();
}