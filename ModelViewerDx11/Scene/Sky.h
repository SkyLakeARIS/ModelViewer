#pragma once
#include "../framework.h"
#include "../Renderer/Resources/ModelData.h"

namespace renderer
{
    class Renderer;
}

namespace scene
{
    class Camera;

    class Sky
    {
    public:

        Sky(Camera& camera);
        ~Sky();

        HRESULT Initialize(uint32 latLines, uint32 lonLines);

        void Draw();
        void Update();
    private:

        HRESULT createSphere(uint32 latLines, uint32 lonLines);

    private:

        Camera* mCamera;

        HashID mModelHash;
        XMMATRIX mWorld;


        renderer::Mesh mMesh; // sphere model

        uint32 mLatLines;
        uint32 mLonLines;

    };
}
