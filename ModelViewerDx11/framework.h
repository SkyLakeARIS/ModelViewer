// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once
#pragma comment(lib, "libfbxsdk-md.lib")    // 컴파일 플래그가 md 설정일 경우 ) 속성-c/c++-코드 생성-런타임 라이브러리
#pragma comment(lib, "libxml2-md.lib")    // 
#pragma comment(lib, "zlib-md.lib")       //

//#pragma comment(lib, "libfbxsdk-mt.lib") // 컴파일 플래그가 mt 설정일 경우

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput8")
#pragma comment(lib, "xinput.lib")

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

// d3d11 header files
//#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectxTex.h>
#include <DirectXColors.h>
#include <dxgidebug.h>
#include <dinput.h>
#include <Xinput.h>

// fbx
#include <fbxsdk.h>

// Windows 헤더 파일
#define NOMINMAX
#include <windows.h>

// C 런타임 헤더 파일입니다.
#include <iostream>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>
#include <crtdbg.h>
// my header
#include "Type.h"
#include "Timer.h"
#include "Input.h"

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
