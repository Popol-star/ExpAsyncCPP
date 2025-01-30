#include <iostream>
#include <UnBoundedSPSC.h>
#include <AsyncExecutor.h>
#include <Async.h>
#include <SingleShot.h>
static async::AsyncCoroutine<void> co(async::SPSCReader<int>&& reader) {
    std::vector<int> t= co_await async::utils::collect(std::move(reader));
    for (int i : t) {
        std::cout << "{ " << i << " }";
    }
    std::cout << std::endl;
};

static async::AsyncCoroutine<void> coss(async::SingleShotReader<int>&& reader) {
    std::optional<int> result= co_await std::move(reader);
    if (result) {
        std::cout << "single shot found" << *result<<std::endl;
    }
    else {
        std::cout << "writer deleted" << std::endl;
    }
}
int main()
{
    auto spsc =  std::make_shared<async::UnBoundedSPSC<int>>();
    auto spsc2 = std::make_shared<async::UnBoundedSPSC<int>>();
    auto spsc3 = std::make_shared<async::UnBoundedSPSC<int>>();
    auto singleShot = std::make_shared<async::SingleShot<int>>();
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
    std::thread t3([writer = async::SPSCWriter<int>(spsc3)]() mutable {
        int i = 0;
        while (i < 50) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            writer.push(i);
            i++;
        }
        });
    std::thread t4([writer = async::SingleShotWriter(singleShot)]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        //writer.set_value(1);
        });
    async::BlockingExecutor().block_on([&spsc, &spsc2,&spsc3,&singleShot]()->async::AsyncCoroutine<void> {
        co_await join_task(co(std::move(async::SPSCReader(spsc))),
                           co(async::SPSCReader(spsc2)),
                           co(async::SPSCReader(spsc3)),
                           coss(async::SingleShotReader(singleShot))
                           );
        }());
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}