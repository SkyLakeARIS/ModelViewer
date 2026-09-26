#include "Application.h"
#include <algorithm>
#include <string>
#include "Window.h"
#include "Core/Input.h"
#include "Core/Timer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Importer/ModelImporter.h"
#include "Renderer/Primitive/MeshGenerator.h"
#include "Renderer/Resources/BufferManager.h"
#include "Renderer/Resources/Model.h"
#include "Renderer/Resources/ResourceManager.h"
#include "Renderer/Resources/TextureManager.h"
#include "Renderer/Shader/ShaderManager.h"
#include "Scene/Billboard.h"
#include "Scene/Camera.h"
#include "Scene/Floor.h"
#include "Scene/Light.h"
#include "Scene/Sky.h"
#include "UI/DebugPanel.h"
#include "Util/Macro.h"



Application::Application()
    : mWindowWidth(1280)
    , mWindowHeight(720)
    , mAppFrameRate(120)
    , mWindow(nullptr)
    , mNdcMeshDeferred{}
    , mCurSubMeshIndexFocusModel(0)
    , mCommandCache()
    , mRenderer(nullptr)
    , mVkRenderer(nullptr)
    , mImporter(nullptr)
    , mCharacter(nullptr)
    , mCamera(nullptr)
    , mSkybox(nullptr)
    , mLight(nullptr)
    , mLightIcon(nullptr)
    , mFloor(nullptr)
    , mBufferManager(nullptr)
    , mTextureManager(nullptr)
    , mResourceManager(nullptr)
    , mShaderManager(nullptr)
    , mDirectInput(nullptr)
    , mShadowDebugPanel(nullptr)
    , mGBufferColorDebugPanel(nullptr)
    , mGBufferNormalDebugPanel(nullptr)
    , mGBufferPositionDebugPanel(nullptr)
    , mGBufferSpecularDebugPanel(nullptr)
    , mGBufferAmbientDebugPanel(nullptr)
{
    mImporter = new renderer::ModelImporter();
    mCommandList.reserve(64);
}

