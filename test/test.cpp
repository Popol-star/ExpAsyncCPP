#include <iostream>
#include <UnBoundedSPSC.h>
#include <AsyncExecutor.h>
#include <Async.h>
static async::AsyncCoroutine<void> co(async::SPSCReader<int>&& reader) {
    std::vector<int> t= co_await async::utils::collect(std::move(reader));
    for (int i : t) {
        std::cout << "{ " << i << " }";
    }
    std::cout << std::endl;
};

int main()
{
    auto spsc =  std::make_shared<async::UnBoundedSPSC<int>>();
    auto spsc2 = std::make_shared<async::UnBoundedSPSC<int>>();
    std::thread t1([writer = async::SPSCWriter<int>(spsc)]() mutable {
        int i = 0;
        while (i < 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            writer.push(i);
            i++;
        }
    });
    std::thread t2([writer = async::SPSCWriter<int>(spsc2)]() mutable {
        int i = 0;
        while (i < 15) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            writer.push(i);
            i++;
        }
    });
    async::BlockingExecutor().block_on([&spsc,&spsc2]()->async::AsyncCoroutine<void>{
        co_await join_task(co(std::move(async::SPSCReader(spsc))),
                           co(async::SPSCReader(spsc2)));
        }());
    t1.join();
    t2.join();
}