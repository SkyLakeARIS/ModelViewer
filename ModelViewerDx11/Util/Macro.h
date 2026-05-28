#pragma once

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
        _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, nullptr, format, ##__VA_ARGS__); \
        __debugbreak();                                                                 \
    }                                                                                   \

    #ifdef _DEBUG
    #define SET_PRIVATE_DATA(obj, objectNameStr)         \
        if(obj != nullptr)                               \
        {                                                \
            obj->SetPrivateData(                         \
                WKPDID_D3DDebugObjectName,               \
                sizeof(objectNameStr)-1,                 \
                objectNameStr);                          \
        }                                                \

    #else
    #define SET_PRIVATE_DATA(obj, objectNameStr)
    #endif
}