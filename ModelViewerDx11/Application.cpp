#include "Application.h"
#include <string>
#include "Window.h"
#include "Renderer/Renderer.h"
#include "Renderer/Importer/ModelImporter.h"
#include "Renderer/Primitive/MeshGenerator.h"
#include "Renderer/Primitive/Plane.h"
#include "Renderer/Resources/BufferManager.h"
#include "Renderer/Resources/Model.h"
#include "Renderer/Resources/ResourceManager.h"
#include "Renderer/Resources/TextureManager.h"
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
    , mRenderer(nullptr)
    , mImporter(nullptr)
    , mCharacter(nullptr)
    , mCamera(nullptr)
    , mSkybox(nullptr)
    , mLight(nullptr)
    , mPlane(nullptr)
    , mFloor(nullptr)
    , mBufferManager(nullptr)
    , mTextureManager(nullptr)
    , mResourceManager(nullptr)
    , mDirectInput(nullptr)
{
    mRenderer = new renderer::Renderer();
    mImporter = new renderer::ModelImporter();
}

Application::~Application()
{
    mDirectInput->Release();
    delete mDirectInput;
    mDirectInput = nullptr;
    delete mFloor;
    delete mPlane;
    delete mSkybox;
    delete mImporter;
    delete mLight;
    delete mCharacter;
    delete mCamera;

    delete mWindow;
    delete mBufferManager;
    delete mTextureManager;
    delete mResourceManager;
    delete mRenderer;
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

    
    if (FAILED(mRenderer->initialize(handleWindow, mWindowWidth, mWindowHeight, mAppFrameRate)))
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
    mImporter->Initialize();

    if(!initializeManagers())
    {
        ASSERT(false, "fail to initialize managers");
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
    bool bReinitDevice = false;
    while (true)
    {
        if (bReinitDevice)
        {
            mRenderer->Cleanup();
            mRenderer->initialize(mWindow->GetHandle(), mWindowWidth, mWindowHeight, mAppFrameRate);
            bReinitDevice = false;
        }

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

        if (mRenderer->CheckDeviceLost(bReinitDevice))
        {
            break;
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




    const int8_t* const modelFilePath = reinterpret_cast<int8_t*>("/AssetData/models/unagi.fbx");
    // TODO: 나중에 Object List를 만들어서 관리하도록 변경(성공하면 drawable 리스트에 추가)
    mCharacter = new renderer::Model(mCamera, mBufferManager);

    mResourceManager->LoadModel(modelFilePath, mCharacter);

    mSkybox = new scene::Sky(*mCamera);
    mSkybox->Initialize(10, 10, mTextureManager);

    mRenderer->BindPrimitiveTopologyTo(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCamera->ChangeFocus(mCharacter->GetCenterPoint(), *mRenderer);
    // MEMO Light 위치값 막 바꾸면 안됨. 그림자 제대로 안그려질 수 있음. 나중에 개선해야 할 항목 중 하나(cascade)
  //  gLight = new Light(XMFLOAT3(0.0f, 50.0f, 70.0f), gCharacter->GetCenterPoint(), XMFLOAT3(1.0f, 1.0f, 1.0f), gCamera, 0.1f, 300.0f);

    mLight = new scene::Light(XMFLOAT3(0.0f, 20.0f, 50.0f), mCharacter->GetCenterPoint(), XMFLOAT3(1.0f, 1.0f, 1.0f), mCamera, 0.1f, 500.0f, *mRenderer);
    mLight->Initialize(mTextureManager, *mRenderer);
    mLight->SetupCascade(*mRenderer);

    // debug quad
    // 깊이 텍스쳐 확인용
    mPlane = new renderer::Plane();
    const XMFLOAT3 planePosition(0.0, 0.0, -1.0);
    const XMFLOAT3 planeScale(0.5f, 0.5f, 0.5f);
    mPlane->SetPosition(planePosition);
    mPlane->SetScale(planeScale);

    mFloor = new scene::Floor(XMFLOAT2(0.0f, 0.0f), 2, 10, 10);

    return true;
}

bool Application::initializeManagers()
{
    ID3D11Device* device = mRenderer->GetDevice();
    ID3D11DeviceContext* deviceContext = mRenderer->GetDeviceContext();
    mBufferManager = new renderer::BufferManager(device, deviceContext, renderer::BufferManager::eIndexListFormat::UInt32);
    if (!mBufferManager->Initialize(renderer::BufferManager::sVertexBufferDefaultSize, renderer::BufferManager::sIndexBufferDefaultSize, renderer::BufferManager::sVertexBufferDefaultSize, renderer::BufferManager::sIndexBufferDefaultSize))
    {
        ASSERT(false, "buffer manager init failed.")
        return false;
    }

    mTextureManager = new renderer::TextureManager(device);
    mResourceManager = new renderer::ResourceManager(device, mTextureManager, mImporter, mBufferManager);

    mRenderer->SetManagers(mBufferManager, mTextureManager);
    renderer::MeshGenerator::Initialize(mBufferManager);
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
            mCamera->RotateAxis(XMConvertToRadians(static_cast<float>(mouseX)) * deltaTime * speed, XMConvertToRadians(static_cast<float>(mouseY)) * deltaTime * speed, *mRenderer);
        }
    }
    else
    {
        if (gKeyboard[DIK_W] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(-(speed * deltaTime)), *mRenderer);
        }

        if (gKeyboard[DIK_S] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(speed * deltaTime), *mRenderer);
        }
        if (gKeyboard[DIK_A] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(-(speed * deltaTime)), 0.0f, *mRenderer);
        }

        if (gKeyboard[DIK_D] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(speed * deltaTime), 0.0f, *mRenderer);
        }
    }

    // 마우스 휠 처리 이전에 임시용.
    // 카메라와 물체간의 거리 조절(구체 크기 확대/축소)
    if (gKeyboard[DIK_Q] & 0x80)
    {
        mCamera->AddRadiusSphere(deltaTime, *mRenderer);
    }

    if (gKeyboard[DIK_E] & 0x80)
    {
        mCamera->AddRadiusSphere(-deltaTime, *mRenderer);
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
        mCamera->AddHeight(-deltaTime, *mRenderer);
    }

    if (gKeyboard[DIK_X] & 0x80)
    {
        mCamera->AddHeight(deltaTime, *mRenderer);
    }

    if (gKeyboard[DIK_ESCAPE] & 0x80)
    {
        SendMessage(mWindow->GetHandle(), WM_DESTROY, 0, 0);
    }


    renderer::CbViewProj cbViewProj;
    cbViewProj.Matrix = XMMatrixTranspose(mCamera->GetViewProjectionMatrix());
    mRenderer->UpdateCB(renderer::eCbType::CbViewProj, &cbViewProj);


    mLight->SetupCascade(*mRenderer);
}

