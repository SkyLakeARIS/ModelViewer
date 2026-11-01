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
    class ResourceManager;
    class Loader;
    class Renderer;
    class Model;
    class ModelImporter;
}

class Application
{
public:

    Application();
    ~Application();

    bool InitializeWithWindows(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow);
    void Run();

private:
    bool initializeScene();
    bool initializeManagers();

    void processInput(double deltaTime);
    void updateScene();

    void renderScene();

private:

    int16_t mWindowWidth;
    int16_t mWindowHeight;
    int16_t mAppFrameRate;

    Window* mWindow;
    renderer::Mesh mNdcMeshDeferred;
    renderer::Loader* mLoader;
    renderer::ResourceManager* mResourceManager;
    int32_t mCurSubMeshIndexFocusModel;
    renderer::Renderer* mVkRenderer;
    renderer::ModelImporter* mImporter;
    renderer::Model* mCharacter;
    scene::Camera* mCamera;
    scene::Sky* mSkybox;
    scene::Light* mLight;
    scene::Billboard* mLightIcon;
    scene::Floor* mFloor;
    core::DirectInput* mDirectInput;
};