Application::~Application()
{
    std::vector<renderer::RenderPacket>().swap(mCommandList);
    delete mShadowDebugPanel;
    delete mGBufferColorDebugPanel;
    delete mGBufferNormalDebugPanel;
    delete mGBufferPositionDebugPanel;
    delete mGBufferSpecularDebugPanel;
    delete mGBufferAmbientDebugPanel;
    mDirectInput->Release();
    delete mDirectInput;
    mDirectInput = nullptr;
    delete mFloor;
    delete mSkybox;
    delete mImporter;
    delete mLight;
    delete mLightIcon;
    delete mCharacter;
    delete mCamera;

    delete mWindow;
    delete mBufferManager;
    delete mTextureManager;
    delete mResourceManager;
    delete mShaderManager;
    delete mRenderer;
    delete mVkRenderer;
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

    mRenderer = new renderer::Renderer();
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

bool Application::InitializeWithWindowsVk(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine,
    int32_t nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    mWindow = new Window(hInstance);

    mWindow->RegisterWindowClass();

    const HWND handleWindow = mWindow->MakeWindow(mWindowWidth, mWindowHeight);
    if (!handleWindow)
    {
        return false;
    }

    mWindow->DisplayWindow(nCmdShow);
    mWindow->RefreshWindow();

    mVkRenderer = new renderer::VkRenderer();
    if (FAILED(mVkRenderer->initializeWithVulkan(handleWindow, hInstance, mWindowWidth, mWindowHeight, mWindow->GetAppNameA())))
    {
        ASSERT(false, "Fail to initialize shaders");
        return false;
    }

    mDirectInput = new core::DirectInput(hInstance, handleWindow, mWindowWidth, mWindowHeight);
    if (FAILED(mDirectInput->Initialize()))
    {
        ASSERT(false, "모델데이터 초기화 실패 SetupGeometry");
        return false;
    }
    mImporter->Initialize();

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
        double deltaTime = startTime - lastFrameTime;
        // MEMO: 디버거 대응. deltaTime이 너무 크면 시간을 재조정한다.
        if(deltaTime > 100.0)
        {
            deltaTime = RenderIntervalTime;
        }

        if (RenderIntervalTime > deltaTime)
        {
            YieldProcessor();
            continue;
        }

        lastFrameTime = startTime;

        processInput(deltaTime);
        updateScene();

        renderScene();
        mRenderer->Present();

        mBufferManager->MarkInvalidateDynamicBuf();

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

void Application::RunWithVulkan()
{

    if(!initializeManagersVulkan())
    {
        return;
    }

    if(!initializeSceneVulkan())
    {
        return;
    }

    const float RenderIntervalTime = 1000.0f / static_cast<float>(mAppFrameRate);

    core::Timer::Tick();
    // MEMO: 첫 프레임이 안정적으로 돌도록 함. 프로그램 내에서 FrameTime이 일관되도록
    double lastFrameTime = core::Timer::GetNowMS();
    double lastFPSTime = core::Timer::GetNowMS();
    int16_t frameCount = 0;
    while (true)
    {
        if (mWindow->ProcessMessages())
        {
            break;
        }

        core::Timer::Tick();

        const double startTime = core::Timer::GetNowMS();
        double deltaTime = startTime - lastFrameTime;
        // MEMO: 디버거 대응. deltaTime이 너무 크면 시간을 재조정한다.
        if (deltaTime > 100.0)
        {
            deltaTime = RenderIntervalTime;
        }

        if (RenderIntervalTime > deltaTime)
        {
            YieldProcessor();
            continue;
        }

        lastFrameTime = startTime;

        processInputVulkan(deltaTime);
        updateSceneVulkan();

        renderSceneVulkan();

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




    const int8_t* const modelFilePath = reinterpret_cast<int8_t*>("/AssetData/models/unagi.fbx");
    mCharacter = new renderer::Model(mCamera, mBufferManager);

    mResourceManager->LoadModel(modelFilePath, mCharacter);

    mSkybox = new scene::Sky();
    mSkybox->Initialize(10, 10, mTextureManager);

    mRenderer->BindPrimitiveTopologyByType(renderer::ePrimitiveTopology::Triangles);
    mCamera->ChangeFocus(mCharacter->GetCenterPoint(0));
    // MEMO Light 위치값 막 바꾸면 안됨. 그림자 제대로 안그려질 수 있음. 나중에 개선해야 할 항목 중 하나(cascade)
  //  gLight = new Light(XMFLOAT3(0.0f, 50.0f, 70.0f), gCharacter->GetCenterPoint(), XMFLOAT3(1.0f, 1.0f, 1.0f), gCamera, 0.1f, 300.0f);

    const XMFLOAT3 lightPosition = XMFLOAT3(30.0f, 20.0f, 50.0f);
    const XMFLOAT3 lightLookAt = mCharacter->GetCenterPoint(0);
    const XMVECTOR vLightPosition = XMLoadFloat3(&lightPosition);
    const XMVECTOR vLightLookAt = XMLoadFloat3(&lightLookAt);
    XMFLOAT3 lightDir;
    XMStoreFloat3(&lightDir, vLightLookAt - vLightPosition);
    mLight = new scene::Light(lightPosition, lightDir, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.1f, 500.0f);
    mLight->Update(*mRenderer);

    mFloor = new scene::Floor(XMFLOAT2(0.0f, 0.0f), 2, 10, 10);

    mShadowDebugPanel = new ui::DebugPanel(0, 0, 200, 200);
    const int16_t shadowTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sShadowTexHash);
    mShadowDebugPanel->SetDebugType(renderer::TextureManager::sShadowTexHash, shadowTexSerial);

    mGBufferColorDebugPanel = new ui::DebugPanel(200, 0, 200, 200);
    const int16_t gBufferColorTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sGBufferColorTexHash);
    mGBufferColorDebugPanel->SetDebugType(renderer::TextureManager::sGBufferColorTexHash, gBufferColorTexSerial);

    mGBufferNormalDebugPanel = new ui::DebugPanel(400, 0, 200, 200);
    const int16_t gBufferNormalTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sGBufferNormalTexHash);
    mGBufferNormalDebugPanel->SetDebugType(renderer::TextureManager::sGBufferNormalTexHash, gBufferNormalTexSerial);

    mGBufferPositionDebugPanel = new ui::DebugPanel(600, 0, 200, 200);
    const int16_t gBufferDepthTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sGBufferPositionTexHash);
    mGBufferPositionDebugPanel->SetDebugType(renderer::TextureManager::sGBufferPositionTexHash, gBufferDepthTexSerial);

    mGBufferSpecularDebugPanel= new ui::DebugPanel(800, 0, 200, 200);
    const int16_t gBufferSpecularTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sGBufferSpecularTexHash);
    mGBufferSpecularDebugPanel->SetDebugType(renderer::TextureManager::sGBufferSpecularTexHash, gBufferSpecularTexSerial);

    mGBufferAmbientDebugPanel= new ui::DebugPanel(1000, 0, 200, 200);
    const int16_t gBufferAmbientTexSerial = mTextureManager->GetTextureSerial(renderer::TextureManager::sGBufferAmbientTexHash);
    mGBufferAmbientDebugPanel->SetDebugType(renderer::TextureManager::sGBufferAmbientTexHash, gBufferAmbientTexSerial);

    mLightIcon = new scene::Billboard();
    mLightIcon->Initialize();

    const int8_t* const filePath = reinterpret_cast<const int8_t*>("./AssetData/textures/lightIcon.png");
    HashID lightIconTexID = 0;
    int16_t lightIconSerialID = 0;
    mTextureManager->AddTexture(filePath, lightIconTexID);
    lightIconSerialID = mTextureManager->GetTextureSerial(lightIconTexID);
    ASSERT(lightIconTexID, "icon texture fail to add");
    ASSERT(lightIconSerialID >= 0, "icon texture 의 serial 값을 얻어오지 못함.");

    mLightIcon->SetTexture(lightIconTexID, lightIconSerialID);

    renderer::MeshGenerator::CreateNdcPlane(mNdcMeshDeferred);
    renderer:renderer::SubMesh& subMeshNdcPlane = mNdcMeshDeferred.SubMeshes.front();
    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::Shadow)] = renderer::TextureManager::sShadowTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::Shadow)] = renderer::TextureManager::sShadowTexSerialID;

    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::GBufferColor)] = renderer::TextureManager::sGBufferColorTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::GBufferColor)] = renderer::TextureManager::sGBufferColorTexSerialID;

    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::GBufferNormal)] = renderer::TextureManager::sGBufferNormalTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::GBufferNormal)] = renderer::TextureManager::sGBufferNormalTexSerialID;

    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::GBufferPosition)] = renderer::TextureManager::sGBufferPositionTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::GBufferPosition)] = renderer::TextureManager::sGBufferPositionTexSerialID;

    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::GBufferSpecular)] = renderer::TextureManager::sGBufferSpecularTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::GBufferSpecular)] = renderer::TextureManager::sGBufferSpecularTexSerialID;

    subMeshNdcPlane.Material.TextureHashes[static_cast<uint8_t>(renderer::eTextureType::GBufferAmbient)] = renderer::TextureManager::sGBufferAmbientTexHash;
    subMeshNdcPlane.Material.TextureSerials[static_cast<uint8_t>(renderer::eTextureType::GBufferAmbient)] = renderer::TextureManager::sGBufferAmbientTexSerialID;
    return true;
}

