#include <iostream>
#include "timer.h"
#include "AsyncScreen.h"
#include "TestIOThread.h"
#include <Windows.h>
struct MyAsyncGame:public AsyncScreen{
    AsyncCoroutine<void> enter_coroutine() override
    {
        printf("entering\n");
        co_await Timer(5000);
        co_return;
    }
    AsyncCoroutine<void> update_coroutine() override
    {
        printf("Start update\n");
        printf("Start waiting 5s\n");
        co_await Timer(5000);
        printf("finished waiting 5s\n");
        printf("Start waiting 6s\n");
        co_await Timer(6000);
        printf("finished waiting 6s\n");
    }
};
int main()
{
    using namespace std::chrono_literals;
    TestIO io;
    MyAsyncGame game;
    game.onEnter();
    while (true) {
        TimerEngine::instance().refresh();
        game.onUpdate();
        std::this_thread::sleep_for(15ms);
    }
    game.onQuit();
}
