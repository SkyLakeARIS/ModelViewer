
HRESULT LoadTextureFromFileAndCreateResource(ID3D11Device* device, const WCHAR* fileName, const D3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc, ID3D11ShaderResourceView** outShaderResourceView)
{
    ScratchImage image;
    ID3D11Resource* textureResource = nullptr;

    HRESULT result = LoadFromWICFile(fileName, WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        ASSERT(false, "이미지 로드 실패");
        goto FAILED;
    }

    result = CreateTexture(device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &textureResource);
    if (FAILED(result))
    {
        OutputDebugStringW(std::to_wstring(result).c_str());
        ASSERT(false, "CreateTexture gTextureResource 생성 실패");
        goto FAILED;
    }


    result = device->CreateShaderResourceView(textureResource, &srvDesc, &(*outShaderResourceView));
    if (FAILED(result))
    {
        ASSERT(false, "outShaderResourceView 생성 실패");
        goto FAILED;
    }

    result = S_OK;

FAILED:
    image.Release();
    SAFETY_RELEASE(textureResource);

    return result;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
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



#ifdef _DEBUG
void CheckLiveObjects()
{
    HMODULE dxgidebugdll = GetModuleHandleW(L"dxgidebug.dll");
    ASSERT(dxgidebugdll != NULL, "dxgidebug.dll 로드 실패");

    decltype(&DXGIGetDebugInterface) GetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(dxgidebugdll, "DXGIGetDebugInterface"));

    IDXGIDebug* debug;

    GetDebugInterface(IID_PPV_ARGS(&debug));

    OutputDebugStringW(L"====================== Direct3D Object ref count 메모리 누수 체크 ======================\r\n");
    debug->ReportLiveObjects(DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL);
    OutputDebugStringW(L"====================== 반환되지 않은 IUnknown 객체가 있을경우 위에 나타납니다. ======================\r\n");


    debug->Release();
}
#endif