bool Application::initializeManagers()
{
    ID3D11Device* device = mRenderer->GetDevice();
    ID3D11DeviceContext* deviceContext = mRenderer->GetDeviceContext();

    mShaderManager = new renderer::ShaderManager(*device, *deviceContext);
    if (!mShaderManager->CreatePresetConstantBuffers())
    {
        ASSERT(false, "CB 초기화 실패")
        return false;
    }

    if (!mShaderManager->SetupShaders())
    {
        ASSERT(false, "셰이더 초기화 실패")
        return false;
    }


    mBufferManager = new renderer::BufferManager(device, deviceContext, renderer::BufferManager::eIndexListFormat::UInt32);
    if (!mBufferManager->Initialize(renderer::BufferManager::sVertexBufferDefaultSize, renderer::BufferManager::sIndexBufferDefaultSize, renderer::BufferManager::sVertexBufferDefaultSize, renderer::BufferManager::sIndexBufferDefaultSize))
    {
        ASSERT(false, "buffer manager init failed.")
        return false;
    }

    mTextureManager = new renderer::TextureManager(device);
    mTextureManager->InitDefaultTexture();

    mResourceManager = new renderer::ResourceManager(device, mTextureManager, mImporter, mBufferManager);

    mRenderer->SetManagers(mBufferManager, mTextureManager, mShaderManager);
    renderer::MeshGenerator::Initialize(mBufferManager);
    return true;
}

