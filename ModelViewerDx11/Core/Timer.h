#pragma once
#include "../framework.h"

namespace core
{
    class Timer final
    {
    public:
        Timer() = delete;
        ~Timer() = delete;

        static void     Initialize();

        static void     Tick();
        static double    GetDeltaTime();
        static double    GetNowMS();

    private:
        static int64_t  mLastTime;
        static double    mTimeScale;
        static double    mDeltaTime;
    };
}