#include "Timer.h"

namespace core
{
    int64_t Timer::mLastTime = 0;
    double Timer::mTimeScale = 0;
    double Timer::mDeltaTime = 0;

    void Timer::Initialize()
    {
        mLastTime = 0;
        mTimeScale = 0.0;
        mDeltaTime = 0.0;
        uint64_t frequency;
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        mTimeScale = 1.0 / static_cast<double>(frequency);

        QueryPerformanceCounter((LARGE_INTEGER*)&mLastTime);
    }

    // 매 프레임마다 DeltaTime을 갱신합니다.
    // 매 프레임마다 호출되야 합니다.
    void Timer::Tick()
    {
        LARGE_INTEGER currentTime;

        QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
        mDeltaTime = (currentTime.QuadPart - mLastTime) * mTimeScale;
        mLastTime = currentTime.QuadPart;
    }

    // Tick()에서 갱신된 DeltaTime을 반환합니다.
    double Timer::GetDeltaTime()
    {
        return mDeltaTime;
    }

    double Timer::GetNowMS()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
        return ((currentTime.QuadPart * 1000LL) / 10000000.0);
    }
}