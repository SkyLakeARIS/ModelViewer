#pragma once
#include "Renderer/Resources/Mesh.h"
#include "Renderer/Resources/RenderPacket.h"


namespace core
{
    class DirectInput;
}

namespace ui
{
    class DebugPanel;
}

class Window;

namespace scene
{
    class Billboard;
    class Floor;
    class Light;
    class Sky;
    class Camera;
}

namespace renderer
{
    class Model;
    class ModelImporter;
    class TextureManager;
    class BufferManager;
    class ResourceManager;
    class Renderer;
    class ShaderManager;
}

class Application
{
public:

    Application();
    ~Application();

    bool InitializeWithWindows(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow);
    bool InitializeWithWindowsVk(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow);
    void Run();
    void RunWithVulkan();

private:
    bool initializeScene();
    bool initializeManagers();

    bool initializeSceneVulkan();
    bool initializeManagersVulkan();

    void processInput(double deltaTime);
    void updateScene();

    void renderScene();

    void processInputVulkan(double deltaTime);
    void updateSceneVulkan();

    void renderSceneVulkan();

private:

    int16_t mWindowWidth;
    int16_t mWindowHeight;
    int16_t mAppFrameRate;

    Window* mWindow;
    renderer::Mesh mNdcMeshDeferred;

    int32_t mCurSubMeshIndexFocusModel;
    std::vector<renderer::RenderPacket> mCommandList;
    renderer::RenderPacketCache mCommandCache;
    renderer::Renderer* mRenderer;
    renderer::ModelImporter* mImporter;
    renderer::Model* mCharacter;
    scene::Camera* mCamera;
    scene::Sky* mSkybox;
    scene::Light* mLight;
    scene::Billboard* mLightIcon;
    scene::Floor* mFloor;
    renderer::BufferManager* mBufferManager;
    renderer::TextureManager* mTextureManager;
    renderer::ResourceManager* mResourceManager;
    renderer::ShaderManager* mShaderManager;
    core::DirectInput* mDirectInput;
    ui::DebugPanel* mShadowDebugPanel;
    ui::DebugPanel* mGBufferColorDebugPanel;
    ui::DebugPanel* mGBufferNormalDebugPanel;
    ui::DebugPanel* mGBufferPositionDebugPanel;
    ui::DebugPanel* mGBufferSpecularDebugPanel;
    ui::DebugPanel* mGBufferAmbientDebugPanel;
};
