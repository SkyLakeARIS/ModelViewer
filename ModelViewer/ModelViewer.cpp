// ModelViewer.cpp : 애플리케이션에 대한 진입점을 정의합니다.

#include "pch.h"
#include "ModelViewer.h"
#include "Camera.h"
#include "ModelImporter.h"
#include "DxUtil.h"


#define MAX_LOADSTRING 100



// 16byte aligned
struct CBCamera
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMMATRIX mWorldViewProjection;
};

struct CBLight
{
    XMFLOAT4 vLightColor;
    XMFLOAT4 vLightDir;
};

// 프로그램 전역변수들
HINSTANCE    gInstance;                                // 현재 인스턴스입니다.
HWND         gWnd;
WCHAR        szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR        szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
size_t       gWidth = 1024;
size_t       gHeight = 720;

// D3D 전역변수들
ID3D11Device*                   gDevice = nullptr;
ID3D11DeviceContext*            gDeviceContext = nullptr;
IDXGISwapChain*                 gSwapChain = nullptr;
ID3D11RenderTargetView*         gRenderTargetView = nullptr;
ID3D11Texture2D*                gDepthStencil = nullptr;
ID3D11DepthStencilView*         gDepthStencilView = nullptr;
D3D_DRIVER_TYPE                 gDriverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL               gFeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11VertexShader*             gVertexShader = nullptr;
ID3D11PixelShader*              gPixelShader = nullptr;

ID3D11InputLayout*              gVertexLayout = nullptr;
ID3D11Buffer*                   gVertexBuffer = nullptr;
ID3D11Buffer*                   gIndexBuffer = nullptr;

ID3D11SamplerState*             gSamplerAnisotropic = nullptr;
enum { NUMBER_TEXTURE = 5 };
ID3D11ShaderResourceView*       gShaderResourceView[NUMBER_TEXTURE];


// 전역변수들
ID3D11Buffer*           gCBCamera = nullptr;
ID3D11Buffer*           gCBChangeOnResize = nullptr;
ID3D11Buffer*           gCBChangesEveryFrame = nullptr;
ID3D11Buffer*           gCBLight = nullptr;

// global model
ModelImporter           gImporter;
Model*                  gModel = nullptr;

size_t                  gVertexCount = 0;
VertexInfo*             gVertexList = nullptr;
size_t                  gIndexListCount = 0;
unsigned int*           gIndexList = nullptr;

#ifdef USING_MYINPUT
MyInput* gMyInput = nullptr;

#else
DirectInput* gDirectInput = nullptr;

#endif

// 여기는 카메라+물체가 가지는 행렬, 굳이 전역으로 뺄 필요없긴 함.
XMMATRIX gMatWorld1;

Camera gCamera(XMVectorSet(0.0f, 10.0f, -15.0f, 0.0f)
    , XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f)
    , XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

// 물체 관련 전역 - 임시
float   gScaleFactor = 1.0f;
size_t  gIndexOfFocusedVertex;


/*
 *  함수 선언
 */
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT             InitD3DDevice();
HRESULT             InitGeometry();
void                UpdateInput(float deltaTime);
HRESULT             Render(float deltaTime);
void                Cleanup();


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MODELVIEWER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    int programReturn = FALSE;

    if (FAILED(InitInstance (hInstance, nCmdShow)))
    {
        MessageBoxW(gWnd, L"초기화 실패", L"애플리케이션 초기화 실패", MB_OK);
        ASSERT(false, "애플리케이션 초기화 실패");

        goto EXIT_PROGRAM;
    }

    if(FAILED(InitD3DDevice()))
    {
        MessageBoxW(gWnd, L"초기화 실패", L"Device 초기화 실패", MB_OK);
        ASSERT(false, "Device 초기화 실패");

        goto EXIT_PROGRAM;
    }

    gImporter.Initialize();
    if(FAILED(InitGeometry()))
    {
        MessageBoxW(gWnd, L"초기화 실패", L"오브젝트 데이터 초기화 실패", MB_OK);
        ASSERT(false, "오브젝트 데이터 초기화 실패");

        goto EXIT_PROGRAM;
    }

    Timer::Initialize();
    SetCursorPos(gWidth / (float)2, gHeight / (float)2);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (true)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Timer::Tick();
            const float deltaTime = Timer::GetDeltaTime();
