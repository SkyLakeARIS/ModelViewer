#pragma once
#include "../framework.h"

namespace renderer
{
    class BufferManager;

    class Renderer final : IUnknown
    {
    public:
        typedef struct CbMatrix
        {
            XMMATRIX Matrix;
        }CbWorld, CbViewProj, CbLightViewProjMatrix;

        typedef struct CbFloat3
        {
            XMFLOAT3    Float3;
            float       Reserve;
        } CbCameraPosition, CbOutlineProperty, CbColor;

        typedef struct CbTwoVec4
        {
            XMFLOAT4    First;
            XMFLOAT4    Second;
        }CbLightProperty;

        typedef struct CbMaterial
        {
            XMFLOAT3 Diffuse;
            float Reserve0;
            XMFLOAT3 Ambient;
            float Reserve1;
            XMFLOAT3 Specular;
            float Reserve2;
            XMFLOAT3 Emissive;
            float Reserve3;
            float Opacity; // 알파값으로 사용
            float Reflectivity;
            float Shininess; // 스페큘러 거듭제곱 값
            float Reserve4;
        };

        //typedef struct CbOneMatrix
        //{
        //    XMMATRIX WVP;
        //}CbOutline, CbLightMatrix, CbWVP;


        enum class eCbType : uint8_t
        {
            CbWorld, // TODO WorldMat용 buffer는 각 개체가 들고 있으므로 제거.(해도 되겠지)
            CbViewProj,
            CbLightViewProjMatrix,
            CbCameraPosition,
            CbOutlineProperty,
            CbLightProperty,
            CbMaterial,
            CbColor,
            NumConstantBuffer
        };

        enum class eRasterType
        {
            Basic,
            Outline,
            Skybox,
            CullBack,
            NumRaster,
        };

        enum class eInputLayout : uint8_t
        {
            PTN,    // pos, normal, tex
            PT,     // pos, tex
            P,      // pos
            NumInputlayout
        };

        enum class eShader : uint32_t
        {
            Outline,
            Skybox,
            Shadow,
            BasicWithShadow,
            RenderToTexture,
            Color,
            NumShader
        };

        // RenderTarget, DepthStencil 
        enum class eRenderTarget : uint8_t
        {
            Default,
            Shadow,
            NumRenderTarget
        };

    private:

        enum class eVertexShader : uint32_t
        {
            VsBasicWithShadow,
            VsOutline,
            VsRenderToTexture,
            VsSimple,
            VsSkybox,
            NumVertexShader
        };

        enum class ePixelShader : uint32_t
        {
            PsBasicWithShadow,
            PsOutline,
            PsShadow,
            PsSkybox,
            PsRenderToTexture,
            PsColor,
            NumPixelShader
        };

        struct ShaderMap
        {
            eShader Type;
            eVertexShader VsIndex;
            ePixelShader PsIndex;
        };


        struct RenderTargetDepthStencilMap
        {
            uint32_t RenderTargetIndex;
            uint32_t NumViews;
            uint32_t DepthStencilIndex;
        };
        typedef RenderTargetDepthStencilMap RtvDsMap;

        struct ConstantBufferMap
        {
            eCbType Index; // added for easy to see.
            uint32_t ByteWidth;
        };


    public:

        static Renderer* GetInstance();

        // D3D
        HRESULT CreateDeviceAndSetup(DXGI_SWAP_CHAIN_DESC& swapChainDesc, HWND hWnd, uint32 height, uint32 width, bool bDebugMode);

        HRESULT CreateRenderTargetView(ID3D11Texture2D* const texture, D3D11_RENDER_TARGET_VIEW_DESC* const desc, ID3D11RenderTargetView** outRtv, const char* const debugTag = "NO_INFO");

        HRESULT CreateDepthStencilView(ID3D11Texture2D* const texture, D3D11_DEPTH_STENCIL_VIEW_DESC* const desc, ID3D11DepthStencilView** outDs, const char* const debugTag = "NO_INFO");

        // 일단 Renderer에 CB 개체 가지도록
        HRESULT CreateConstantBuffer(D3D11_BUFFER_DESC& desc, ID3D11Buffer** outCb);


        // TODO : 정리필요. 
        // 그림자 매핑을 위한 설계
        void SetViewport(bool bFullScreen);
        HRESULT CreateShadowRenderTarget();


        // init - program

        HRESULT PrepareRender();


        // Cate : shader

        HRESULT CreateVertexShaderAndInputLayout(
            const WCHAR* const path
            , D3D11_INPUT_ELEMENT_DESC* const desc
            , uint32 numDescElements
            , ID3D11VertexShader** const outVertexShader
            , ID3D11InputLayout** const outInputLayout);

        HRESULT CreateInputLayout(
            const WCHAR* const path
            , D3D11_INPUT_ELEMENT_DESC* const desc
            , uint32 numDescElements
            , eInputLayout type
            , ID3D11InputLayout** const outInputLayout);

        HRESULT CreateVertexShader(
            const WCHAR* const path
            , ID3D11VertexShader** const outVertexShader);

        HRESULT CreatePixelShader(const WCHAR* const path, ID3D11PixelShader** const outPixelShader);


        // Cate : texture 
        HRESULT CreateTexture2D(D3D11_TEXTURE2D_DESC& desc, ID3D11Texture2D** outTex, const char* tag);

