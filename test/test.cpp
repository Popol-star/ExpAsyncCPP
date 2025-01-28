#include <iostream>
#include <UnBoundedSPSC.h>
#include <BlockingAsyncExecutor.h>

static AsyncCoroutine<void> co(SPSCReader<int>&& reader) {
    while (true) {
        std::optional<int> t = co_await reader.popAsync();
        if (t) {
            std::cout << *t << std::endl;
        }
        else {
            printf("writer closed\n");
            co_return;
        }
    }
};
static AsyncCoroutine<void> test(SPSCReader<int>&& reader1, SPSCReader<int>&& reader2) {
    co_await join_task(co(std::move(reader1)),
                       co(std::move(reader2)));
}
int main()
{
    auto spsc = std::make_shared<UnBoundedSPSC<int>>();
    auto spsc2 = std::make_shared<UnBoundedSPSC<int>>();
    std::thread t1([writer = SPSCWriter<int>(spsc)]() mutable {
        int i = 0;
        while (i < 10) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            writer.push(1);
            i++;
        }
    });
    std::thread t2([writer = SPSCWriter<int>(spsc2)]() mutable {
        int i = 0;
        while (i < 15) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            writer.push(2);
            i++;
        }
        });    BlockingExecutor().block_on(test(SPSCReader(spsc), SPSCReader(spsc2)));
    t1.join();
    t2.join();
}