#ifndef USING_MYINPUT
            gDirectInput->UpdateInput();
#endif
            UpdateInput(deltaTime);
            Render(deltaTime);
        }
    }

    programReturn = (int)msg.wParam;
    EXIT_PROGRAM:
    Cleanup();
#ifdef _DEBUG
    CheckLiveObjects();
#endif
    return programReturn;
}

//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MODELVIEWER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MODELVIEWER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   gInstance = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   gWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       0, 0, gWidth, gHeight, nullptr, nullptr, hInstance, nullptr);

   if (!gWnd)
   {
      return FALSE;
   }

   ShowWindow(gWnd, nCmdShow);
   UpdateWindow(gWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

HRESULT InitD3DDevice()
{
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE
    };
    UINT32 numDriverTypes = ARRAYSIZE(driverTypes);

    // gpu가 지원하는 최신버전으로 맞춰질 것이다.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    UINT32 numFeatureLevels = ARRAYSIZE(featureLevels);

    // 백버퍼와 프론트 버퍼를 스왑하는 방식, 백버퍼에서 프론트로 카피하는 방식 두 개가 존재함.
    DXGI_SWAP_CHAIN_DESC swapDesc;
    ZeroMemory(&swapDesc, sizeof(swapDesc));
    swapDesc.BufferCount = 1;
    swapDesc.BufferDesc.Width = gWidth;
    swapDesc.BufferDesc.Height = gHeight;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = gWnd;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.SampleDesc.Quality = 0;
    swapDesc.Windowed = TRUE;

    UINT32 createDeviceFlag = 0;
#ifdef _DEBUG
    createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT result = NULL;
    for (UINT32 driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
    {
        gDriverType = driverTypes[driverTypeIndex];
        result = D3D11CreateDeviceAndSwapChain(nullptr, gDriverType, nullptr, createDeviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapDesc, &gSwapChain, &gDevice, &gFeatureLevel, &gDeviceContext);
        if (SUCCEEDED(result))
        {
            break;
        }
    }

    if (FAILED(result))
    {
        return E_FAIL;
    }

    SET_PRIVATE_DATA(gDevice, "gDevice");



    // 백버퍼를 얻어와서 렌더타겟으로 설정하는 부분
    ID3D11Texture2D* backBuffer = nullptr;
    // 데스크탑 버전에서는 기본적으로 스왑체인은 하나의 백버퍼를 가지고 있고, uwp는 만들어줘야한다.
    result = gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    if (FAILED(result))
    {
        return E_FAIL;
    }
    // view가 붙은것과 아닌것의 차이가 있다.
    // 어떤 리소스가 있고 그것을 사용하기 위한 파생 인터페이스. 보통 읽기전용으로 동작한다.
    // 9과달리 11에서는 어떤 원천 리소스를 가지고 어떤 방식으로 사용하냐에 따라 인터페이스가 다 나뉘어 있다고 한다.
    result = gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRenderTargetView);
    // 백버퍼의 역할은 렌더타겟으로 지정해주는 것이 끝이다.
    // refCount이기 때문에 바로 사라지는건 아니고, gRenderTargetView가 백버퍼를 가지기 때문에 이후에 사라질 것.
    backBuffer->Release();
    if (FAILED(result))
    {
        return E_FAIL;
    }

    // output merger, 그래픽 버퍼에 써넣는 일을 할 때 OM이 붙음.
    gDeviceContext->OMSetRenderTargets(1, &gRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport;
    viewport.Width = (FLOAT)gWidth;
    viewport.Height = (FLOAT)gHeight;
    // 보통 0~1 값으로 지정한다.
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    // 렌더링될 영역을 지정. s가 붙으니까 여러개 지정가능.(모델링 프로그램을 생각)
    // rasterizer stage
    gDeviceContext->RSSetViewports(1, &viewport);

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = gWidth;
    depthDesc.Height = gHeight;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
    depthDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
    depthDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
    depthDesc.MiscFlags = 0;

    result = gDevice->CreateTexture2D(&depthDesc, nullptr, &gDepthStencil);
    if (FAILED(result))
    {
        ASSERT(false, "depth stencil 생성 실패");
        return E_FAIL;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    result = gDevice->CreateDepthStencilView(gDepthStencil, &depthStencilViewDesc, &gDepthStencilView);
    if (FAILED(result))
    {
        ASSERT(false, "depth stencil view 생성 실패");
        return E_FAIL;
    }

    gDeviceContext->OMSetRenderTargets(1, &gRenderTargetView, gDepthStencilView);


    //
    // DirectInput/MyInput initialize
    //
#ifdef USING_MYINPUT
    gMyInput = new MyInput;
    if (FAILED(gMyInput->Initialize(&gWnd, gWidth, gHeight)))
    {
        return E_FAIL;
    }
#else
    gDirectInput = new DirectInput;
    if (FAILED(gDirectInput->Initialize(gInstance, gWnd, gWidth, gHeight)))
    {
        return E_FAIL;
    }
#endif

    return S_OK;
    return S_OK;
}

HRESULT InitGeometry()
{
    HRESULT result;

    /*
     *
     * 셰이더 설정
     *
     */

    // blob은 보통 텍스트 형태의 뭔가를 저장할 때 사용. 대개 셰이더에서 사용. 여기에서는 컴파일된 쉐이더의 바이너리를 가질 것.
    // 11은 적어도 하나의 셰이더는 있어야 렌더링이 가능하다?
    ID3DBlob* vsBlob = nullptr;
    // d3dx는 이제는 ms에서 지원 안함. 12에서는 사용 불가. 11에서가 마지막인데, 결국에는 안쓰는게 좋긴하다.
    // d3dcompile2도 가능.
    // 내 피처 레벨에 따라서 쉐이더 버전을 선택하도록 하는것도 좋을 듯.

    result = CompileShaderFromFile(L"shader.fxh", "VS", "vs_5_0", &vsBlob);
    if (FAILED(result))
    {
        MessageBoxA(gWnd, "vertex shader can not be compiled. please check the file", "ERROR", MB_OK);
        return E_FAIL;
    }

    // create ~ 는 반드시 나중에 해제를 해줘야 한다.
    result = gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gVertexShader);

    if (FAILED(result))
    {
        vsBlob->Release();
        return E_FAIL;
    }

    // 셰이더에 전달할 정보. 시멘틱.
    // 셰이더와 정보가 일치해야 한다.
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);


    result = gDevice->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gVertexLayout);
    vsBlob->Release();
    if (FAILED(result))
    {
        return E_FAIL;
    }


    // 보통 IASetVertexBuffer 세팅은 렌더링할 때 사용한다고 한다.
    // stride는 몇바이트 건너뛰어야 다음 버텍스가 나오는가, offset은 데이터 내부에서 몇번째 바이트에 버텍스 정보가 있는가.
    // 예제에서는 단순하기 때문에 미리 세팅.
    gDeviceContext->IASetInputLayout(gVertexLayout);


    // 픽셀 셰이더도 마찬가지로 진행
    ID3DBlob* psBlob = nullptr;
    // 4.0은 D3D10버전 셰이더
    // 여러개 만들어도 된다. 컴파일 할 때 함수명만 잘 지정해두면. (여러 셰이더 컴파일 해두고, blob만 바꿔서 런타임에 쓰도록 하는것?)
    result = CompileShaderFromFile(L"shader.fxh", "PS_TextureAndLighting", "ps_5_0", &psBlob);
    if (FAILED(result))
    {
        MessageBoxA(gWnd, "pixel shader solid can not be compiled. please check the file", "ERROR", MB_OK);
        return E_FAIL;
    }

    result = gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gPixelShader);
    psBlob->Release();
    if (FAILED(result))
    {
        return result;
    }

    /*
     *
     *  버텍스, 인덱스 리스트 설정
     *
     */
    gModel = new Model();

    gImporter.LoadFbxModel("/models/anbi.fbx");

    gModel->SetupMesh(gImporter);

    size_t numMesh = gImporter.GetMeshCount();
    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    {
        OutputDebugStringW(gModel->GetMeshName(meshIndex));
        OutputDebugStringW(L"\n");
        gVertexCount += gModel->GetVertexCount(meshIndex);
    }

    gVertexList = new VertexInfo[gVertexCount];
    gModel->UpdateVertexBuffer(gVertexList, gVertexCount, 0);


    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VertexInfo) * gVertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = gVertexList;

    result = gDevice->CreateBuffer(&bd, &initData, &gVertexBuffer);
    if (FAILED(result))
    {
        ASSERT(false, "버텍스 버퍼 생성 실패 : gVertexBuffer");
        return E_FAIL;
    }

    UINT32 stride = sizeof(VertexInfo);
    UINT32 offset = 0;
    gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &stride, &offset);

    /*
     *
     * index list 구성
     *
     */

    for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
    {
        gIndexListCount += gModel->GetIndexListCount(meshIndex);
    }
    gIndexList = new size_t[gIndexListCount];
    gModel->UpdateIndexBuffer(gIndexList, gIndexListCount, 0);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(unsigned int) * gIndexListCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    initData.pSysMem = gIndexList;

    result = gDevice->CreateBuffer(&bd, &initData, &gIndexBuffer);
    if (FAILED(result))
    {
        ASSERT(false, "인덱스 버퍼 생성 실패");
        return E_FAIL;
    }

    // DXGI_FORMAT_R32G32_UINT DXGI_FORMAT_R16_UINT 
    gDeviceContext->IASetIndexBuffer(gIndexBuffer, DXGI_FORMAT_R32_UINT, 0);


    gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    /*
     *
     *  상수 버퍼 설정
     *
     */

    // 상수 버퍼 생성 : matView
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBCamera);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    result = gDevice->CreateBuffer(&bd, nullptr, &gCBCamera);
    if (FAILED(result))
    {
        ASSERT(false, "상수 버퍼 생성 실패 : gCBCamera");
        return E_FAIL;
    }

    // 상수 버퍼 생성 : projectionView
    bd.ByteWidth = sizeof(CBChangeOnResize);
    result = gDevice->CreateBuffer(&bd, nullptr, &gCBChangeOnResize);
    if (FAILED(result))
    {
        ASSERT(false, "상수 버퍼 생성 실패 : gCBChangeOnResize");
        return E_FAIL;
    }

    // 상수 버퍼 생성 : matWorld and color
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    result = gDevice->CreateBuffer(&bd, nullptr, &gCBChangesEveryFrame);
    if (FAILED(result))
    {
        ASSERT(false, "상수 버퍼 생성 실패 : gCBChangesEveryFrame");
        return E_FAIL;
    }
    // 상수 버퍼 생성 : light
    bd.ByteWidth = sizeof(CBLight);
    result = gDevice->CreateBuffer(&bd, nullptr, &gCBLight);
    if (FAILED(result))
    {
        ASSERT(false, "상수 버퍼 생성 실패 : gCBLight");
        return E_FAIL;
    }


    D3D11_SAMPLER_DESC samplerDesc = {};
    // D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR  D3D11_FILTER_MIN_MAG_MIP_POINT
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 아직 정확히는 잘 모름.
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = gDevice->CreateSamplerState(&samplerDesc, &gSamplerAnisotropic);
    if (FAILED(result))
    {
        ASSERT(false, "SamplerState 생성 실패");
        return E_FAIL;
    }

    // 텍스처 입힐 때 다시 활성화
    //// 로드한 텍스처의 리소스를 셰이더 리소스뷰로 전환 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;


    result = LoadTextureFromFileAndCreateResource(gDevice, L"textures/Anbi_Body.png", srvDesc, &gShaderResourceView[0]);
    if(FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        return E_FAIL;
    }

    result = LoadTextureFromFileAndCreateResource(gDevice, L"textures/Anbi_Face.png", srvDesc, &gShaderResourceView[1]);
    if (FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        return E_FAIL;
    }

    result = LoadTextureFromFileAndCreateResource(gDevice, L"textures/Anbi_Hair.png", srvDesc, &gShaderResourceView[2]);
    if (FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        return E_FAIL;
    }

    result = LoadTextureFromFileAndCreateResource(gDevice, L"textures/AnbiWeapon_CompanyB_Blade_Fire01.png", srvDesc, &gShaderResourceView[3]);
    if (FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        return E_FAIL;
    }


    gMatWorld1 = XMMatrixIdentity();

    // 초점 대상의 버텍스를 월드로 변환 (임시로 render와 동일한 world TM 사용)
    gIndexOfFocusedVertex = (int)(gVertexCount / (float)2);
    XMFLOAT3 focusPoint = gVertexList[gIndexOfFocusedVertex].Pos;
    XMMATRIX matFocus = XMMatrixIdentity() * XMMatrixScaling(gScaleFactor, gScaleFactor, gScaleFactor);
    XMStoreFloat3(&focusPoint, XMVector3TransformCoord(XMLoadFloat3(&focusPoint), matFocus));

    gCamera.ChangeFocus(focusPoint);

    CBChangeOnResize cbChangeOnResize;
    cbChangeOnResize.mProjection = XMMatrixTranspose(gCamera.GetProjectionMatrix());
    gDeviceContext->UpdateSubresource(gCBChangeOnResize, 0, nullptr, &cbChangeOnResize, 0, 0);


    // 미리 바인드 : 렌더 상태를 바꾸는 것은 역시 비용이 발생하기 때문에 최대한 덜 호출하는 것이 좋다.
    gDeviceContext->VSSetConstantBuffers(0, 1, &gCBCamera);
    gDeviceContext->VSSetConstantBuffers(1, 1, &gCBChangeOnResize);
    gDeviceContext->VSSetConstantBuffers(2, 1, &gCBChangesEveryFrame);

    gDeviceContext->PSSetConstantBuffers(2, 1, &gCBChangesEveryFrame);
    gDeviceContext->PSSetConstantBuffers(3, 1, &gCBLight);


    gDeviceContext->VSSetShader(gVertexShader, nullptr, 0);
    gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);

    gDeviceContext->PSSetSamplers(0, 1, &gSamplerAnisotropic);

    srand(time(NULL));
    return S_OK;
}