bool Application::initializeSceneVulkan()
{
    core::Timer::Initialize();

    mCamera = new scene::Camera(
        XMVectorSet(0.0f, 10.0f, -15.0f, 0.0f)
        , XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f)
        , XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
        mWindowWidth,
        mWindowHeight);

    return true;
}

bool Application::initializeManagersVulkan()
{
    return true;
}

void Application::processInput(double deltaTime)
{
    mDirectInput->UpdateInput();

    /*
     *  direct input ver
     */

    const unsigned char* gKeyboard = mDirectInput->GetKeyboardPress();

    if (!(mDirectInput->GetControlMode() & static_cast<uint32_t>(core::eControlFlags::KEYBOARD_MOVEMENT_MODE)))
    {
        int mouseX = 0;
        int mouseY = 0;
        mDirectInput->GetMouseDeltaPosition(mouseX, mouseY);
        if (!(mouseX == 0 && mouseY == 0))
        {
            constexpr float KEYBOARD_SPEED = 0.1f;
            mCamera->RotateAxis(XMConvertToRadians(static_cast<float>(mouseX)) * static_cast<float>(deltaTime) * KEYBOARD_SPEED, XMConvertToRadians(static_cast<float>(mouseY)) * static_cast<float>(deltaTime) * KEYBOARD_SPEED);
        }
    }
    else
    {
        constexpr float MOUSE_SPEED = 10.0f;
        if (gKeyboard[DIK_W] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(-(MOUSE_SPEED * static_cast<float>(deltaTime))));
        }

        if (gKeyboard[DIK_S] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(MOUSE_SPEED * static_cast<float>(deltaTime)));
        }
        if (gKeyboard[DIK_A] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(-(MOUSE_SPEED * static_cast<float>(deltaTime))), 0.0f);
        }

        if (gKeyboard[DIK_D] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(MOUSE_SPEED * static_cast<float>(deltaTime)), 0.0f);
        }
    }

    static bool bPressFKey = false;
    if (!(gKeyboard[DIK_F] & 0x80 )&& bPressFKey)
    {
        // MEMO: 시스템을 어떻게 짜둘까? 매니저를 빨리 만들어야 할듯하다.
        const int32_t curSubMeshCountFocusModel = mCharacter->GetSubMeshCount();
        if(curSubMeshCountFocusModel > 0)
        {
            mCurSubMeshIndexFocusModel = (mCurSubMeshIndexFocusModel + 1) % curSubMeshCountFocusModel;
        }
        const XMFLOAT3 centerPoint = mCharacter->GetCenterPoint(mCurSubMeshIndexFocusModel);
        mCamera->ChangeFocus(centerPoint);
    }
    bPressFKey = gKeyboard[DIK_F] & 0x80;

    // 마우스 휠 처리 이전에 임시용.
    // 카메라와 물체간의 거리 조절(구체 크기 확대/축소)
    constexpr float MOVEMENT_SPEED = 0.01f;
    if (gKeyboard[DIK_Q] & 0x80)
    {
        mCamera->AddRadiusSphere(static_cast<float>(deltaTime * MOVEMENT_SPEED));
    }

    if (gKeyboard[DIK_E] & 0x80)
    {
        mCamera->AddRadiusSphere(static_cast<float>(-deltaTime * MOVEMENT_SPEED));
    }

    // 키보드<-> 마우스 조작 전환
    static bool bPressCKey = false;
    if (!(gKeyboard[DIK_C] & 0x80) && bPressCKey)
    {
        mDirectInput->SetControlMode(static_cast<uint32_t>(core::eControlFlags::KEYBOARD_MOVEMENT_MODE));
    }
    bPressCKey = gKeyboard[DIK_C] & 0x80;

    static bool bPressHKey = false;
    static bool bHighlight = false;
    if (!(gKeyboard[DIK_H] & 0x80) && bPressHKey)
    {
        bHighlight = !bHighlight;
        mCharacter->SetHighlight(bHighlight);
    }
    bPressHKey = gKeyboard[DIK_H] & 0x80;

    if (gKeyboard[DIK_Z] & 0x80)
    {
        mCamera->AddHeight(static_cast<float>(-deltaTime) * MOVEMENT_SPEED);
    }

    if (gKeyboard[DIK_X] & 0x80)
    {
        mCamera->AddHeight(static_cast<float>(deltaTime) * MOVEMENT_SPEED);
    }

    if (gKeyboard[DIK_ESCAPE] & 0x80)
    {
        SendMessage(mWindow->GetHandle(), WM_DESTROY, 0, 0);
    }
}

