#include "Application.h"
#include <algorithm>
#include <string>
#include "Window.h"
#include "Core/Input.h"
#include "Core/Timer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Importer/ImportedModelData.h"
#include "Renderer/Importer/ModelImporter.h"
#include "Renderer/Resources/Model.h"
#include "Scene/Billboard.h"
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
    , mNdcMeshDeferred{}
    , mCurSubMeshIndexFocusModel(0)
    , mVkRenderer(nullptr)
    , mImporter(nullptr)
    , mCharacter(nullptr)
    , mCamera(nullptr)
    , mSkybox(nullptr)
    , mLight(nullptr)
    , mLightIcon(nullptr)
    , mFloor(nullptr)
    , mDirectInput(nullptr)
{
    mImporter = new renderer::ModelImporter();
}

Application::~Application()
{
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
    delete mVkRenderer;
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
    if (!(mVkRenderer->initializeWithVulkan(handleWindow, hInstance, mWindowWidth, mWindowHeight, mWindow->GetAppNameA())))
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


        mVkRenderer->WaitForLastFrame();
        mVkRenderer->BeginRender();

        processInputVulkan(deltaTime);
        updateSceneVulkan();


        renderSceneVulkan();

        mVkRenderer->EndRender();

        mVkRenderer->SubmitCommands();
        mVkRenderer->Present();

        mVkRenderer->CountFrame();
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


bool Application::initializeSceneVulkan()
{
    core::Timer::Initialize();

    mCamera = new scene::Camera(
        XMVectorSet(0.0f, 10.0f, -15.0f, 0.0f)
        , XMVectorSet(0.0f, 10.0f, 0.0f, 0.0f)
        , XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
        mWindowWidth,
        mWindowHeight);

    mCharacter = new renderer::Model(mCamera);

    HashID modelHash = 0;
    renderer::ImportedModelContainer modelContainer;
    const int8_t* const modelFilePath = reinterpret_cast<int8_t*>("/AssetData/models/unagi.fbx");
    mImporter->LoadFbxModel(modelFilePath, modelHash, modelContainer);

    renderer::Mesh newMesh = {};
    newMesh.SubMeshes.reserve(modelContainer.SubMeshes.size());
    newMesh.VertexFormat = renderer::eVertexFormat::PTN;
    const int16_t strideVertex = GetVertexStrideSize(newMesh.VertexFormat);
    const int16_t strideIndex = sizeof(uint32_t);
    const int16_t filePathLength = static_cast<int16_t>(strlen(reinterpret_cast<char const*>(modelFilePath)));
    ASSERT(filePathLength + 1 <= util::MAX_NAME_LENGTH, "str이 버퍼 사이즈보다 큼. 입력 문자열을 검점하거나, util의 Length 상수 조정 필요. str(%d)", filePathLength + 1);
    (void)memcpy(newMesh.MeshName, modelFilePath, filePathLength + 1);
    newMesh.MeshHash = modelHash;

    int32_t totalVertexCount = 0;
    int32_t totalIndexCount = 0;
    for (auto& subMesh : modelContainer.SubMeshes)
    {
        renderer::SubMesh newSubMesh = {};

        newSubMesh.SubMeshHash = util::GetDjb2Hash(subMesh.MeshName, modelHash);
        mVkRenderer->AddVertexData(subMesh.VertexBuffer.get(), sizeof(renderer::VertexPTN), subMesh.VertexCount, newSubMesh.SubMeshHash, newSubMesh.VertexRange);
        mVkRenderer->AddIndexData(subMesh.IndexBuffer.get(), sizeof(uint32_t), subMesh.IndexCount, newSubMesh.SubMeshHash, newSubMesh.IndexRange);

        totalVertexCount += newSubMesh.VertexRange.Count;
        totalIndexCount += newSubMesh.IndexRange.Count;
        (void)memcpy(newSubMesh.SubMeshName, subMesh.MeshName, util::MAX_NAME_LENGTH);

        newSubMesh.MinBound = std::move(subMesh.MinBound);
        newSubMesh.MaxBound = std::move(subMesh.MaxBound);

        newSubMesh.Material.Factors = std::move(subMesh.MaterialParam);
        newSubMesh.Material.Factors.IsLitOn = static_cast<float>(true);

        newMesh.SubMeshes.push_back(std::move(newSubMesh));
    }

    newMesh.VertexRange.StartIndex = newMesh.SubMeshes.front().VertexRange.StartIndex;
    newMesh.VertexRange.Count = totalVertexCount;

    newMesh.IndexRange.StartIndex = newMesh.SubMeshes.front().IndexRange.StartIndex;
    newMesh.IndexRange.Count = totalIndexCount;

    mCharacter->SetMesh(newMesh);

    HashID simplePipelineHash;
    mVkRenderer->TryCreateGraphicsPipeline(simplePipelineHash, renderer::eShader::Simple, renderer::eVertexFormat::PTN, renderer::ePrimitiveTopology::Triangles, renderer::eRasterType::Basic, renderer::Opaque, renderer::eDepthStencilState::DepthOffStencilOff, renderer::eRenderPass::Main);
    //mVkRenderer->SetSimplePipeline(simplePipelineHash);
    return true;
}

bool Application::initializeManagersVulkan()
{
    mVkRenderer->PrepareObjects();
    return true;
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
    // MEMO: 이후 작업들이 카메라 정보에 의존하므로 카메라를 먼저 업데이트
    mCamera->Update();

    // MEMO: Renderer가 예약한 CB들 업로드
    renderer::CbViewProj cbViewProj;
    cbViewProj.Matrix = XMMatrixTranspose(mCamera->GetViewProjectionMatrix());

    mCharacter->Update();
    renderer::CbWorld cbWorld{ mCharacter->GetMatrix() };
    std::vector<std::pair<renderer::eCbType, void*>> uniforms =
    {
        {renderer::eCbType::CbViewProj, static_cast<void*>(&cbViewProj)},
        {renderer::eCbType::CbWorld, static_cast<void*>(&cbWorld)},
    };
    mVkRenderer->UpdateScene(uniforms);
}

void Application::renderSceneVulkan()
{
    mVkRenderer->RenderScene();
    mCharacter->DrawVk(*mVkRenderer);
}