void UpdateInput(float deltaTime)
{

    constexpr float speed = 30.0f;

#ifdef USING_MYINPUT
    /*
     *  myinput ver
     */
    int keyboardArrSize = 0;
    bool gKeyboard[256];
    gMyInput->GetKeyboardPressed(gKeyboard, &keyboardArrSize);
    if (gMyInput->GetControlMode() & (uint32)eControlFlags::KEYBOARD_MOVEMENT_MODE)
    {
        if (gKeyboard['W'] || gKeyboard['w'])
        {
            gCamera.RotateAxis(0.0f, XMConvertToRadians(-speed * deltaTime));
        }
        if (gKeyboard['S'] || gKeyboard['s'])
        {
            gCamera.RotateAxis(0.0f, XMConvertToRadians(speed * deltaTime));
        }
        if (gKeyboard['A'] || gKeyboard['a'])
        {
            gCamera.RotateAxis(XMConvertToRadians(-speed * deltaTime), 0.0f);
        }
        if (gKeyboard['D'] || gKeyboard['d'])
        {
            gCamera.RotateAxis(XMConvertToRadians(speed * deltaTime), 0.0f);
        }
    }

    if (gKeyboard['Q'] || gKeyboard['q'])
    {
        gCamera.AddRadiusSphere(speed * deltaTime);
    }
    if (gKeyboard['E'] || gKeyboard['e'])
    {
        gCamera.AddRadiusSphere(-speed * deltaTime);
    }

    // 물체 스케일링
    if (gKeyboard['R'] || gKeyboard['r'])
    {
        gScaleFactor += 0.1f;
        if (gScaleFactor > 15.0f)
        {
            gScaleFactor = 15.0f;
        }
    }

    if (gKeyboard['F'] || gKeyboard['f'])
    {
        gScaleFactor -= 0.1f;
        if (gScaleFactor < 0.1f)
        {
            gScaleFactor = 0.1f;
        }
    }

    if (gKeyboard['C'] || gKeyboard['c'])
    {
        gMyInput->SetControlMode((uint32)eControlFlags::KEYBOARD_MOVEMENT_MODE);
    }

    if (gKeyboard[VK_ESCAPE])
    {
        SendMessage(gWnd, WM_DESTROY, 0, 0);
    }

    /*
     *  카메라 거리 메서드랑 input 클래스 한번 점검해서 라디안 모드로 변경 그리고 그에맞게 시그니처 변경
     *  그리고 키 컨트롤 모드 테스트하기
     *  메인 로직 점검 후, 불필요 코드 과감히 삭제
     */

     ///*
     // *  마우스 움직임은 매끄럽게 처리안되고 있음.
     // */
    int mouseX = 0;
    int mouseY = 0;
    gMyInput->GetMouseDeltaPosition(mouseX, mouseY);

    gCamera.RotateAxis(XMConvertToRadians(mouseX * deltaTime), XMConvertToRadians(mouseY * deltaTime));


#else
    /*
     *  direct input ver
     */

    int keyboardArrSize = 0;
    unsigned char* gKeyboard = gDirectInput->GetKeyboardPress();


    if (!(gDirectInput->GetControlMode() & (uint32)eControlFlags::KEYBOARD_MOVEMENT_MODE))
    {
        int mouseX = 0;
        int mouseY = 0;
        gDirectInput->GetMouseDeltaPosition(mouseX, mouseY);
        gCamera.RotateAxis(XMConvertToRadians(mouseX * deltaTime), XMConvertToRadians(mouseY * deltaTime));
    }
    else
    {
        if (gKeyboard[DIK_W] & 0x80)
        {
            gCamera.RotateAxis(0.0f, XMConvertToRadians(-speed * deltaTime));
        }

        if (gKeyboard[DIK_S] & 0x80)
        {
            gCamera.RotateAxis(0.0f, XMConvertToRadians(speed * deltaTime));
        }
        if (gKeyboard[DIK_A] & 0x80)
        {
            gCamera.RotateAxis(XMConvertToRadians(-speed * deltaTime), 0.0f);
        }

        if (gKeyboard[DIK_D] & 0x80)
        {
            gCamera.RotateAxis(XMConvertToRadians(speed * deltaTime), 0.0f);
        }
    }

    // 마우스 휠 처리 이전에 임시용.
    // 카메라와 물체간의 거리 조절(구체 크기 확대/축소)
    if (gKeyboard[DIK_Q] & 0x80)
    {
        gCamera.AddRadiusSphere(speed * deltaTime);
    }

    if (gKeyboard[DIK_E] & 0x80)
    {
        gCamera.AddRadiusSphere(-speed * deltaTime);
    }

    // 물체 스케일링
    if (gKeyboard[DIK_R] & 0x80)
    {
        gScaleFactor += 0.1f * speed * deltaTime;
        if (gScaleFactor > 15.0f)
        {
            gScaleFactor = 15.0f;
        }
        // 변경된 사이즈에 의해 달라진 초점 위치를 다시 계산한 뒤,
        // 월드공간으로 변환하여 카메라에 전달.
        XMFLOAT3 newFocus;
        XMVECTOR lookAt = XMLoadFloat3(&gVertexList[gIndexOfFocusedVertex].Pos);
        XMMATRIX matFocus = XMMatrixIdentity() * XMMatrixScaling(gScaleFactor, gScaleFactor, gScaleFactor);
        XMStoreFloat3(&newFocus, XMVector3TransformCoord(lookAt, matFocus));

        gCamera.ChangeFocus(newFocus);
    }

    if (gKeyboard[DIK_F] & 0x80)
    {
        gScaleFactor -= 0.1f * speed * deltaTime;
        if (gScaleFactor < 0.2f)
        {
            gScaleFactor = 0.2f;
        }
        XMFLOAT3 newFocus;
        XMVECTOR lookAt = XMLoadFloat3(&gVertexList[gIndexOfFocusedVertex].Pos);
        XMMATRIX matFocus = XMMatrixIdentity() * XMMatrixScaling(gScaleFactor, gScaleFactor, gScaleFactor);
        XMStoreFloat3(&newFocus, XMVector3TransformCoord(lookAt, matFocus));

        gCamera.ChangeFocus(newFocus);
    }

    // 키보드<-> 마우스 조작 전환
    if (gKeyboard[DIK_C] & 0x80)
    {
        gDirectInput->SetControlMode((uint32)eControlFlags::KEYBOARD_MOVEMENT_MODE);
    }

    // 초점 정점 변경
    if (gKeyboard[DIK_H] & 0x80)
    {
        gIndexOfFocusedVertex = rand() % gVertexCount;
        XMFLOAT3 focusPoint = gVertexList[gIndexOfFocusedVertex].Pos;
        XMMATRIX matFocus = XMMatrixIdentity() * XMMatrixScaling(gScaleFactor, gScaleFactor, gScaleFactor);
        XMStoreFloat3(&focusPoint, XMVector3TransformCoord(XMLoadFloat3(&focusPoint), matFocus));

        gCamera.ChangeFocus(focusPoint);
    }


    if (gKeyboard[DIK_ESCAPE] & 0x80)
    {
        SendMessage(gWnd, WM_DESTROY, 0, 0);
    }


#endif

    XMMATRIX matView = gCamera.GetViewMatrix();

    CBCamera camera;
    camera.mView = XMMatrixTranspose(matView);
    gDeviceContext->UpdateSubresource(gCBCamera, 0, nullptr, &camera, 0, 0);

}