        HRESULT CreateTextureResource(
            const WCHAR* fileName
            , WIC_FLAGS flag
            , D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc
            , ID3D11ShaderResourceView** outShaderResourceView);

        // for skybox
        HRESULT CreateDdsTextureResource(
            const WCHAR* fileName
            , DDS_FLAGS flag
            , D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc
            , ID3D11ShaderResourceView** outShaderResourceView);

        // Renderer 
        void ClearScreenAndDepth(eRenderTarget type);
        void ClearDepthBuffer();
        void Present() const;

        void    Cleanup();

        HRESULT        CheckDeviceLost();

        // Debug
        static void    CheckLiveObjects();

        // COM
        ULONG   AddRef() override;
        ULONG   Release() override;
        HRESULT QueryInterface(const IID& riid, void** ppvObject) override;


        //  D3D state

        void UpdateCB(eCbType type, void* data) const
        {
            mDeviceContext->UpdateSubresource(mCbList[static_cast<uint32_t>(type)], 0U, nullptr, data, 0U, 0U);
        }

        void UpdateCbTo(ID3D11Buffer* buffer, void* data) const
        {
            mDeviceContext->UpdateSubresource(buffer, 0U, nullptr, data, 0U, 0U);
        }

        void BindCbToVsByType(uint32_t slot, uint32_t numBuffer, eCbType type) const
        {
            mDeviceContext->VSSetConstantBuffers(slot, numBuffer, &mCbList[static_cast<uint32_t>(type)]);
        }

        void BindCbToVsByObj(uint32_t slot, uint32_t numBuffer, ID3D11Buffer** buffer) const
        {
            mDeviceContext->VSSetConstantBuffers(slot, numBuffer, buffer);
        }

        void BindCbToPs(uint32_t slot, uint32_t numBuffer, eCbType type) const
        {
            mDeviceContext->PSSetConstantBuffers(slot, numBuffer, &mCbList[static_cast<uint32_t>(type)]);
        }

        void BindVertexBuffer(uint32_t stride, uint32_t offset);
        void BindIndexBuffer(uint32_t offset);

        void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology) const;

        void SetRenderTargetTo(eRenderTarget type);
        void SetInputLayoutTo(eInputLayout type) const;
        void SetShaderTo(eShader type);
        void SetRasterState(eRasterType type);

        void SetDepthStencilState(bool bSkybox); // 현재는 스카이박스만 사용하므로

        // getter
        ID3D11Device*           GetDevice() const;
        ID3D11DeviceContext*    GetDeviceContext() const;

        BufferManager* const GetBufferManager() const;

        void                    GetWindowSize(uint32& outWidth, uint32& outHeight) const;
        HWND                    GetWindowHandle() const;


        // tex resource
        ID3D11ShaderResourceView*        GetShadowTexture();

        ID3D11ShaderResourceView*        GetDefaultTexture() const;
    private:
        Renderer();
        ~Renderer();

        HRESULT compileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

        HRESULT createRasterState();

        HRESULT setupShaders();

    public:

        static constexpr int32_t sVertexBufferDefaultSize = 4096;
        static constexpr int32_t sIndexBufferDefaultSize = 4096;
    private:
        // TODO texture resource manager 생기면 이동 시키기.
        ID3D11ShaderResourceView*   mDefaultTexture;
    private:

        static Renderer*            mInstance;

        ULONG                       mRefCount;

        // D3D Device
        ID3D11Device*               mDevice;
        ID3D11DeviceContext*        mDeviceContext;


        // shader
        ShaderMap           mShaderMapTable[static_cast<uint32_t>(eShader::NumShader)]; // combine vs-ps pairs
        ID3D11VertexShader* mVertexShadersList[static_cast<uint32_t>(eVertexShader::NumVertexShader)];
        ID3D11PixelShader*  mPixelShaderList[static_cast<uint32_t>(ePixelShader::NumPixelShader)];
        ID3D11InputLayout*  mInputLayoutList[static_cast<uint32_t>(eInputLayout::NumInputlayout)];

        // 
        IDXGISwapChain*             mSwapChain;

        ID3D11Texture2D*            mDepthStencilTexture;
        ID3D11DepthStencilState*    mSkyboxDepthStencil;

        // render target, depthStencil
        // 일단은 쉽게 무조건 1:1매핑으로 (nullptr 처리는 나중에 최적화)
        ID3D11RenderTargetView* mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::NumRenderTarget)];
        ID3D11DepthStencilView* mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::NumRenderTarget)];
        RtvDsMap mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::NumRenderTarget)]; // combine rtv - depth-stencil pairs

        // shadow
        ID3D11Texture2D*            mTexShadow;
        ID3D11Texture2D*            mTexColor;
        ID3D11ShaderResourceView*     mShadowSrv;
        ID3D11ShaderResourceView**     mCascadeShadowSrvList;
        D3D11_VIEWPORT mViewportFull;
        D3D11_VIEWPORT mViewportTex;

        // raster state
        ID3D11RasterizerState*      mRasterStates[static_cast<uint32>(eRasterType::NumRaster)]; // 0: back cull, 1: front cull

        // CB
        ID3D11Buffer* mCbList[static_cast<uint8_t>(eCbType::NumConstantBuffer)];
    
        // window
        HWND        mhWindow;
        uint32      mWindowHeight;
        uint32      mWindowWidth;

        // Managers
        BufferManager* mBufferManager;
    };
}
