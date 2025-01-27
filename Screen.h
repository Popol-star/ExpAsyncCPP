#pragma once
struct Screen {
    virtual void onEnter() = 0;
    virtual void onUpdate() = 0;
    virtual void onQuit() = 0;
    virtual ~Screen(){}
};