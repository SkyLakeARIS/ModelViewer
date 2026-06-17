#include "Renderer.h"
#include <string>
#include "../Util/Macro.h"
#include "Resources/BufferManager.h"
#include "Resources/TextureManager.h"

namespace renderer
{
    Renderer* Renderer::mInstance = nullptr;

    void Renderer::CheckLiveObjects()
    {
        HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
        ASSERT(dxgidebugdll != NULL, "dxgidebug.dll 로드 실패");

        decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

        IDXGIDebug* debug;

        GetDebugInterface(IID_PPV_ARGS(&debug));

        OutputDebugStringW(L"========================== Direct3D Object ref count 메모리 누수 체크 ===============================\r\n");
        OutputDebugStringW(L"========================== 반환되지 않은 IUnknown 객체가 있을경우 아래에 나타납니다. ============================\r\n");

        debug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
        OutputDebugStringW(L"==============================================================================================\r\n");


        debug->Release();
    }

    ID3D11Device* Renderer::GetDevice() const
    {
        mDevice->AddRef();
        return mDevice;
    }

    ID3D11DeviceContext* Renderer::GetDeviceContext() const
    {
        mDeviceContext->AddRef();
        return mDeviceContext;
    }

    BufferManager* const Renderer::GetBufferManager() const
    {
        return mBufferManager;
    }




    Renderer::Renderer()
        : mDefaultTexture(nullptr)
        , mRefCount(1)
        , mDevice(nullptr)
        , mDeviceContext(nullptr)
        , mShaderMapTable{}
        , mVertexShadersList{}
        , mPixelShaderList{}
        , mInputLayoutList{}
        , mSwapChain(nullptr)
        , mDepthStencilTexture(nullptr)
        , mSkyboxDepthStencil(nullptr)
        , mRenderTargetViewList{nullptr}
        , mDepthStencilViewList{ nullptr }
        , mRtvDsMapTable{0}
        , mTexShadow(nullptr)
        , mTexColor(nullptr)
        , mShadowSrv(nullptr)
        , mViewportFull()
        , mViewportTex()
        , mRasterStates{ nullptr }
        , mBufferManager(nullptr)
        , mTextureManager(nullptr)
    {}

    Renderer::~Renderer()
    {
        Cleanup();
    }