HRESULT Render(float deltaTime)
{
    float clearColor[] = { 0.0f, 0.2f, 0.7f, 1.0f };
    gDeviceContext->ClearRenderTargetView(gRenderTargetView, clearColor);
    gDeviceContext->ClearDepthStencilView(gDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Setup our lighting parameters
    static XMFLOAT4 lightDirs(0.0f, 0.577f, -0.550f, 1.0f);
    XMFLOAT4 lightColor(0.1f, 0.1f, 0.1f, 1.0f);

    XMMATRIX matRotateLight = XMMatrixRotationY(deltaTime);
    XMVECTOR vLightDir = XMLoadFloat4(&lightDirs);
    vLightDir = XMVector3Transform(vLightDir, matRotateLight);

    XMStoreFloat4(&lightDirs, vLightDir);

    CBLight cbLight;
    cbLight.vLightColor = lightColor;
    cbLight.vLightDir = lightDirs;

    gDeviceContext->UpdateSubresource(gCBLight, 0, nullptr, &cbLight, 0, 0);

    // arbit fbx model
    gMatWorld1 = XMMatrixIdentity() * XMMatrixScaling(gScaleFactor, gScaleFactor, gScaleFactor);

    CBChangesEveryFrame cbChangesEveryFrame;
    cbChangesEveryFrame.mWorld = XMMatrixTranspose(gMatWorld1);
    cbChangesEveryFrame.mWorldViewProjection = XMMatrixTranspose(gMatWorld1 * gCamera.GetViewMatrix()* gCamera.GetProjectionMatrix());
    gDeviceContext->UpdateSubresource(gCBChangesEveryFrame, 0, nullptr, &cbChangesEveryFrame, 0, 0);

    // 리소스뷰를 어떻게 괜찮은 방법으로 처리할 방법을 검색하기
    gModel->Render(gDeviceContext, gShaderResourceView, NUMBER_TEXTURE);


    gSwapChain->Present(0, 0);
    //디바이스 로스트 처리
    HRESULT result = gDevice->GetDeviceRemovedReason();

    switch (result)
    {
    case S_OK:
        return S_OK;

    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_RESET:

        Cleanup();

        if (FAILED(InitD3DDevice()))
        {
            SendMessage(gWnd, WM_DESTROY, 0, 0);
            return E_FAIL;
        }

        if (FAILED(InitGeometry()))
        {
            SendMessage(gWnd, WM_DESTROY, 0, 0);
            return E_FAIL;
        }

        return S_OK;
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    case DXGI_ERROR_INVALID_CALL:
    default:
        SendMessage(gWnd, WM_DESTROY, 0, 0);
        return E_FAIL;
    }
}

void Cleanup()
{
#ifdef USING_MYINPUT
    gMyInput->Release();
    delete gMyInput;
    gMyInput = nullptr;
#else
    gDirectInput->Release();
    delete gDirectInput;
    gDirectInput = nullptr;
#endif

    gImporter.Release();

    if (gDeviceContext)
    {
        gDeviceContext->ClearState();
    }
    delete[] gVertexList;
    delete[] gIndexList;
    
    delete gModel;

    SAFETY_RELEASE(gSamplerAnisotropic);


    for (size_t textureIndex = 0; textureIndex < NUMBER_TEXTURE; ++textureIndex)
    {
        SAFETY_RELEASE(gShaderResourceView[textureIndex]);
    }

    SAFETY_RELEASE(gCBCamera);
    SAFETY_RELEASE(gCBChangeOnResize);
    SAFETY_RELEASE(gCBChangesEveryFrame);
    SAFETY_RELEASE(gCBLight);

    SAFETY_RELEASE(gIndexBuffer);
    SAFETY_RELEASE(gVertexBuffer);
    SAFETY_RELEASE(gVertexLayout);
    SAFETY_RELEASE(gPixelShader);
    SAFETY_RELEASE(gVertexShader);
    SAFETY_RELEASE(gDepthStencil);
    SAFETY_RELEASE(gDepthStencilView);
    SAFETY_RELEASE(gRenderTargetView);
    SAFETY_RELEASE(gSwapChain);
    SAFETY_RELEASE(gDeviceContext);
    SAFETY_RELEASE(gDevice);
}
