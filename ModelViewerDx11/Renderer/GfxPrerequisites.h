#pragma once
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectxTex.h>
#include <dxgidebug.h>

using namespace DirectX;

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