    HRESULT Renderer::compileShaderFromFile(
        const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
    {
        HRESULT result = S_OK;

        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    #ifdef _DEBUG
        // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
        // Setting this flag improves the shader debugging experience, but still allows 
        // the shaders to be optimized and to run exactly the way they will run in 
        // the release configuration of this program.
        dwShaderFlags |= D3DCOMPILE_DEBUG;

        // Disable optimizations to further improve shader debugging
        dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

        ID3DBlob* pErrorBlob = nullptr;
        result = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
            dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

        if (FAILED(result))
        {
            if (pErrorBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
                pErrorBlob->Release();
            }
            return result;
        }

        if (pErrorBlob)
        {
            pErrorBlob->Release();
        }

        return S_OK;
    }

    HRESULT Renderer::createRasterState()
    {
        HRESULT result = S_OK;
        // 기본 래스터 스테이트
        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = false;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32>(eRasterType::Basic)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for basic");
        }
        // 아웃라인용 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_FRONT;
      //  rasterDesc.CullMode = D3D11_CULL_BACK;
        rasterDesc.FrontCounterClockwise = false;
        // TODO: msdn 읽어보고 설정.
        rasterDesc.DepthBias = 1;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32>(eRasterType::Outline)]);
        if(FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for outline");
        }

        // 스카이박스용 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.FrontCounterClockwise = true;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32>(eRasterType::Skybox)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for outline");
        }

        // back-culling 래스터 스테이트
        rasterDesc.CullMode = D3D11_CULL_BACK;
        result = mDevice->CreateRasterizerState(&rasterDesc, &mRasterStates[static_cast<uint32>(eRasterType::CullBack)]);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create RasterState for back face culling");
        }

        return result;
    }

    HRESULT Renderer::createSamplerState()
    {
        const D3D11_SAMPLER_DESC SamplerDescTable[static_cast<uint8_t>(eSamplerType::SamplerCount)] =
        {
                {D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, 0, 0, D3D11_COMPARISON_NEVER, {}, 0, D3D11_FLOAT32_MAX}
        };

        for (uint8_t sampler = 0; sampler < static_cast<uint8_t>(eSamplerType::SamplerCount); ++sampler)
        {
            if (FAILED(mDevice->CreateSamplerState(&SamplerDescTable[sampler], &mSamplerState[sampler])))
            {
                ASSERT(false, "Failed to create SamplerState. check the desc options. failedIndex(%u)", sampler);
                return E_FAIL;
            }
        }

        return S_OK;
    }

    HRESULT Renderer::createPresetConstantBuffers()
    {
        constexpr ConstantBufferMap cbMapTable[static_cast<uint8_t>(eCbType::NumConstantBuffer)] =
            {
                {eCbType::CbWorld, sizeof(CbWorld)},
                {eCbType::CbViewProj, sizeof(CbViewProj)},
                {eCbType::CbLightViewProjMatrix, sizeof(CbLightViewProjMatrix)},
                {eCbType::CbCameraPosition, sizeof(CbCameraPosition)},
                {eCbType::CbOutlineProperty, sizeof(CbOutlineProperty)},
                {eCbType::CbLightProperty, sizeof(CbLightProperty)},
                {eCbType::CbMaterial, sizeof(CbMaterial)},
                {eCbType::CbColor, sizeof(CbMaterial)},
            };

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        HRESULT result = {};
        for (uint8_t index = 0; index < static_cast<uint8_t>(eCbType::NumConstantBuffer); ++index)
        {
            desc.ByteWidth = cbMapTable[index].ByteWidth;
            result = CreateConstantBuffer(desc, &mCbList[index]);
            if (FAILED(result))
            {
                break;
            }
        }
        return result;
    }


    Renderer* Renderer::GetInstance()
    {
        if(!mInstance)
        {
            mInstance = new Renderer();
        }
        return mInstance;
    }

    HashID Renderer::GetBlendStateHash(D3D11_BLEND_DESC& desc)
    {
        HashID hash = 0;
        hash |= static_cast<uint32_t>(desc.RenderTarget[0].BlendEnable);
        hash |= (desc.RenderTarget[0].SrcBlend << 1);
        hash |= (desc.RenderTarget[0].DestBlend << 6);
        hash |= (desc.RenderTarget[0].SrcBlendAlpha << 11);
        hash |= (desc.RenderTarget[0].DestBlendAlpha << 16);
        hash |= (desc.RenderTarget[0].RenderTargetWriteMask << 21);
        hash |= (desc.RenderTarget[0].BlendOp << 26);
        hash |= (desc.RenderTarget[0].BlendOpAlpha << 29);

        return hash;
    }

    void Renderer::SetManagers(BufferManager* const bufferManager, TextureManager* const textureManager)
    {
        ASSERT(bufferManager, "bufferManager is nullptr");
        ASSERT(textureManager, "textureManager is nullptr");
        mBufferManager = bufferManager;
        mTextureManager = textureManager;
    }

    HRESULT Renderer::CreateDeviceAndSetup(
        DXGI_SWAP_CHAIN_DESC& swapChainDesc
        , uint32              width
        , uint32              height
        , bool                bDebugMode)
    {


        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE
        };
        UINT32 numDriverTypes = ARRAYSIZE(driverTypes);

        // gpu가 지원하는 최신버전으로 맞춰질 것이다.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };
        UINT32 numFeatureLevels = ARRAYSIZE(featureLevels);

        UINT32 createDeviceFlag = 0;
        if (bDebugMode)
        {
            createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
        }


        // 왠만하면 D3D_DRIVER_TYPE_HARDWARE로 정해질 것임.
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT result;
        for (UINT32 driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
        {
            result = D3D11CreateDeviceAndSwapChain(nullptr, driverTypes[driverTypeIndex], nullptr, createDeviceFlag, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapChainDesc, &mSwapChain, &mDevice, &featureLevel, &mDeviceContext);
            if(SUCCEEDED(result))
            {
                break;
            }
        }

        if (FAILED(result))
        {
            ASSERT(false, "failed to create device and swapchain");
        }

        SET_PRIVATE_DATA(mDevice, "Renderer::mDevice");
        SET_PRIVATE_DATA(mDeviceContext, "Renderer::mDeviceContext");
        SET_PRIVATE_DATA(mSwapChain, "Renderer::mSwapChain");


        // setup back buffer

        // 백버퍼를 얻어와서 렌더타겟으로 설정하는 부분
        ID3D11Texture2D* backBuffer = nullptr;
        // 데스크탑 버전에서는 기본적으로 스왑체인은 하나의 백버퍼를 가지고 있고, uwp는 만들어줘야한다.
        result = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        // view가 붙은것과 아닌것의 차이가 있다.
        // 어떤 리소스가 있고 그것을 사용하기 위한 파생 인터페이스. 보통 읽기전용으로 동작한다.
        // 9과달리 11에서는 어떤 원천 리소스를 가지고 어떤 방식으로 사용하냐에 따라 인터페이스가 다 나뉘어 있다고 한다.
        result = CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)]);
        // 백버퍼의 역할은 렌더타겟으로 지정해주는 것이 끝이다.
        // refCount이기 때문에 바로 사라지는건 아니고, gRenderTargetView가 백버퍼를 가지기 때문에 이후에 사라질 것.
        backBuffer->Release();

        SET_PRIVATE_DATA(mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Default)], "eRenderTarget::Default");

        if (FAILED(result))
        {
            return E_FAIL;
        }

        // output merger, 그래픽 버퍼에 써넣는 일을 할 때 OM이 붙음.
        /*
         * 이부분 렌더타겟에 바인딩 되지 않는데 왜 호출했는지 다시 조사 필요함
         */

        mViewportFull.Width = (FLOAT)width;
        mViewportFull.Height = (FLOAT)height;
        // 보통 0~1 값으로 지정한다.
        mViewportFull.MinDepth = 0.0f;
        mViewportFull.MaxDepth = 1.0f;
        mViewportFull.TopLeftX = 0;
        mViewportFull.TopLeftY = 0;
        // 렌더링될 영역을 지정. s가 붙으니까 여러개 지정가능.(모델링 프로그램을 생각)
        // rasterizer stage
        mDeviceContext->RSSetViewports(1, &mViewportFull);

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
        depthDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
        depthDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
        depthDesc.MiscFlags = 0;

        CreateTexture2D(depthDesc, &mDepthStencilTexture, "Renderer::DepthStencilTexture");

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = depthDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        result = CreateDepthStencilView(mDepthStencilTexture, &depthStencilViewDesc, &mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)]);
        if (FAILED(result))
        {
            ASSERT(false, "mDepthStencilView 생성 실패");
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)], "eRenderTarget::Default");

        uint8 index = static_cast<uint8_t>(eRenderTarget::Default);
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].RenderTargetIndex = index;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].DepthStencilIndex = index;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Default)].NumViews = 1U;



        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, &mSkyboxDepthStencil);
        if (FAILED(result))
        {
            return E_FAIL;
        }

        result = createRasterState();
        if(FAILED(result))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    bool Renderer::initialize(HWND handleWindow, int16_t width, int16_t height, int16_t frameRate)
    {
        HRESULT result = S_OK;

        // TODO: 개선 경고 메세지 관련하여 조사하고 개선 필요 함
        DXGI_SWAP_CHAIN_DESC swapDesc;
        ZeroMemory(&swapDesc, sizeof(swapDesc));
        swapDesc.BufferCount = 1;
        swapDesc.BufferDesc.Width = width;
        swapDesc.BufferDesc.Height = height;
        swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapDesc.BufferDesc.RefreshRate.Numerator = frameRate;
        swapDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.OutputWindow = handleWindow;
        swapDesc.SampleDesc.Count = 1;
        swapDesc.SampleDesc.Quality = 0;
        swapDesc.Windowed = TRUE;

        result = CreateDeviceAndSetup(swapDesc, width, height, true);
        if (FAILED(result))
        {
            ASSERT(false, "모델데이터 초기화 실패 SetupGeometry");
            return false;
        }

        // set default resources

        D3D11_SHADER_RESOURCE_VIEW_DESC texDefaultDesc;
        texDefaultDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDefaultDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        texDefaultDesc.Texture2D.MipLevels = 1;
        texDefaultDesc.Texture2D.MostDetailedMip = 0;

        // TODO: HRESULT를 에러 메세지로 변환하여 출력해주는 로그 클래스도 만드는 게 좋을 것 같다.(그래야 result를 받는 의미가 있을 듯)
        result = CreateTextureResource(L"AssetData/textures/default.png", WIC_FLAGS_NONE, texDefaultDesc, &mDefaultTexture);
        if (FAILED(result))
        {
            ASSERT(false, "FAIL : CreateTextureResource - default texture");
            return false;
        }
        SET_PRIVATE_DATA(mDefaultTexture, "DefaultTexture");

        result = CreateShadowRenderTarget();
        if (FAILED(result))
        {
            ASSERT(false, "FAIL : FAIL : CreateShadowRenderTarget");
            return false;
        }

        result = setupShaders();
        if (FAILED(result))
        {
            ASSERT(false, "FAIL : FAIL : setupShaders");
            return false;
        }

        result = createSamplerState();
        if (FAILED(result))
        {
            ASSERT(false, "FAIL : FAIL : createSamplerState");
            return false;
        }

        result = createPresetConstantBuffers();
        if (FAILED(result))
        {
            ASSERT(false, "FAIL : FAIL : createPresetConstantBuffers");
            return false;
        }

        return true;
    }

    HRESULT Renderer::CreateInputLayout(const WCHAR* const path, D3D11_INPUT_ELEMENT_DESC* const desc,
        uint32 numDescElements, eInputLayout type, ID3D11InputLayout** const outInputLayout)
    {

        ASSERT(outInputLayout != nullptr, "do not pass nullptr");
        ID3D11VertexShader* dummyShader = nullptr;
        ID3DBlob* blob = nullptr;
        HRESULT result = compileShaderFromFile(path, "main", "vs_5_0", &blob);
        if (FAILED(result))
        {
            ASSERT(false, "failed to compile vertex shader : compileShaderFromFile");
            return E_FAIL;
        }

        result = mDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &dummyShader);
        if (FAILED(result))
        {
            ASSERT(false, "failed to create InputLayout : CreateInputLayout");
            return E_FAIL;
        }

        result = mDevice->CreateInputLayout(desc, numDescElements, blob->GetBufferPointer(), blob->GetBufferSize(), &(*outInputLayout));
        if (FAILED(result))
        {
            ASSERT(false, "failed to create InputLayout : CreateInputLayout");
            return E_FAIL;
        }

        blob->Release();
        SAFETY_RELEASE(dummyShader);

        return result;
    }

    HRESULT Renderer::CreateVertexShader(
        const WCHAR* const path, ID3D11VertexShader** const outVertexShader)
    {
        ASSERT(outVertexShader != nullptr, "do not pass nullptr");

        ID3DBlob* blob = nullptr;
        HRESULT result = compileShaderFromFile(path, "main", "vs_5_0", &blob);
        if (FAILED(result))
        {
            ASSERT(false, "failed to compile vertex shader : compileShaderFromFile");
            return E_FAIL;
        }

        result = mDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &(*outVertexShader));
        if (FAILED(result))
        {
            ASSERT(false, "failed to create InputLayout : CreateInputLayout");
            return E_FAIL;
        }

        blob->Release();

        return result;
    }

    HRESULT Renderer::CreatePixelShader(
        const WCHAR* const path, ID3D11PixelShader** const outPixelShader)
    {
        ASSERT(outPixelShader != nullptr, "do not pass nullptr");

        ID3DBlob* blob = nullptr;
        // PS_Lighting
        HRESULT result = compileShaderFromFile(path, "main", "ps_5_0", &blob);
        if (FAILED(result))
        {
            ASSERT(false, "failed to compile pixel shader : compileShaderFromFile");
            return E_FAIL;
        }

        result = mDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &(*outPixelShader));
        blob->Release();
        if (FAILED(result))
        {
            return result;
        }

        return result;
    }

    HRESULT Renderer::CreateBlendState(D3D11_BLEND_DESC& desc, HashID& outHash)
    {
        outHash = GetBlendStateHash(desc);
        if (mBlendStateMap.find(outHash) == mBlendStateMap.end())
        {
            ID3D11BlendState* newBlendState = nullptr;
            if (FAILED(mDevice->CreateBlendState(&desc, &newBlendState)))
            {
                ASSERT(false, "failed to create BlendState");
                return E_FAIL;
            }
            mBlendStateMap.insert(std::make_pair(outHash, newBlendState));
        }
        else
        {
            // TODO: 나중에 해당 코드 포함하여 모든 Create 함수에 Debug 매크로로만 체크될 수 있도록 분기하면 디버깅용으로 좋을 듯
            const auto& it = mBlendStateMap.find(outHash);
            ASSERT(false, "hash collision detected or double insertion. Hash(%u)", it->first);
        }
        return S_OK;
    }

    HRESULT Renderer::CreateTextureResource(
        const WCHAR* fileName,
        WIC_FLAGS flag,
        D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc,
        ID3D11ShaderResourceView** outShaderResourceView)
    {
        ScratchImage image;
        ID3D11Texture2D* textureResource = nullptr;
        
        HRESULT result = LoadFromWICFile(fileName, flag, nullptr, image);
        if (FAILED(result))
        {
            ASSERT(false, "failed to load imamge file : 이미지 로드 실패");
            goto FAILED;
        }

        result = CreateTexture(mDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), (ID3D11Resource**)(&textureResource));
        if (FAILED(result))
        {
            ASSERT(false, "failed to create TextureResource : gTextureResource 생성 실패");
            goto FAILED;
        }
        D3D11_TEXTURE2D_DESC desc;
        textureResource->GetDesc(&desc);
        srvDesc.Format = desc.Format;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        result = mDevice->CreateShaderResourceView((ID3D11Resource*)textureResource, &srvDesc, &(*outShaderResourceView));
        if (FAILED(result))
        {
            ASSERT(false, "failed to create outShaderResourceView : outShaderResourceView 생성 실패");
            goto FAILED;
        }

        result = S_OK;

    FAILED:
        image.Release();
        SAFETY_RELEASE(textureResource);

        return result;
        
    }

    HRESULT Renderer::CreateRenderTargetView(ID3D11Texture2D* const texture, D3D11_RENDER_TARGET_VIEW_DESC* const desc,
        ID3D11RenderTargetView** outRtv, const char* const debugTag)
    {
        HRESULT result = S_OK;
        ASSERT(texture != nullptr, "texture) do not pass nullptr");
        ASSERT(outRtv != nullptr, "outRtv) do not pass nullptr.");
        ASSERT((*outRtv) == nullptr, "outRtv)pRtv is already initialized.");

        result = mDevice->CreateRenderTargetView(texture, desc, outRtv);
        if(FAILED(result))
        {
            ASSERT(false, "failed to create RenderTargetView: RenderTargetView 생성 실패");
            ASSERT(false, debugTag);
        }

        return result;
    }

    HRESULT Renderer::CreateDepthStencilView(ID3D11Texture2D* const texture, D3D11_DEPTH_STENCIL_VIEW_DESC* const desc,
        ID3D11DepthStencilView** outDs, const char* const debugTag)
    {
        HRESULT result = S_OK;
        ASSERT(texture != nullptr, "texture) do not pass nullptr");
        ASSERT(outDs != nullptr, "outDs) do not pass nullptr.");
        ASSERT((*outDs) == nullptr, "outDs)pOutDs is already initialized.");

        result = mDevice->CreateDepthStencilView(texture, desc, outDs);
        if (FAILED(result))
        {
            ASSERT(false, "failed to create DepthStencilView: DepthStencilView 생성 실패");
            ASSERT(false, debugTag);
        }

        return result;
    }

    HRESULT Renderer::CreateConstantBuffer(D3D11_BUFFER_DESC& desc, ID3D11Buffer** outCb)
    {
        ASSERT(desc.BindFlags == D3D11_BIND_CONSTANT_BUFFER, "desc.BindFlags not bind as Constant-buffer");
        ASSERT(desc.ByteWidth != 0, "desc.ByteWidth is zero");

        HRESULT result = mDevice->CreateBuffer(&desc, nullptr, outCb);

        return result;
    }

    void Renderer::SetRenderTargetTo(eRenderTarget type)
    {
        RtvDsMap& rtvDs = mRtvDsMapTable[static_cast<uint8_t>(type)];

        mDeviceContext->OMSetRenderTargets(rtvDs.NumViews, &mRenderTargetViewList[rtvDs.RenderTargetIndex], mDepthStencilViewList[rtvDs.DepthStencilIndex]);
    }

    void Renderer::SetInputLayoutTo(eInputLayout type) const
    {
        mDeviceContext->IASetInputLayout(mInputLayoutList[static_cast<uint32>(type)]);
    }

    void Renderer::SetShaderTo(eShader type)
    {
        const ShaderMap& shaderMap = mShaderMapTable[static_cast<uint32_t>(type)];
        mDeviceContext->VSSetShader(mVertexShadersList[static_cast<uint32_t>(shaderMap.VsIndex)], nullptr, 0U);
        mDeviceContext->PSSetShader(mPixelShaderList[static_cast<uint32_t>(shaderMap.PsIndex)], nullptr, 0U);
    }

    void Renderer::SetViewport(bool bFullScreen)
    {
        if (bFullScreen)
        {
            mDeviceContext->RSSetViewports(1U, &mViewportFull);
        }
        else
        {
            mDeviceContext->RSSetViewports(1U, &mViewportTex);
        }
    }

    HRESULT Renderer::CreateShadowRenderTarget()
    {
        /*
         * 렌더 타겟의 사이즈와 거기에 붙인 텍스쳐들의 사이즈는 동일해야 한다.
         * 따라서 텍스쳐만 갈아끼우며 사용하려면 지정한 사이즈와 동일한 사이즈의 텍스쳐로 사용해야 한다.
         * 
         */
       /* uint32_t texWidth = 2048U;
        uint32_t texHeight = 2048U;*/

        uint32_t texWidth = 4096;
        uint32_t texHeight = 4096U;
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = texWidth;
        depthDesc.Height = texHeight;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        depthDesc.SampleDesc.Count = 1;                      // 멀티샘플링 수
        depthDesc.SampleDesc.Quality = 0;                    // 멀티샘플링 퀼리티
        depthDesc.Usage = D3D11_USAGE_DEFAULT;               // 디폴트로 사용
        depthDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        depthDesc.CPUAccessFlags = 0;                        // cpu 액세스 여부
        depthDesc.MiscFlags = 0;

        CreateTexture2D(depthDesc, &mTexColor, "Renderer::mTexColor"); // mTexShadow


        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = depthDesc.Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        HRESULT result = CreateRenderTargetView(mTexColor, &rtvDesc, &mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Shadow)]); // mShadowRtv
        if (FAILED(result))
        {
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mRenderTargetViewList[static_cast<uint8_t>(eRenderTarget::Shadow)], "eRenderTarget::Shadow");
        SAFETY_RELEASE(mTexColor);

        mViewportTex.Width    = static_cast<float>(texWidth);
        mViewportTex.Height   = static_cast<float>(texHeight);
        mViewportTex.MinDepth = 0.0f;
        mViewportTex.MaxDepth = 1.0f;
        mViewportTex.TopLeftX = 0;
        mViewportTex.TopLeftY = 0;
        // 렌더링될 영역을 지정. s가 붙으니까 여러개 지정가능.(모델링 프로그램을 생각)
        // rasterizer stage
        mDeviceContext->RSSetViewports(1, &mViewportTex); // mViewportForTex TODO: 이렇게되면 기존 viewport도 변수화 해야한다.
     
        depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        CreateTexture2D(depthDesc, &mTexShadow, "Renderer::mTexShadow"); // mTexShadow

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        result = CreateDepthStencilView(mTexShadow, &depthStencilViewDesc, &mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Shadow)]); // mShadowDsv
        if (FAILED(result))
        {
            ASSERT(false, "mShadowDsv 생성 실패");
            return E_FAIL;
        }
        SET_PRIVATE_DATA(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Shadow)], "eRenderTarget::Shadow");

        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].RenderTargetIndex = 1U;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].DepthStencilIndex = 1U;
        mRtvDsMapTable[static_cast<uint8_t>(eRenderTarget::Shadow)].NumViews = 1U;


        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        result = mDevice->CreateShaderResourceView(mTexShadow, &desc, &mShadowSrv);
        if (FAILED(result))
        {
            ASSERT(false, "Failed to create mTexShadow");
        }

        SAFETY_RELEASE(mTexShadow);

        return S_OK;
    }

    HRESULT Renderer::CreateTexture2D(D3D11_TEXTURE2D_DESC& desc, ID3D11Texture2D** outTex, const char* tag)
    {
        ASSERT(*outTex == nullptr, "pass nullptr before create texture.");
        if(!tag)
        {
            tag = "Renderer::UNKNOWN_TEXTURE";
        }

        HRESULT result = mDevice->CreateTexture2D(&desc, nullptr, &(*outTex));
        if (FAILED(result))
        {
            ASSERT(false, "dRenderer::MyCreateTexture ) 텍스처 생성 실패");
            result = E_FAIL;
        }
        SET_PRIVATE_DATA((*outTex), tag);
        return result;
    }

    void Renderer::BindVertexBuffer(uint32_t stride, uint32_t offset)
    {
        ID3D11Buffer* const vertexBuffer = mBufferManager->GetVertexBuffer();
        mDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    }

    void Renderer::BindIndexBuffer(uint32_t offset)
    {
        ID3D11Buffer* const indexBuffer = mBufferManager->GetIndexBuffer();
        // TODO: 나중에 BufferManager가 format을 가지도록 하는게 관리에 좋을 것으로 보임.
        mDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, offset);
    }

    void Renderer::BindSamplerToPsByType(uint32_t slot, eSamplerType type) const
    {
        mDeviceContext->PSSetSamplers(slot, 1, &mSamplerState[static_cast<int32_t>(type)]);
    }

    void Renderer::BindBlendStateByHash(HashID hash, const float* const blendFactors, uint32_t mask)
    {
        const auto& it = mBlendStateMap.find(hash);
        if (it != mBlendStateMap.end())
        {
            mDeviceContext->OMSetBlendState(it->second, blendFactors, mask);
        }
        else
        {
            ASSERT(false, "no blendState to bind. Hash(%u)", hash);
        }
    }

    void Renderer::BindTextureToPs(uint32_t slot, HashID textureHash) const
    {
        ASSERT(textureHash > 0, "invalid hash. hash(%u)", textureHash);
        ID3D11ShaderResourceView* srv = mTextureManager->GetTextureByHash(textureHash);
        if(srv)
        {
            mDeviceContext->PSSetShaderResources(slot, 1, &srv);
        }
    }

    void Renderer::BindShadowTextureToPs(uint32_t slot) const
    {
        mDeviceContext->PSSetShaderResources(slot, 1, &mShadowSrv);
    }

    void Renderer::BindDefaultTextureToPs(uint32_t slot) const
    {
        mDeviceContext->PSSetShaderResources(slot, 1, &mDefaultTexture);
    }

    void Renderer::UnbindTexturePs(uint32_t slot) const
    {
        ID3D11ShaderResourceView* unbindSRV = nullptr;
        mDeviceContext->PSSetShaderResources(slot, 1, &unbindSRV);
    }

    void Renderer::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology) const
    {
        mDeviceContext->IASetPrimitiveTopology(topology);
    }

    void Renderer::SetRasterState(eRasterType type)
    {
        mDeviceContext->RSSetState(mRasterStates[static_cast<uint32>(type)]);
    }

    void Renderer::SetDepthStencilState(bool bSkybox)
    {
        if(bSkybox)
        {
            mDeviceContext->OMSetDepthStencilState(mSkyboxDepthStencil, 0);
        }
        else
        {
            mDeviceContext->OMSetDepthStencilState(nullptr, 0);
        }
    }

    void Renderer::ClearScreenAndDepth(eRenderTarget type)
    {
        constexpr float CLEAR_COLOR[] = { 0.4f, 0.6f, 1.0f, 1.0f };
        RtvDsMap rtvDs = mRtvDsMapTable[static_cast<uint8_t>(type)];

        mDeviceContext->ClearRenderTargetView(mRenderTargetViewList[rtvDs.RenderTargetIndex], CLEAR_COLOR);
        mDeviceContext->ClearDepthStencilView(mDepthStencilViewList[rtvDs.DepthStencilIndex], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    void Renderer::ClearDepthBuffer()
    {
        mDeviceContext->ClearDepthStencilView(mDepthStencilViewList[static_cast<uint8_t>(eRenderTarget::Default)], D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    void Renderer::Present() const
    {
        mSwapChain->Present(0, 0);
    }

    HRESULT Renderer::setupShaders()
    {

        D3D11_INPUT_ELEMENT_DESC layoutPTNDesc[] =
        {
            { "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
            { "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, 12U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
            { "NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 20U, D3D11_INPUT_PER_VERTEX_DATA, 0U}
        };

        const wchar_t* InputLayoutSourceList[] =
        {
            L"Renderer/Shaders/LayoutPTN.hlsl",
            L"Renderer/Shaders/LayoutPT.hlsl",
            L"Renderer/Shaders/LayoutP.hlsl",
        };

        const wchar_t* VertexShaderSourceList[] =
        {
            L"Renderer/Shaders/VsOutline.hlsl",
            L"Renderer/Shaders/VsBasicWithShadow.hlsl",
            L"Renderer/Shaders/VsSimple.hlsl",
            L"Renderer/Shaders/VsSkybox.hlsl",
            L"Renderer/Shaders/VsRenderToTexture.hlsl",
        };
        const wchar_t* PixelShaderSourceList[] =
        {
            L"Renderer/Shaders/PsOutline.hlsl",
            L"Renderer/Shaders/PsBasicWithShadow.hlsl",
            L"Renderer/Shaders/PsShadow.hlsl",
            L"Renderer/Shaders/PsRenderToTexture.hlsl",
            L"Renderer/Shaders/PsSkybox.hlsl",
            L"Renderer/Shaders/PsColor.hlsl"
        };

        struct PixelShaderContainer
        {
            ePixelShader ListIndex;
            uint32_t     SourceIndex;
        };
        struct VertexShaderContainer
        {
            eVertexShader ListIndex;
            uint32_t      SourceIndex;
        };

        struct InputLayoutContainer
        {
            eInputLayout ListIndex;
            uint32_t      SourceIndex;
            D3D11_INPUT_ELEMENT_DESC* Desc;
            uint32_t numDescElements;
        };

        InputLayoutContainer InputLayoutListMapTable[static_cast<uint8_t>(eInputLayout::NumInputlayout)] =
        {
            { eInputLayout::PTN, 0U, layoutPTNDesc, 3},
            { eInputLayout::PT, 1U, layoutPTNDesc, 2},
            {eInputLayout::P, 2U, layoutPTNDesc, 1},
        };

        constexpr VertexShaderContainer VertexShaderListMapTable[static_cast<uint32_t>(eVertexShader::NumVertexShader)] =
        {
            {eVertexShader::VsBasicWithShadow, 1U},
            { eVertexShader::VsOutline, 0U},
            {eVertexShader::VsSimple, 2U},
            {eVertexShader::VsRenderToTexture, 4U}, // ?
            {eVertexShader::VsSkybox, 3U},
        };

        constexpr PixelShaderContainer PixelShaderListMapTable[static_cast<uint32_t>(ePixelShader::NumPixelShader)] =
        {
            {ePixelShader::PsBasicWithShadow, 1U},
            {ePixelShader::PsOutline, 0U},
            {ePixelShader::PsRenderToTexture, 3U},
            {ePixelShader::PsShadow, 2U},
            {ePixelShader::PsSkybox, 4U},
            {ePixelShader::PsColor, 5U},
        };


        // construct shader mapping table
        constexpr ShaderMap ShaderMapTable[] =
        {
            {eShader::Outline, eVertexShader::VsOutline, ePixelShader::PsOutline}, 
            {eShader::Skybox, eVertexShader::VsSkybox, ePixelShader::PsSkybox}, 
            { eShader::Shadow, eVertexShader::VsSimple, ePixelShader::PsShadow}, // TODO : 개선 예정(셰이더 최적화) - VSSimple을 다른걸로 변경하기.(inputlayout관점 최적화)
            {eShader::BasicWithShadow,  eVertexShader::VsBasicWithShadow, ePixelShader::PsBasicWithShadow},
            {eShader::RenderToTexture,  eVertexShader::VsRenderToTexture, ePixelShader::PsRenderToTexture}, // TODO : 개선 예정(셰이더 최적화)
            {eShader::Color,  eVertexShader::VsSimple, ePixelShader::PsColor},
        };

        static_assert(sizeof(mShaderMapTable) == sizeof(ShaderMapTable), "mShaderMapTable and ShaderMapTable MUST be same size.");
        memcpy(mShaderMapTable, ShaderMapTable, sizeof(mShaderMapTable));


        HRESULT result = S_OK;

        // input layout
        for (const InputLayoutContainer& layout : InputLayoutListMapTable)
        {
            ASSERT(mInputLayoutList[static_cast<uint32_t>(layout.ListIndex)] == nullptr, "The InputLayout-Mapping List may be incorrect or not initialized as nullptr.");

            result = Renderer::GetInstance()->CreateInputLayout(InputLayoutSourceList[layout.SourceIndex], layout.Desc, layout.numDescElements, layout.ListIndex, &mInputLayoutList[static_cast<uint32_t>(layout.ListIndex)]);
            if (FAILED(result))
            {
                ASSERT(false, "To Create InputLayout FAILED");
            }
        }

        for (const VertexShaderContainer& vs : VertexShaderListMapTable)
        {
            ASSERT(mVertexShadersList[static_cast<uint32_t>(vs.ListIndex)] == nullptr, "The VS-Mapping List may be incorrect or not initialized as nullptr.");

            result = CreateVertexShader(VertexShaderSourceList[vs.SourceIndex], &mVertexShadersList[static_cast<uint32_t>(vs.ListIndex)]);
            if (FAILED(result))
            {
                ASSERT(false, "To Compile Vertex Shader FAILED");
            }
        }

        for (const PixelShaderContainer& ps : PixelShaderListMapTable)
        {
            ASSERT(mPixelShaderList[static_cast<uint32_t>(ps.ListIndex)] == nullptr, "The PS-Mapping List may be incorrect or not initialized as nullptr.");

            result = Renderer::GetInstance()->CreatePixelShader(PixelShaderSourceList[ps.SourceIndex], &mPixelShaderList[static_cast<uint32_t>(ps.ListIndex)]);
            if (FAILED(result))
            {
                ASSERT(false, "To Compile Pixel Shader FAILED");
            }
        }

        return result;
    }

    bool Renderer::CheckDeviceLost(bool& outIsReInitialize) const
    {
        bool bTerminateProgram = false;
        switch (mDevice->GetDeviceRemovedReason())
        {
        case DXGI_ERROR_DEVICE_HUNG:
        case DXGI_ERROR_DEVICE_RESET:
        {
            outIsReInitialize = true;
            break;
        }
        case S_OK:
        {
            outIsReInitialize = false;
            break;
        }
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        case DXGI_ERROR_INVALID_CALL:
        {
            bTerminateProgram = true;
            outIsReInitialize = false;
            break;
        }
        default:
        {
            // MEMO: 다른 에러는 일단 돌게 만드는데, 계속 써보면서 다른 에러처리를 추가하도록 함
            outIsReInitialize = false;
            break;
        }
        }
        return bTerminateProgram;
    }

    void Renderer::Cleanup()
    {
        for (uint32 i = 0; i < static_cast<uint32>(eCbType::NumConstantBuffer); ++i)
        {
            SAFETY_RELEASE(mCbList[i]);
        }

        for (uint32 i = 0; i < static_cast<uint32>(eRasterType::NumRaster); ++i)
        {
            SAFETY_RELEASE(mRasterStates[i]);
        }

        for (uint32 i = 0; i < static_cast<uint32>(eSamplerType::SamplerCount); ++i)
        {
            SAFETY_RELEASE(mSamplerState[i]);
        }

        for (auto& it : mBlendStateMap)
        {
            SAFETY_RELEASE(it.second);
        }
        mBlendStateMap.clear();

        for (uint32 i = 0; i < static_cast<uint32>(eVertexShader::NumVertexShader); ++i)
        {
            SAFETY_RELEASE(mVertexShadersList[i]);
        }

        for (uint32 i = 0; i < static_cast<uint32>(ePixelShader::NumPixelShader); ++i)
        {
            SAFETY_RELEASE(mPixelShaderList[i]);
        }

        for (uint32 i = 0; i < static_cast<uint32>(eInputLayout::NumInputlayout); ++i)
        {
            SAFETY_RELEASE(mInputLayoutList[i]);
        }

        for (uint32 i = 0; i < static_cast<uint8_t>(eRenderTarget::NumRenderTarget); ++i)
        {
            SAFETY_RELEASE(mRenderTargetViewList[i]);
            SAFETY_RELEASE(mDepthStencilViewList[i]);
        }

        SAFETY_RELEASE(mDefaultTexture);
        SAFETY_RELEASE(mTexShadow);
        SAFETY_RELEASE(mTexColor);
        SAFETY_RELEASE(mShadowSrv);
        SAFETY_RELEASE(mDepthStencilTexture);
        SAFETY_RELEASE(mSkyboxDepthStencil);
        SAFETY_RELEASE(mSwapChain);
        SAFETY_RELEASE(mDeviceContext);
        SAFETY_RELEASE(mDevice);
    }

    ULONG Renderer::AddRef()
    {
        ++mRefCount;
        return mRefCount;
    }

    ULONG Renderer::Release()
    {
        --mRefCount;
        if(mRefCount <= 0)
        {
            delete this;
        }
        return mRefCount;
    }

    HRESULT Renderer::QueryInterface(const IID& riid, void** ppvObject)
    {
        ASSERT(false, "not implements");
        return E_FAIL;
    }

    void Renderer::UpdateCB(eCbType type, void* data) const
    {
        mDeviceContext->UpdateSubresource(mCbList[static_cast<uint32_t>(type)], 0U, nullptr, data, 0U, 0U);
    }


    void Renderer::BindCbToVsByType(uint32_t slot, uint32_t numBuffer, eCbType type) const
    {
        mDeviceContext->VSSetConstantBuffers(slot, numBuffer, &mCbList[static_cast<uint32_t>(type)]);
    }


    void Renderer::BindCbToPs(uint32_t slot, uint32_t numBuffer, eCbType type) const
    {
        mDeviceContext->PSSetConstantBuffers(slot, numBuffer, &mCbList[static_cast<uint32_t>(type)]);
    }
}
