#pragma once
#include "framework.h"
#include "Scene/Billboard.h"
#include "UI/DebugPanel.h"

class Window;

namespace scene
{
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

    void updateScene(double deltaTime);

    void preprocess();
    void renderScene();
    void renderUI();
private:

    int16_t mWindowWidth;
    int16_t mWindowHeight;
    int16_t mAppFrameRate;

    Window* mWindow;

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
    core::DirectInput* mDirectInput;
    ui::DebugPanel* mShadowDebugPanel;
};
