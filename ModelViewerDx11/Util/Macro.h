#pragma once
#include <crtdbg.h>

namespace util
{
    // macro
    #define SAFETY_RELEASE(obj)     \
     if((obj) != nullptr)           \
    {                               \
         obj->Release();            \
         obj = nullptr;             \
    }                               \

    #define ASSERT(expr, format, ...)                                                   \
    if(!(expr))                                                                         \
    {                                                                                   \
        _CrtDbgReportW(_CRT_ASSERT, _CRT_WIDE(__FILE__), __LINE__, nullptr, _CRT_WIDE(format), ##__VA_ARGS__); \
        __debugbreak();                                                                 \
    }                                                                                   \

}
