#include "Application.h"
#include <string>
#include "Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/Importer/ModelImporter.h"
#include "Renderer/Primitive/Plane.h"
#include "Renderer/Resources/Model.h"
#include "Scene/Camera.h"
#include "Scene/Floor.h"
#include "Scene/Light.h"
#include "Scene/Sky.h"
#include "Util/Macro.h"


Application::Application()
    : mWindowWidth(1280)
    , mWindowHeight(720)
    , mAppFrameRate(120)
    , mWindow(nullptr)
    , mImporter(nullptr)
    , mCharacter(nullptr)
    , mCamera(nullptr)
    , mSkybox(nullptr)
    , mLight(nullptr)
    , mPlane(nullptr)
    , mFloor(nullptr)
    , mDirectInput(nullptr)
{}

Application::~Application()
{
    mDirectInput->Release();
    delete mDirectInput;
    mDirectInput = nullptr;
    delete mFloor;
    delete mPlane;
    //   gImporter->Release();
    delete mSkybox;
    delete mImporter;
    delete mLight;
    delete mCharacter;
    delete mCamera;

    delete mWindow;
    // MEMO device가 가장 마지막에 해제되도록.
    renderer::Renderer::GetInstance()->Release();
#ifdef _DEBUG
    renderer::Renderer::CheckLiveObjects();
#endif
}

bool Application::InitializeWithWindows(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int32_t nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    mWindow = new Window(hInstance);

    mWindow->RegisterWindowClass();

    const HWND handleWindow = mWindow->MakeWindow(mWindowWidth, mWindowHeight);
    if(!handleWindow)
    {
        return false;
    }
 
    mWindow->DisplayWindow(nCmdShow);
    mWindow->RefreshWindow();

    // 백버퍼와 프론트 버퍼를 스왑하는 방식, 백버퍼에서 프론트로 카피하는 방식 두 개가 존재함.
    DXGI_SWAP_CHAIN_DESC swapDesc;
    ZeroMemory(&swapDesc, sizeof(swapDesc));
    swapDesc.BufferCount = 1;
    swapDesc.BufferDesc.Width = mWindowWidth;
    swapDesc.BufferDesc.Height = mWindowHeight;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferDesc.RefreshRate.Numerator = mAppFrameRate;
    swapDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = handleWindow;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.SampleDesc.Quality = 0;
    swapDesc.Windowed = TRUE;

    if (FAILED(renderer::Renderer::GetInstance()->CreateDeviceAndSetup(
            swapDesc, handleWindow, mWindowHeight, mWindowWidth, true)))
    {
        ASSERT(false, "모델데이터 초기화 실패 SetupGeometry");
        return false;
    }

    if (FAILED(renderer::Renderer::GetInstance()->PrepareRender()))
    {
        ASSERT(false, "Fail to initialize shaders");
        return false;
    }

    mDirectInput = new core::DirectInput(hInstance, handleWindow, mWindowWidth,
                                         mWindowHeight);
    if (FAILED(mDirectInput->Initialize()))
    {
        ASSERT(false, "모델데이터 초기화 실패 SetupGeometry");
        return false;
    }

    initializeScene();

    return true;
}

void Application::Run()
{
    const float RenderIntervalTime = 1000.0f / static_cast<float>(mAppFrameRate);

    core::Timer::Tick();
    // MEMO: 첫 프레임이 안정적으로 돌도록 함. 프로그램 내에서 FrameTime이 일관되도록
    double lastFrameTime = core::Timer::GetNowMS();
    double lastFPSTime = core::Timer::GetNowMS();
    int16_t frameCount = 0;
    while (true)
    {
        if(mWindow->ProcessMessages())
        {
            break;
        }

        core::Timer::Tick();

        const double startTime = core::Timer::GetNowMS();
        const double deltaTime = startTime - lastFrameTime;

        if (RenderIntervalTime >= deltaTime)
        {
            YieldProcessor();
            continue;
        }

        lastFrameTime = startTime;

        mDirectInput->UpdateInput();

        updateScene(deltaTime);

        preprocess();
        renderScene();

        ++frameCount;
        if (startTime - lastFPSTime >= 1000.0)
        {
            OutputDebugString(L"Render FPS : ");
            OutputDebugString(std::to_wstring(frameCount).c_str());
            OutputDebugString(L"\n");
            lastFPSTime += 1000.0;
            frameCount = 0;
        }
    }
}

