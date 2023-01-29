#pragma once


#define SAFETY_RELEASE(obj)     \
 if((obj) != nullptr)           \
{                               \
     obj->Release();            \
     obj = nullptr;             \
}                               \

#define ASSERT(expr, msg)                                               \
 if(!(expr))                                                            \
{                                                                       \
     fprintf(stderr, "%s (%d), msg: %s\n", __FILE__, __LINE__, msg);    \
     __debugbreak();                                                    \
}                                                                       \

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