void Application::preprocess()
{
    mRenderer->BindRenderTargetTo(renderer::eRenderTarget::Shadow);
    mRenderer->SetViewport(false);
    mRenderer->ClearScreenAndDepth(renderer::eRenderTarget::Shadow);
    mCharacter->Update(*mRenderer);
    mCharacter->DrawShadow(*mRenderer);
}

void Application::renderScene()
{
    mBufferManager->MarkInvalidateDynamicBuf();
    mRenderer->BindRenderTargetTo(renderer::eRenderTarget::Default);
    mRenderer->SetViewport(true);
    mRenderer->ClearScreenAndDepth(renderer::eRenderTarget::Default);


    // TODO: 지금은 WorldCB를 공유하여 Update함수를 같이 붙여둬야 하지만, 나중에 렌더큐를 가면 내부적으로 자동으로 업데이트 되게끔 처리해보기
    mSkybox->Update(*mRenderer);
    mSkybox->Draw(*mRenderer);

    mFloor->Draw(*mRenderer);
    mCharacter->Update(*mRenderer);
    mCharacter->Draw(*mRenderer);

    mLight->Update(*mRenderer);
    mLight->Draw(*mRenderer);
    mLight->Update(*mRenderer);
    mLight->DrawDebug(*mRenderer);

    mPlane->Update();
    mPlane->DrawTexture(*mRenderer);
    mRenderer->Present();
}
