#pragma once
#include "Renderer/Resources/Mesh.h"
#include "framework.h"

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
    class VkRenderer;
    class Model;
    class ModelImporter;
}

class Application
{
public:

    Application();
    ~Application();

    bool InitializeWithWindowsVk(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow);
    void RunWithVulkan();

private:
    bool initializeSceneVulkan();
    bool initializeManagersVulkan();

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
    renderer::VkRenderer* mVkRenderer;
    renderer::ModelImporter* mImporter;
    renderer::Model* mCharacter;
    scene::Camera* mCamera;
    scene::Sky* mSkybox;
    scene::Light* mLight;
    scene::Billboard* mLightIcon;
    scene::Floor* mFloor;
    core::DirectInput* mDirectInput;
};
