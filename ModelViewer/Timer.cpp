#include "pch.h"
#include "Timer.h"

//Timer::Timer()
//    : mLastTime(0)
//    , mTimeScale(0.0f)
//    , mDeltaTime(0.0f)
//{
//    uint64_t frequency;
//    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
//    mTimeScale = 1 / (float)frequency;
//
//    QueryPerformanceCounter((LARGE_INTEGER*)&mLastTime);
//}
int64_t Timer::mLastTime = 0;
float Timer::mTimeScale = 0;
float Timer::mDeltaTime = 0;

void Timer::Initialize()
{
    mLastTime = 0;
    mTimeScale = 0.0f;
    mDeltaTime = 0.0f;
    uint64_t frequency;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    mTimeScale = 1 / (float)frequency;

    QueryPerformanceCounter((LARGE_INTEGER*)&mLastTime);
}

// 매 프레임마다 DeltaTime을 갱신합니다.
// 매 프레임마다 호출되야 합니다.
void Timer::Tick()
{
    uint64_t currentTime = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
    mDeltaTime = (currentTime - mLastTime) * mTimeScale;
    mLastTime = currentTime;
}

// Tick()에서 갱신된 DeltaTime을 반환합니다.
float Timer::GetDeltaTime()
{
    return mDeltaTime;
}
