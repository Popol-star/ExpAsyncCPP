#pragma once
#include "AsyncScreen.h"
class AsyncGame :public AsyncScreen {
protected:
    virtual AsyncCoroutine<void> game_update() = 0;;
private:
    // Inherited via AsyncScreen
    AsyncCoroutine<void> update_coroutine() override;

};