void Application::updateScene()
{
    // MEMO: 이후 작업들이 카메라 정보에 의존하므로 카메라를 먼저 업데이트
    mCamera->Update();

    // MEMO: Renderer가 예약한 CB들 업로드
    renderer::CbViewProj cbViewProj;
    cbViewProj.Matrix = XMMatrixTranspose(mCamera->GetViewProjectionMatrix());
    mShaderManager->UpdateCB(renderer::eCbType::CbViewProj, &cbViewProj);

    renderer::CbCameraPosition cbCameraPos = {};
    cbCameraPos.Float3 = mCamera->GetEye();
    mShaderManager->UpdateCB(renderer::eCbType::CbCameraPosition, &cbCameraPos);

    mLight->Update(*mRenderer);
    renderer::CbLightViewProjMatrix cbLightVpMat;
    cbLightVpMat.Matrix = XMMatrixTranspose(mLight->GetViewProjMatrix());
    mShaderManager->UpdateCB(renderer::eCbType::CbLightViewProjMatrix, &cbLightVpMat);

    const XMFLOAT4 lightDirection(mLight->GetDirection());
    renderer::CbLightProperty cbLightProperty;
    cbLightProperty.First = mLight->GetColor();
    cbLightProperty.Second = XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
    mShaderManager->UpdateCB(renderer::eCbType::CbLightProperty, &cbLightProperty);

   const XMMATRIX uiProjMat = XMMatrixOrthographicOffCenterLH(0.0, mWindowWidth, mWindowHeight, 0.0, 0.1f, 100.0f);
    renderer::CbScreenSpaceMatrix cbScreenSpaceMatrix = {};
    cbScreenSpaceMatrix.Matrix = XMMatrixTranspose(uiProjMat);
    mShaderManager->UpdateCB(renderer::eCbType::CbOrthoMatrix, &cbScreenSpaceMatrix);

    const XMFLOAT3 lightPosition(mLight->GetPosition());
    mLightIcon->SetPosition(lightPosition);
    mLightIcon->UpdateScaleMatrix(mCamera->GetViewMatrix());

    mSkybox->Update(mCamera->GetCameraPositionFloat());
    mCharacter->Update();


    mCommandList.clear();

    mSkybox->SubmitCommand(mCommandList);

    mFloor->SubmitCommand(mCommandList);

    mCharacter->SubmitCommand(mCommandList);

    mLight->SubmitDebugCommand(mCommandList);
    mLightIcon->SubmitCommand(mCommandList);

    mShadowDebugPanel->SubmitCommand(mCommandList);
    mGBufferColorDebugPanel->SubmitCommand(mCommandList);
    mGBufferNormalDebugPanel->SubmitCommand(mCommandList);
    mGBufferPositionDebugPanel->SubmitCommand(mCommandList);
    mGBufferSpecularDebugPanel->SubmitCommand(mCommandList);
    mGBufferAmbientDebugPanel->SubmitCommand(mCommandList);

    // MEMO: Renderer 전용 pass를 사용하는 command가 존재하는지 검사하여 assertion.
    for(renderer::RenderPacket& command : mCommandList)
    {
        if(command.RenderPass == renderer::eRenderPass::Deferred)
        {
            ASSERT(false, "Deferred는 Renderer 내부에서만 사용하므로 외부에서 사용할 수 없습니다.");
        }
    }

    renderer::SubMesh& subMeshNdcPlane = mNdcMeshDeferred.SubMeshes.front();
    renderer::RenderPacket deferredCommand = renderer::RenderPacket::MakeCommand(
        renderer::eVertexFormat::PT,
        renderer::GetVertexStrideSize(renderer::eVertexFormat::PT),
        renderer::eBufferUsage::Static,
        false,
        subMeshNdcPlane.VertexRange,
        subMeshNdcPlane.IndexRange,
        subMeshNdcPlane.Material,
        renderer::eRenderPass::Deferred,
        XMMatrixIdentity(),
        renderer::eShader::Deferred,
        renderer::eRasterType::Basic,
        renderer::eSamplerType::AnisotropicWrap,
        renderer::eBlendState::Opaque,
        renderer::ePrimitiveTopology::Triangles,
        true,
        renderer::eDepthStencilState::DepthOffStencilOff
    );
    mCommandList.push_back(deferredCommand);

    std::sort(mCommandList.begin(), mCommandList.end(), renderer::RenderPacketCompareDecr);
}

