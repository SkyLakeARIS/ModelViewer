#pragma once
#include <cstdint>

class Timer final
{
public:
    Timer() = delete;
    ~Timer() = delete;
    static void Initialize();

    static void Tick();
    static float GetDeltaTime();
private:
    static int64_t mLastTime;
    static float mTimeScale;
    static float mDeltaTime;
};
//