bool Application::initializeScene()
{
    core::Timer::Initialize();

    mCamera = new scene::Camera(
        XMVectorSet(0.0f, 10.0f, -15.0f, 0.0f)
        , XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f)
        , XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
        mWindowWidth,
        mWindowHeight);

    mImporter = new renderer::ModelImporter();
    mImporter->Initialize();

    mCharacter = new renderer::Model(mCamera, reinterpret_cast<int8_t*>("/AssetData/models/unagi.fbx"));

    // TODO: 로드 시점과 처리에 대한 내용도 고민이 필요함. (모델이 로드를 요청할 것인지? - 요청하면 언제 로드되었음을 확인하고 처리할 것인지? - Importer와는 hash id로 통신)
    mImporter->LoadFbxModel("/AssetData/models/unagi.fbx", renderer::Renderer::GetInstance());

    mSkybox = new scene::Sky(*mCamera);
    mSkybox->Initialize(10, 10);

    // TODO: 실패시 프로그램 종료말고, 나중에 Object List를 만들어서 관리하도록 변경(성공하면 drawable 리스트에 추가)
    if (FAILED(mCharacter->SetupMesh(*mImporter)))
    {
        ASSERT(false, "gCharacter::SetupMesh 모델데이터 혹은 D3D개체 초기화 실패 _ could not initialize mesh or d3d obj");
        return false;
    }

    renderer::Renderer::GetInstance()->SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCamera->ChangeFocus(mCharacter->GetCenterPoint());
    // MEMO Light 위치값 막 바꾸면 안됨. 그림자 제대로 안그려질 수 있음. 나중에 개선해야 할 항목 중 하나(cascade)
  //  gLight = new Light(XMFLOAT3(0.0f, 50.0f, 70.0f), gCharacter->GetCenterPoint(), XMFLOAT3(1.0f, 1.0f, 1.0f), gCamera, 0.1f, 300.0f);

    mLight = new scene::Light(XMFLOAT3(0.0f, 20.0f, 50.0f), mCharacter->GetCenterPoint(), XMFLOAT3(1.0f, 1.0f, 1.0f), mCamera, 0.1f, 500.0f);
    mLight->Initialize();
    mLight->SetupCascade();
    mCharacter->SetLight(mLight);

    // debug quad
    // 깊이 텍스쳐 확인용
    mPlane = new renderer::Plane();
    mPlane->SetPosition(XMFLOAT3(0.0, 0.0, -1.0));

    mFloor = new scene::Floor(XMFLOAT2(0.0f, 0.0f), 2, 10, 10);

    return true;
}