void Application::renderScene()
{
    mRenderer->ClearAllScreenAndDepth();

    for (auto& command : mCommandList)
    {
        if (mCommandCache.RenderPass != command.RenderPass)
        {
            const renderer::eRenderTarget renderTarget = mRenderer->GetRenderTargetByRenderPass(command.RenderPass);
            if(renderTarget != mCommandCache.RenderTarget)
            {
                mRenderer->BindRenderTargetByRenderPass(command.RenderPass);
                mCommandCache.RenderTarget = renderTarget;

                // MEMO: 현재 렌더패킷에 정보가 있지 않아서 이렇게 처리.
                mRenderer->SetViewport(renderTarget != renderer::eRenderTarget::Shadow);
            }
            mCommandCache.RenderPass = command.RenderPass;

        }

        bool bNeedBindBuffer = (mCommandCache.BufferUsage != command.BufferUsage) || (mCommandCache.Stride != command.Stride);

        // MEMO: Buffer는 Stride 별로 Chunk가 나뉘어져 있기 때문에 Stride가 달라도 Bind를 다시 해줘야 함.
        if (mCommandCache.VertexFormat != command.VertexFormat)
        {
            mRenderer->BindInputLayoutTo(command.VertexFormat);
            bNeedBindBuffer = true;
            mCommandCache.VertexFormat = command.VertexFormat;
        }

        if(bNeedBindBuffer)
        {
            ASSERT(command.Stride > 0, "유효하지 않은 버퍼이거나 올바르지 않은 command. stride(%d)", command.Stride);
            if (command.BufferUsage == renderer::eBufferUsage::Dynamic)
            {
                mRenderer->BindVertexBufferDynamic(command.Stride);
                mRenderer->BindIndexBufferDynamic();
            }
            else
            {
                mRenderer->BindVertexBuffer(command.Stride);
                mRenderer->BindIndexBuffer();
            }
            mCommandCache.BufferUsage = command.BufferUsage;
            mCommandCache.Stride = command.Stride;
        }

        if (mCommandCache.TopologyType != command.RenderState.TopologyType)
        {
            mRenderer->BindPrimitiveTopologyByType(command.RenderState.TopologyType);
            mCommandCache.TopologyType = command.RenderState.TopologyType;
        }

        if (mCommandCache.ShaderType != command.RenderState.ShaderType)
        {
            mRenderer->BindShaderTo(command.RenderState.ShaderType);
            mCommandCache.ShaderType = command.RenderState.ShaderType;

            // MEMO: Renderer 예약 Slot 바인딩
            mRenderer->BindCbToVsByType(0, 1, renderer::eCbType::CbWorld);
            mRenderer->BindCbToVsByType(1, 1, renderer::eCbType::CbViewProj);
            mRenderer->BindCbToVsByType(2, 1, renderer::eCbType::CbLightViewProjMatrix);
            mRenderer->BindCbToVsByType(3, 1, renderer::eCbType::CbLightProperty);
            mRenderer->BindCbToVsByType(4, 1, renderer::eCbType::CbCameraPosition);
            mRenderer->BindCbToVsByType(5, 1, renderer::eCbType::CbOrthoMatrix);

            // MEMO: Material 바인딩
            if(command.RenderState.CbBindingDesc.BindSlot >= 0)
            {
                if (command.RenderState.CbBindingDesc.bBindPixelShader)
                {
                    mRenderer->BindCbToPs(command.RenderState.CbBindingDesc.BindSlot, 1, command.RenderState.CbBindingDesc.Type);
                }
                else
                {
                    mRenderer->BindCbToVsByType(command.RenderState.CbBindingDesc.BindSlot, 1, command.RenderState.CbBindingDesc.Type);
                }
            }
        }

        for(uint8_t texture = static_cast<uint8_t>(renderer::eTextureType::Diffuse); texture < static_cast<uint8_t>(renderer::eTextureType::TextureTypeCount); ++texture)
        {
            if(command.RenderState.TexBindingSlots[texture] < 0)
            {
                continue;
            }

            if (command.Material.TextureHashes[texture])
            {
                mRenderer->BindTextureToPs(command.RenderState.TexBindingSlots[texture], command.Material.TextureHashes[texture]);
            }
        }
        

        if(mCommandCache.DepthStencilUsage != command.RenderState.DepthStencilState)
        {
            mRenderer->BindDepthStencilState(command.RenderState.DepthStencilState);
            mCommandCache.DepthStencilUsage = command.RenderState.DepthStencilState;
        }

        if (mCommandCache.SamplerType != command.RenderState.SamplerType || mCommandCache.SamplerBindingSlot != command.RenderState.SamplerBindingSlot)
        {
            if(command.RenderState.SamplerBindingSlot >= 0)
            {
                mRenderer->BindSamplerToPsByType(command.RenderState.SamplerBindingSlot, command.RenderState.SamplerType);
                mCommandCache.SamplerType = command.RenderState.SamplerType;
                mCommandCache.SamplerBindingSlot = command.RenderState.SamplerBindingSlot;
            }
        }

        if (mCommandCache.RasterType != command.RenderState.RasterType)
        {
            mRenderer->BindRasterStateByType(command.RenderState.RasterType);
            mCommandCache.RasterType = command.RenderState.RasterType;
        }

        if (mCommandCache.BlendState != command.RenderState.BlendState)
        {
            // MEMO: blendFactor는 아직 사용하지 않음.
            mRenderer->BindBlendStateByType(command.RenderState.BlendState);
            mCommandCache.BlendState = command.RenderState.BlendState;
        }

        // MEMO: Material 식별자가 없는 상태이므로 우선은 매번 업로드
        if (command.RenderState.CbBindingDesc.BindSlot >= 0)
        {
            mShaderManager->UpdateMaterial(command.RenderState.CbBindingDesc.Type, command.Material);
        }

        const renderer::CbWorld cbMatWorld = { command.MatWorld };
        mShaderManager->UpdateCB(renderer::eCbType::CbWorld, &cbMatWorld);

        if(command.IndexRange.Count)
        {
            mRenderer->DrawIndexed(command.IndexRange.Count, command.IndexRange.StartIndex, command.VertexRange.StartIndex);
        }
        else
        {
            mRenderer->Draw(command.VertexRange.Count, command.VertexRange.StartIndex);
        }

        // MEMO: Shadow RenderTarget으로 써야 하므로 다시 Texture Slot에서 제거.
        if (static_cast<bool>(command.RenderState.UseShadowMapUsage))
        {
            if(mCommandCache.RenderPass == renderer::eRenderPass::UI)
            {
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::Diffuse)]);
            }
            else if (mCommandCache.RenderPass == renderer::eRenderPass::Deferred)
            {
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::Shadow)]);
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::GBufferColor)]);
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::GBufferNormal)]);
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::GBufferPosition)]);
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::GBufferSpecular)]);
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::GBufferAmbient)]);
            }
            else
            {
                mRenderer->UnbindTexturePs(command.RenderState.TexBindingSlots[static_cast<uint8_t>(renderer::eTextureType::Shadow)]);
            }
        }
    }
}

