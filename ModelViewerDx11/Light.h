#pragma once
#include "Camera.h"
#include "framework.h"

// MEMO: 만들어놓고 XMVECTOR 비율이 더 높으면 XMFLOAT3->XMVECTOR로
class Light final // working like directional light
{
    enum eCascadeLevel
    {
        Level_4 = 5 // near - 1 - 2- 3- 4 - far
    };
public:
    Light(XMFLOAT3 pos, XMFLOAT3 dir, XMFLOAT3 color, Camera* camera, float nearPlane, float farPlane);
    ~Light();

    void Initialize();
    void Update(Camera* camera);
    void Draw();
    void DrawDebug();

    void Move(double deltaTime, float direction);

    void SetupCascade();

    void SetDirection(XMFLOAT3 dir);
    void SetColor(XMFLOAT3 color);

    XMFLOAT4 GetDirection() const;
    XMFLOAT4 GetPosition() const;
    XMFLOAT4 GetColor() const;
    const XMMATRIX* const GetViewProjMatrix() const;

private:

    void updateMatrices();
    void updateLightPropertyCB();


    void getPointsFromMatrix(XMMATRIX* matView, float nearPlane, float farPlane, XMMATRIX* const outMatLightView, XMMATRIX* const outMatLightProj);
private:

    XMFLOAT3 mPosition;
    XMFLOAT3 mDirection;
    XMFLOAT3 mColor;

    // for shadow-map
    XMMATRIX mMatView;
    XMMATRIX mMatProj;
    XMMATRIX mMatViewProj;

    ID3D11Buffer* mCbMatWorld;

    class Plane* mMesh;
    XMMATRIX mMatWorld;
    ID3D11BlendState* mBlendState;

  //  XMFLOAT3 mLines[24];
    std::vector<XMFLOAT3> mLines;
    ID3D11Buffer* mLinesBuffer;

    float mNearPlane;
    float mFarPlane;
    Camera* mCamera;
    XMMATRIX mMatLightViews[eCascadeLevel::Level_4];
    XMMATRIX mMatLightProjs[eCascadeLevel::Level_4];
    float mCascadePlaneDistances[eCascadeLevel::Level_4+1];
};
