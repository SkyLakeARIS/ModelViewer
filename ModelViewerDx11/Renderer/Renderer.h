#pragma once
#include "./Resources/RenderTypes.h"

namespace renderer
{
    class ShaderManager;
    struct RenderPacket;
}

namespace renderer
{
    class BufferManager;
    class TextureManager;

    class VkRenderer final
    {
    public:
        VkRenderer();
        ~VkRenderer();

        bool initializeWithVulkan(HWND handleWindow, HINSTANCE handleInstance, int16_t width, int16_t height, const char* const appName);

    private:
        bool createInstance(const char* const appName);
        bool createSurface(HWND handleWindow, HINSTANCE handleInstance);
        bool choosePhysicalDevice();
        bool createLogicalDevice();
        bool createSwapChain(uint16_t width, uint16_t height);

    private:
        int16_t mWindowHeight;
        int16_t mWindowWidth;

        VkInstance mVkInstance;

        VkPhysicalDevice mPhysicalDevice;
        VkPhysicalDeviceProperties2 mPhysicalDeviceProperties;
        VkDevice mDevice;

        VkSurfaceKHR mSurface;
        VkSwapchainKHR mSwapChain;
        std::vector<VkImage> mImages;
        std::vector<VkImageView> mImageViews;
    };

    class Renderer final : IUnknown
    {
    private:
        static constexpr uint8_t MAX_RENDER_TARGET_VIEW = 8;
        struct RenderTargetBindDesc
        {
            eRenderPass RenderPass;
            uint32_t ViewCount;
            ID3D11RenderTargetView* RenderTargetViews[MAX_RENDER_TARGET_VIEW];
            ID3D11DepthStencilView* DepthStencilViews;
        };

        struct RenderTargetDepthStencilMap
        {
            uint32_t RenderTargetIndex;
            uint32_t NumViews;
            uint32_t DepthStencilIndex;
        };
        typedef RenderTargetDepthStencilMap RtvDsMap;

        struct PrimitiveTopologyMap
        {
            ePrimitiveTopology UserType;
            D3D11_PRIMITIVE_TOPOLOGY ApiType;
        };

        struct DepthStencilStateMap
        {
            eDepthStencilState Type;
            D3D11_DEPTH_STENCIL_DESC Desc;
        };

        struct BlendStatePreset
        {
            eBlendState Type;
            int32_t SrcBlend;
            int32_t DestBlend;
        };
    public:
        Renderer();
        ~Renderer();


        void SetManagers(BufferManager* const bufferManager, TextureManager* const textureManager, ShaderManager* const shaderManager);

        // D3D
        HRESULT CreateDeviceAndSetup(DXGI_SWAP_CHAIN_DESC& swapChainDesc, uint32_t width, uint32_t height, bool bDebugMode);
        HRESULT CreateRenderTargetView(ID3D11Texture2D* const texture, D3D11_RENDER_TARGET_VIEW_DESC* const desc, ID3D11RenderTargetView** outRtv, const char* const debugTag = "NO_INFO") const;
        HRESULT CreateDepthStencilView(ID3D11Texture2D* const texture, D3D11_DEPTH_STENCIL_VIEW_DESC* const desc, ID3D11DepthStencilView** outDs, const char* const debugTag = "NO_INFO") const;

        // 그림자 매핑을 위한 설계
        void    SetViewport(bool bFullScreen) const;
        HRESULT CreateShadowRenderTarget();

        // init - program
        bool initialize(HWND handleWindow, int16_t width, int16_t height, int16_t frameRate);

        // Cate : texture 
        HRESULT CreateTexture2D(D3D11_TEXTURE2D_DESC& desc, ID3D11Texture2D** outTex, const char* tag) const;

        // Renderer 
        void ClearAllScreenAndDepth() const;
        void ClearDepthBuffer() const;
        void Present() const;

        void    Cleanup();
        bool CheckDeviceLost(bool& outIsReInitialize) const;


        // COM
        ULONG   AddRef() override;
        ULONG   Release() override;
        HRESULT QueryInterface(const IID& riid, void** ppvObject) override;