void Application::processInputVulkan(double deltaTime)
{

    mDirectInput->UpdateInput();

    /*
     *  direct input ver
     */

    const unsigned char* gKeyboard = mDirectInput->GetKeyboardPress();

    if (!(mDirectInput->GetControlMode() & static_cast<uint32_t>(core::eControlFlags::KEYBOARD_MOVEMENT_MODE)))
    {
        int mouseX = 0;
        int mouseY = 0;
        mDirectInput->GetMouseDeltaPosition(mouseX, mouseY);
        if (!(mouseX == 0 && mouseY == 0))
        {
            constexpr float KEYBOARD_SPEED = 0.1f;
            mCamera->RotateAxis(XMConvertToRadians(static_cast<float>(mouseX)) * static_cast<float>(deltaTime) * KEYBOARD_SPEED, XMConvertToRadians(static_cast<float>(mouseY)) * static_cast<float>(deltaTime) * KEYBOARD_SPEED);
        }
    }
    else
    {
        constexpr float MOUSE_SPEED = 10.0f;
        if (gKeyboard[DIK_W] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(-(MOUSE_SPEED * static_cast<float>(deltaTime))));
        }

        if (gKeyboard[DIK_S] & 0x80)
        {
            mCamera->RotateAxis(0.0f, XMConvertToRadians(MOUSE_SPEED * static_cast<float>(deltaTime)));
        }
        if (gKeyboard[DIK_A] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(-(MOUSE_SPEED * static_cast<float>(deltaTime))), 0.0f);
        }

        if (gKeyboard[DIK_D] & 0x80)
        {
            mCamera->RotateAxis(XMConvertToRadians(MOUSE_SPEED * static_cast<float>(deltaTime)), 0.0f);
        }
    }

    // 마우스 휠 처리 이전에 임시용.
    // 카메라와 물체간의 거리 조절(구체 크기 확대/축소)
    constexpr float MOVEMENT_SPEED = 0.01f;
    if (gKeyboard[DIK_Q] & 0x80)
    {
        mCamera->AddRadiusSphere(static_cast<float>(deltaTime * MOVEMENT_SPEED));
    }

    if (gKeyboard[DIK_E] & 0x80)
    {
        mCamera->AddRadiusSphere(static_cast<float>(-deltaTime * MOVEMENT_SPEED));
    }

    // 키보드<-> 마우스 조작 전환
    static bool bPressCKey = false;
    if (!(gKeyboard[DIK_C] & 0x80) && bPressCKey)
    {
        mDirectInput->SetControlMode(static_cast<uint32_t>(core::eControlFlags::KEYBOARD_MOVEMENT_MODE));
    }
    bPressCKey = gKeyboard[DIK_C] & 0x80;

    if (gKeyboard[DIK_Z] & 0x80)
    {
        mCamera->AddHeight(static_cast<float>(-deltaTime) * MOVEMENT_SPEED);
    }

    if (gKeyboard[DIK_X] & 0x80)
    {
        mCamera->AddHeight(static_cast<float>(deltaTime) * MOVEMENT_SPEED);
    }

    if (gKeyboard[DIK_ESCAPE] & 0x80)
    {
        SendMessage(mWindow->GetHandle(), WM_DESTROY, 0, 0);
    }
}

void Application::updateSceneVulkan()
{

}

void Application::renderSceneVulkan()
{

}