void Application::updateScene(double deltaTime)
{
    float speed = 10.0f;

    /*
     *  direct input ver
     */

    unsigned char* gKeyboard = mDirectInput->GetKeyboardPress();

    if (!(mDirectInput->GetControlMode() & (uint32)core::eControlFlags::KEYBOARD_MOVEMENT_MODE))
    {
        int mouseX = 0;
        int mouseY = 0;
        speed = 0.5f;
        mDirectInput->GetMouseDeltaPosition(mouseX, mouseY);
        if (!(mouseX == 0 && mouseY == 0))
        {
            mCamera->RotateAxis(XMConvertToRadians(static_cast<float>(mouseX)) * deltaTime * speed, XMConvertToRadians(static_cast<float>(mouseY)) * deltaTime * speed);
        }
    }
    else
    {
        if (gKeyboard[DIK_W] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(-(speed * deltaTime)));
        }

        if (gKeyboard[DIK_S] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(speed * deltaTime));
        }
        if (gKeyboard[DIK_A] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(-(speed * deltaTime)), 0.0f);
        }

        if (gKeyboard[DIK_D] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(speed * deltaTime), 0.0f);
        }
    }

    // 마우스 휠 처리 이전에 임시용.
    // 카메라와 물체간의 거리 조절(구체 크기 확대/축소)
    if (gKeyboard[DIK_Q] & 0x80)
    {
        mCamera->AddRadiusSphere(deltaTime);
        // gLight->Move(0.001f, -1.0f);
       //  gLight->SetDirection(gCharacter->GetCenterPoint());

    }

    if (gKeyboard[DIK_E] & 0x80)
    {
        mCamera->AddRadiusSphere(-deltaTime);
        //     gLight->Move(-0.001f, 1.0f);
         //    gLight->SetDirection(gCharacter->GetCenterPoint());
    }

    // 키보드<-> 마우스 조작 전환
    static bool bPressKey = false;
    if (!(gKeyboard[DIK_C] & 0x80) && bPressKey)
    {
        mDirectInput->SetControlMode((uint32)core::eControlFlags::KEYBOARD_MOVEMENT_MODE);
    }
    bPressKey = gKeyboard[DIK_C] & 0x80;

    static bool bPressHKey = false;
    static bool bHightlight = false;
    if (!(gKeyboard[DIK_H] & 0x80) && bPressHKey)
    {
        bHightlight = !bHightlight;
        mCharacter->SetHighlight(bHightlight);
    }
    bPressHKey = gKeyboard[DIK_H] & 0x80;

    if (gKeyboard[DIK_Z] & 0x80)
    {
        mCamera->AddHeight(-deltaTime);
    }

    if (gKeyboard[DIK_X] & 0x80)
    {
        mCamera->AddHeight(deltaTime);
    }

    if (gKeyboard[DIK_ESCAPE] & 0x80)
    {
        SendMessage(mWindow->GetHandle(), WM_DESTROY, 0, 0);
    }


    renderer::Renderer::CbViewProj cbViewProj;
    cbViewProj.Matrix = XMMatrixTranspose(mCamera->GetViewProjectionMatrix());
    renderer::Renderer::GetInstance()->UpdateCB(renderer::Renderer::eCbType::CbViewProj, &cbViewProj);


    mLight->SetupCascade();
}

void Application::preprocess()
{
    renderer::Renderer::GetInstance()->SetRenderTargetTo(renderer::Renderer::eRenderTarget::Shadow);
    renderer::Renderer::GetInstance()->SetViewport(false);
    renderer::Renderer::GetInstance()->ClearScreenAndDepth(renderer::Renderer::eRenderTarget::Shadow);
    mCharacter->Update();
    mCharacter->DrawShadow();
}

void Application::renderScene()
{
    renderer::Renderer::GetInstance()->SetRenderTargetTo(renderer::Renderer::eRenderTarget::Default);
    renderer::Renderer::GetInstance()->SetViewport(true);
    renderer::Renderer::GetInstance()->ClearScreenAndDepth(renderer::Renderer::eRenderTarget::Default);

    // 리소스뷰를 어떻게 괜찮은 방법으로 처리할 방법을 검색하기

    // TODO: 지금은 WorldCB를 공유하여 Update함수를 같이 붙여둬야 하지만, 나중에 렌더큐를 가면 내부적으로 자동으로 업데이트 되게끔 처리해보기
    mSkybox->Update();
    mSkybox->Draw();

    // gFloor->Draw();
    mFloor->Draw();
    mCharacter->Update();
    mCharacter->Draw();

    mLight->Update(mCamera);
    mLight->Draw();
    //mLight->DrawDebug();

    mPlane->Update();
    //gPlane->DrawTexture(gLight);
    mPlane->DrawTexture(mLight);
    renderer::Renderer::GetInstance()->Present();
}