        //  D3D state

        void BindCbToVsByType(uint32_t slot, uint32_t numBuffer, eCbType type) const;
        void BindCbToPs(uint32_t slot, uint32_t numBuffer, eCbType type) const;

        void BindVertexBuffer(uint32_t stride) const;
        void BindIndexBuffer() const;
        void BindVertexBufferDynamic(uint32_t stride) const;
        void BindIndexBufferDynamic() const;

        void BindSamplerToPsByType(uint32_t slot, eSamplerType type) const;
        void BindBlendStateByType(eBlendState type) const;
        void BindTextureToPs(uint32_t slot, HashID textureHash) const;
        void BindRasterStateByType(eRasterType type) const;
        void BindDepthStencilState(eDepthStencilState type) const;

        void UnbindTexturePs(uint32_t slot) const;

        void BindPrimitiveTopologyByType(ePrimitiveTopology topology) const;
        void BindRenderTargetByRenderPass(eRenderPass pass);
        void BindInputLayoutTo(eVertexFormat type) const;
        void BindShaderTo(eShader type) const;

        // SubmitCommand
        void Draw(uint32_t vertexCount, uint32_t startVertexLocation) const;
        void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation) const;

        // getter
        ID3D11Device*           GetDevice() const;
        ID3D11DeviceContext*    GetDeviceContext() const;

        void GetCurrentPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& outTopology) const;
        BufferManager* const GetBufferManager() const;
        eRenderTarget GetRenderTargetByRenderPass(eRenderPass renderPass) const;
    public:
        // Debug
        static void CheckLiveObjects();
    private:
        // MEMO: textureManager가 초기화된 후, 렌더러가 사용하는 텍스처를 추가. 등록되면 Manager가 수명 관리
        void registerSrvTexture();


        bool    createRasterState();
        HRESULT createSamplerState();
        bool    createPresetBlendStates();
        bool    createGBufferRenderTargets();
    private:


        ULONG                       mRefCount;

        int16_t mWindowHeight;
        int16_t mWindowWidth;

        // D3D Device
        ID3D11Device*               mDevice;
        ID3D11DeviceContext*        mDeviceContext;

        // 
        IDXGISwapChain*             mSwapChain;

        ID3D11DepthStencilState*    mDepthStencilStates[static_cast<uint8_t>(eDepthStencilState::StateCount)];

        // render target, depthStencil
        // 일단은 쉽게 무조건 1:1매핑으로 (nullptr 처리는 나중에 최적화)
        ID3D11RenderTargetView*   mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::RenderTargetCount)];
        ID3D11DepthStencilView*   mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::RenderTargetCount)];
        RtvDsMap                  mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::RenderTargetCount)]; // combine rtv - depth-stencil pairs
        ID3D11ShaderResourceView* mRenderTargetSRVs[static_cast<uint8_t>(eRenderTarget::RenderTargetCount)];
        RenderTargetBindDesc      mRenderTargetBindDescMap[static_cast<uint8_t>(eRenderPass::PassCount)];
        D3D11_VIEWPORT            mViewportFull;
        D3D11_VIEWPORT            mViewportTex;

        // raster state
        ID3D11RasterizerState*      mRasterStates[static_cast<uint32_t>(eRasterType::RasterCount)]; // 0: back cull, 1: front cull

        // sampler state
        ID3D11SamplerState* mSamplerState[static_cast<uint8_t>(eSamplerType::SamplerCount)];
        // blend state
        // MEMO: 자주 쓰이는 옵션으로 Preset을 뽑아서 사용(XTK의 CommonState)
        ID3D11BlendState* mBlendStates[static_cast<uint8_t>(eBlendState::StateCount)];
        // topology
        PrimitiveTopologyMap mPrimitiveTopologies[static_cast<uint8_t>(ePrimitiveTopology::TopologyCount)];
        // Managers
        BufferManager* mBufferManager;
        TextureManager* mTextureManager;
        ShaderManager* mShaderManager;
    };
}
