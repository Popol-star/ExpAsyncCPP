#include "AsyncGame.h"

AsyncCoroutine<void> AsyncGame::update_coroutine()
{
    co_await game_update();
}
