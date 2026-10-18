#include "ShaderManager.h"
#include "../Renderer.h"
#include "../../Util/Macro.h"
#include "../Resources/Material.h"

namespace renderer
{
    ShaderManager::ShaderManager(ID3D11Device& device, ID3D11DeviceContext& deviceContext)
        : mDevice(&device)
        , mDeviceContext(&deviceContext)
        , mShaderMapTable{}
        , mVertexShadersList{}
        , mPixelShaderList{}
        , mInputLayoutList{}
    {}

    ShaderManager::ShaderManager()
    {
	    
    }

    ShaderManager::~ShaderManager()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(eCbType::ConstantBufferCount); ++i)
        {
            SAFETY_RELEASE(mCbList[i].Buffer);
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(eVertexShader::VertexShaderCount); ++i)
        {
            SAFETY_RELEASE(mVertexShadersList[i]);
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(ePixelShader::PixelShaderCount); ++i)
        {
            SAFETY_RELEASE(mPixelShaderList[i]);
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(eVertexFormat::FormatCount); ++i)
        {
            SAFETY_RELEASE(mInputLayoutList[i]);
        }
        mDevice = nullptr;
        mDeviceContext = nullptr;
    }

    bool ShaderManager::SetupShadersVk(VkRenderer& renderer)
    {
        struct VertexShaderSourceInfo
        {
            eVertexShader Type;
            const char* SourcePath;
        };

        constexpr VertexShaderSourceInfo VertexShaderSourceList[] =
        {
            {eVertexShader::VsOutline, "Renderer/Shader/Precompiled/VsOutline.spv"},
            {eVertexShader::VsBasicWithShadow, "Renderer/Shader/Precompiled/VsBasicWithShadow.spv"},
            {eVertexShader::VsSimple, "Renderer/Shader/Precompiled/VsSimple.spv"},
            {eVertexShader::VsSkybox, "Renderer/Shader/Precompiled/VsSkybox.spv"},
            {eVertexShader::VsTexture, "Renderer/Shader/Precompiled/VsTexture.spv"},
            {eVertexShader::VsScreen, "Renderer/Shader/Precompiled/VsScreen.spv"},
            {eVertexShader::VsShadow, "Renderer/Shader/Precompiled/VsShadow.spv"},
            {eVertexShader::VsDeferred, "Renderer/Shader/Precompiled/VsDeferred.spv"},
            {eVertexShader::VsDebugColor, "Renderer/Shader/Precompiled/VsDebugColor.spv"},
        };
        static_assert(std::size(VertexShaderSourceList) == static_cast<uint8_t>(eVertexShader::VertexShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");


        struct PixelShaderSourceInfo
        {
            ePixelShader Type;
            const char* SourcePath;
        };

        constexpr PixelShaderSourceInfo PixelShaderSourceList[] =
        {
            {ePixelShader::PsOutline, "Renderer/Shader/Precompiled/PsOutline.spv"},
            {ePixelShader::PsBasicWithShadow, "Renderer/Shader/Precompiled/PsBasicWithShadow.spv"},
            {ePixelShader::PsShadow, "Renderer/Shader/Precompiled/PsShadow.spv"},
            {ePixelShader::PsTexture, "Renderer/Shader/Precompiled/PsTexture.spv"},
            {ePixelShader::PsSkybox, "Renderer/Shader/Precompiled/PsSkybox.spv"},
            {ePixelShader::PsColor, "Renderer/Shader/Precompiled/PsColor.spv"},
            {ePixelShader::PsDeferred, "Renderer/Shader/Precompiled/PsDeferred.spv"},
            {ePixelShader::PsTextureMRT, "Renderer/Shader/Precompiled/PsTextureMRT.spv"},
            {ePixelShader::PsSimple, "Renderer/Shader/Precompiled/PsSimple.spv"},
        };
        static_assert(std::size(PixelShaderSourceList) == static_cast<uint8_t>(ePixelShader::PixelShaderCount), "열거형값과 List의 요소가 서로 일치해야 합니다.");

        HashID vsHashes[static_cast<uint8_t>(eVertexShader::VertexShaderCount)];

        for (const VertexShaderSourceInfo shaderInfo : VertexShaderSourceList)
        {
            if(renderer.CreateShaderStage(shaderInfo.SourcePath, false, vsHashes[static_cast<uint8_t>(shaderInfo.Type)]) == false)
            {
                return false;
            }
        }

        HashID psHashes[static_cast<uint8_t>(ePixelShader::PixelShaderCount)];
        for (const PixelShaderSourceInfo shaderInfo : PixelShaderSourceList)
        {
            if(renderer.CreateShaderStage(shaderInfo.SourcePath, true, psHashes[static_cast<uint8_t>(shaderInfo.Type)]) == false)
            {
                return false;
            }
        }

        // construct shader mapping table
        const ShaderStageHash ShaderMapTable[] =
        {
            {eShader::Outline, vsHashes[static_cast<uint8_t>(eVertexShader::VsOutline)], psHashes[static_cast<uint8_t>(ePixelShader::PsOutline)]},
            {eShader::Skybox, vsHashes[static_cast<uint8_t>(eVertexShader::VsSkybox)], psHashes[static_cast<uint8_t>(ePixelShader::PsSkybox)]},
            { eShader::Shadow, vsHashes[static_cast<uint8_t>(eVertexShader::VsShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsShadow)]},
            {eShader::BasicWithShadow,  vsHashes[static_cast<uint8_t>(eVertexShader::VsBasicWithShadow)], psHashes[static_cast<uint8_t>(ePixelShader::PsBasicWithShadow)]},
            {eShader::Texture,  vsHashes[static_cast<uint8_t>(eVertexShader::VsTexture)], psHashes[static_cast<uint8_t>(ePixelShader::PsTextureMRT)]},
            {eShader::Color,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::DebugHUD,  vsHashes[static_cast<uint8_t>(eVertexShader::VsScreen)], psHashes[static_cast<uint8_t>(ePixelShader::PsTexture)]},
            {eShader::Deferred,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDeferred)], psHashes[static_cast<uint8_t>(ePixelShader::PsDeferred)]},
            {eShader::DebugColor,  vsHashes[static_cast<uint8_t>(eVertexShader::VsDebugColor)], psHashes[static_cast<uint8_t>(ePixelShader::PsColor)]},
            {eShader::Simple,  vsHashes[static_cast<uint8_t>(eVertexShader::VsSimple)], psHashes[static_cast<uint8_t>(ePixelShader::PsSimple)]},
        };
        static_assert(std::size(ShaderMapTable) == static_cast<uint8_t>(eShader::ShaderCount), "열거형값과 ShaderMapTable의 요소가 서로 일치해야 합니다.");
        static_assert(sizeof(mShaderHashes) == sizeof(ShaderMapTable), "mShaderHashes and ShaderMapTable MUST be same size.");
        memcpy(mShaderHashes, ShaderMapTable, sizeof(mShaderHashes));

        for(const auto& shader : ShaderMapTable)
        {
            renderer.CreateShaderLayout(shader.Type);
        }
        renderer.CreateDescriptorSet();

        return true;
    }

    void ShaderManager::GetStageInfoByType(eShader shader, HashID& outVsHash, HashID& outPsHash)
    {
        outVsHash = mShaderHashes[static_cast<uint8_t>(shader)].VertexShaderHash;
        outPsHash = mShaderHashes[static_cast<uint8_t>(shader)].PixelShaderHash;
    }

    bool ShaderManager::SetupShaders()
    {

        D3D11_INPUT_ELEMENT_DESC layoutPTNDesc[] =
        {
            { "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
            { "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, 12U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
            { "NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 20U, D3D11_INPUT_PER_VERTEX_DATA, 0U}
        };

        const wchar_t* InputLayoutSourceList[] =
        {
            L"Renderer/Shader/ShaderSources/LayoutPTN.hlsl",
            L"Renderer/Shader/ShaderSources/LayoutPT.hlsl",
            L"Renderer/Shader/ShaderSources/LayoutP.hlsl",
        };

        const wchar_t* VertexShaderSourceList[] =
        {
            L"Renderer/Shader/ShaderSources/VsOutline.hlsl",
            L"Renderer/Shader/ShaderSources/VsBasicWithShadow.hlsl",
            L"Renderer/Shader/ShaderSources/VsSimple.hlsl",
            L"Renderer/Shader/ShaderSources/VsSkybox.hlsl",
            L"Renderer/Shader/ShaderSources/VsTexture.hlsl",
            L"Renderer/Shader/ShaderSources/VsScreen.hlsl",
            L"Renderer/Shader/ShaderSources/VsShadow.hlsl",
            L"Renderer/Shader/ShaderSources/VsDeferred.hlsl",
            L"Renderer/Shader/ShaderSources/VsDebugColor.hlsl",
        };
        const wchar_t* PixelShaderSourceList[] =
        {
            L"Renderer/Shader/ShaderSources/PsOutline.hlsl",
            L"Renderer/Shader/ShaderSources/PsBasicWithShadow.hlsl",
            L"Renderer/Shader/ShaderSources/PsShadow.hlsl",
            L"Renderer/Shader/ShaderSources/PsTexture.hlsl",
            L"Renderer/Shader/ShaderSources/PsSkybox.hlsl",
            L"Renderer/Shader/ShaderSources/PsColor.hlsl",
            L"Renderer/Shader/ShaderSources/PsDeferred.hlsl",
            L"Renderer/Shader/ShaderSources/PsTextureMRT.hlsl",
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
            eVertexFormat ListIndex;
            uint32_t      SourceIndex;
            D3D11_INPUT_ELEMENT_DESC* Desc;
            uint32_t numDescElements;
        };

        InputLayoutContainer InputLayoutListMapTable[static_cast<uint8_t>(eVertexFormat::FormatCount)] =
        {
            { eVertexFormat::PTN, 0U, layoutPTNDesc, 3},
            { eVertexFormat::PT, 1U, layoutPTNDesc, 2},
            {eVertexFormat::P, 2U, layoutPTNDesc, 1},
        };

        constexpr VertexShaderContainer VertexShaderListMapTable[static_cast<uint32_t>(eVertexShader::VertexShaderCount)] =
        {
            {eVertexShader::VsBasicWithShadow, 1U},
            { eVertexShader::VsOutline, 0U},
            {eVertexShader::VsSimple, 2U},
            {eVertexShader::VsTexture, 4U}, // ?
            {eVertexShader::VsSkybox, 3U},
            {eVertexShader::VsScreen, 5U},
            {eVertexShader::VsShadow, 6U},
            {eVertexShader::VsDeferred, 7U},
            {eVertexShader::VsDebugColor, 8U},
        };

        constexpr PixelShaderContainer PixelShaderListMapTable[static_cast<uint32_t>(ePixelShader::PixelShaderCount)] =
        {
            {ePixelShader::PsBasicWithShadow, 1U},
            {ePixelShader::PsOutline, 0U},
            {ePixelShader::PsTexture, 3U},
            {ePixelShader::PsShadow, 2U},
            {ePixelShader::PsSkybox, 4U},
            {ePixelShader::PsColor, 5U},
            {ePixelShader::PsDeferred, 6U},
            {ePixelShader::PsTextureMRT, 7U},
        };


        // construct shader mapping table
        constexpr ShaderMap ShaderMapTable[] =
        {
            {eShader::Outline, eVertexShader::VsOutline, ePixelShader::PsOutline},
            {eShader::Skybox, eVertexShader::VsSkybox, ePixelShader::PsSkybox},
            { eShader::Shadow, eVertexShader::VsShadow, ePixelShader::PsShadow},
            {eShader::BasicWithShadow,  eVertexShader::VsBasicWithShadow, ePixelShader::PsBasicWithShadow},
            {eShader::Texture,  eVertexShader::VsTexture, ePixelShader::PsTextureMRT},
            {eShader::Color,  eVertexShader::VsSimple, ePixelShader::PsColor},
            {eShader::DebugHUD,  eVertexShader::VsScreen, ePixelShader::PsTexture},
            {eShader::Deferred,  eVertexShader::VsDeferred, ePixelShader::PsDeferred},
            {eShader::DebugHUD,  eVertexShader::VsDebugColor, ePixelShader::PsColor},
            {eShader::Simple,  eVertexShader::VsSimple, ePixelShader::PsSimple},
        };

        static_assert(sizeof(mShaderMapTable) == sizeof(ShaderMapTable), "mShaderMapTable and ShaderMapTable MUST be same size.");
        memcpy(mShaderMapTable, ShaderMapTable, sizeof(mShaderMapTable));


        bool result = true;

        // input layout
        for (const InputLayoutContainer& layout : InputLayoutListMapTable)
        {
            ASSERT(mInputLayoutList[static_cast<uint32_t>(layout.ListIndex)] == nullptr, "The InputLayout-Mapping List may be incorrect or not initialized as nullptr.");

            result = createInputLayout(InputLayoutSourceList[layout.SourceIndex], layout.Desc, layout.numDescElements, layout.ListIndex, &mInputLayoutList[static_cast<uint32_t>(layout.ListIndex)]);
            if (!result)
            {
                ASSERT(false, "To Create InputLayout FAILED");
            }
        }

        for (const VertexShaderContainer& vs : VertexShaderListMapTable)
        {
            ASSERT(mVertexShadersList[static_cast<uint32_t>(vs.ListIndex)] == nullptr, "The VS-Mapping List may be incorrect or not initialized as nullptr.");

            result = createVertexShader(VertexShaderSourceList[vs.SourceIndex], &mVertexShadersList[static_cast<uint32_t>(vs.ListIndex)]);
            if (!result)
            {
                ASSERT(false, "To Compile Vertex Shader FAILED");
            }
        }

        for (const PixelShaderContainer& ps : PixelShaderListMapTable)
        {
            ASSERT(mPixelShaderList[static_cast<uint32_t>(ps.ListIndex)] == nullptr, "The PS-Mapping List may be incorrect or not initialized as nullptr.");

            result = createPixelShader(PixelShaderSourceList[ps.SourceIndex], &mPixelShaderList[static_cast<uint32_t>(ps.ListIndex)]);
            if (!result)
            {
                ASSERT(false, "To Compile Pixel Shader FAILED");
            }
        }
        return result;
    }

    void ShaderManager::UpdateCB(eCbType type, const void* const data) const
    {
        if (mCbList[static_cast<uint32_t>(type)].Usage == D3D11_USAGE_DYNAMIC)
        {
            // MEMO: Discard하거나, 4-5개 사이즈 여유를 두고 NoOverWrite해도 괜찮을지도.
            D3D11_MAPPED_SUBRESOURCE mappedRes = {};
            mDeviceContext->Map(mCbList[static_cast<uint32_t>(type)].Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);

            (void)memcpy(mappedRes.pData, data, mCbList[static_cast<uint32_t>(type)].ByteWidth);

            mDeviceContext->Unmap(mCbList[static_cast<uint32_t>(type)].Buffer, 0);
        }
        else
        {
            mDeviceContext->UpdateSubresource(mCbList[static_cast<uint32_t>(type)].Buffer, 0U, nullptr, data, 0U, 0U);
        }
    }

    void ShaderManager::UpdateMaterial(eCbType type, const SemiMaterial& material)
    {
        switch (type)
        {
        case eCbType::CbWorld:
        case eCbType::CbViewProj:
        case eCbType::CbLightViewProjMatrix:
        case eCbType::CbCameraPosition:
        case eCbType::CbLightProperty:
        case eCbType::CbOrthoMatrix:
        {
            ASSERT(false, "렌더러가 예약하고 있는 상수버퍼에 업데이트를 시도. 머테리얼로 지정된 슬롯에만 접근해야 합니다. type(%d)", static_cast<uint8_t>(type))
            break;
        }
        case eCbType::CbMaterialFactors:
        {
            const CbMaterialFactors cbMaterialParam = material.Factors;
            UpdateCB(type, reinterpret_cast<const void*>(&cbMaterialParam));
            break;
        }
        case eCbType::CbColor:
        {
            CbColor cbColor = {};
            cbColor.Float3 = material.Factors.Diffuse;
            UpdateCB(type, reinterpret_cast<void*>(&cbColor));
            break;
        }
        case eCbType::CbOutlineProperty:
        {
            CbOutlineProperty CbOutlineProperty = {};
            CbOutlineProperty.Float = material.OutlineWidth;
            UpdateCB(type, reinterpret_cast<void*>(&CbOutlineProperty));
            break;
        }
        case eCbType::ConstantBufferCount:
        default:
        {
            ASSERT(false, "invalid type. 올바르지 않은 타입으로 호출됨. type(%d)", static_cast<uint8_t>(type))
        }
        }
    }

    void ShaderManager::GetShadersByType(eShader             type, ID3D11VertexShader** vs,
                                         ID3D11PixelShader** ps) const
    {
        const ShaderMap& shaderMap = mShaderMapTable[static_cast<uint32_t>(type)];
        *vs = mVertexShadersList[static_cast<uint32_t>(shaderMap.VsIndex)];
        *ps = mPixelShaderList[static_cast<uint32_t>(shaderMap.PsIndex)];
    }

    ID3D11InputLayout* ShaderManager::GetInputLayoutByType(eVertexFormat type) const
    {
        ASSERT(type != eVertexFormat::FormatCount, "올바르지 않은 type 전달. type(%d)", static_cast<uint8_t>(type));
        return mInputLayoutList[static_cast<uint8_t>(type)];
    }

    ID3D11Buffer* ShaderManager::GetConstantBufferByType(eCbType type) const
    {
        ASSERT(type != eCbType::ConstantBufferCount, "올바르지 않은 cb type 전달. type(%d)", static_cast<uint8_t>(type));
        return mCbList[static_cast<uint8_t>(type)].Buffer;
    }

    void ShaderManager::GetMaterialCbBindingDesc(eShader type, MaterialCbBinding& outBindingDesc)
    {
        ASSERT(type != eShader::ShaderCount, "올바르지 않은 셰이더 타입. type(%d)", static_cast<uint8_t>(type));

        constexpr MaterialCbBinding MaterialCbTableEachShader[] =
        {
            { eCbType::CbOutlineProperty,   false,  6 }, // Outline
            { eCbType::CbMaterialFactors,   true,   0 }, // Skybox
            { eCbType::ConstantBufferCount, false, -1 }, // Shadow
            { eCbType::CbMaterialFactors,   true,   0 }, // BasicWithShadow
            { eCbType::CbMaterialFactors,   true,   0 }, // Texture
            { eCbType::CbMaterialFactors,   true,   0 }, // Color
            { eCbType::ConstantBufferCount, false, -1 }, // DebugHUD
            { eCbType::ConstantBufferCount, false, -1 }, // Deferred
            { eCbType::CbMaterialFactors, true, 0 },     // DebugColor
            { eCbType::CbMaterialFactors, true, 0 },     // DebugColor
        };
        static_assert(sizeof(MaterialCbTableEachShader) / sizeof(MaterialCbBinding) == static_cast<uint8_t>(eShader::ShaderCount), "셰이더 수와 Table 수가 맞지 않음.");
        outBindingDesc = MaterialCbTableEachShader[static_cast<uint8_t>(type)];
    }

    void ShaderManager::GetMaterialTextureBindSlots(eShader type, int8_t* const outBindingSlots)
    {
        constexpr int8_t TexBindingSlotsEachShader[static_cast<uint8_t>(eShader::ShaderCount)][static_cast<uint8_t>(eTextureType::TextureTypeCount)] =
        {
            // Diffuse, Normal, Shadow, GColor, GNormal, GPosition, GSpecular, GAmbient
            { -1, -1, -1, -1, -1, -1, -1, -1 }, // Outline
            {  0, -1, -1, -1, -1, -1, -1, -1 }, // Skybox
            { -1, -1, -1, -1, -1, -1, -1, -1 }, // Shadow
            {  0, -1, -1, -1, -1, -1, -1, -1 }, // BasicWithShadow
            {  0, -1, -1, -1, -1, -1, -1, -1 }, // Texture
            { -1, -1, -1, -1, -1, -1, -1, -1 }, // Color
            {  0, -1, -1, -1, -1, -1, -1, -1 }, // DebugHUD
            { -1, -1, 5,  0,  1,  2,  3,  4},   // Deferred
            { -1, -1, -1, -1, -1, -1, -1, -1 }, // DebugColor

        };
        static_assert(sizeof(TexBindingSlotsEachShader) / sizeof(TexBindingSlotsEachShader[0]) == static_cast<uint8_t>(eShader::ShaderCount),
            "셰이더 수와 Table 수가 맞지 않음.");

        (void)memcpy(outBindingSlots, TexBindingSlotsEachShader[static_cast<uint8_t>(type)], sizeof(TexBindingSlotsEachShader[0]));
    }

    void ShaderManager::GetMaterialSamplerBindSlot(eShader type, int8_t& outBindingSlot)
    {
        constexpr int8_t SamplerBindingSlotsEachShader[static_cast<uint8_t>(eShader::ShaderCount)][static_cast<uint8_t>(eSamplerType::SamplerCount)] =
        {
            // AnisotropicWrap
            { -1 }, // Outline
            {  0 }, // Skybox
            { -1 }, // Shadow
            {  0 }, // BasicWithShadow
            {  0 }, // Texture
            { -1 }, // Color
            {  0 }, // DebugHUD
            {  0 }, // Deferred
            { -1 }, // DebugColor
        };
        static_assert(sizeof(SamplerBindingSlotsEachShader) / sizeof(SamplerBindingSlotsEachShader[0]) == static_cast<uint8_t>(eShader::ShaderCount),
            "셰이더 수와 Table 수가 맞지 않음.");

        outBindingSlot = SamplerBindingSlotsEachShader[static_cast<uint8_t>(type)][0];
    }

    bool ShaderManager::createInputLayout(const WCHAR* const path, D3D11_INPUT_ELEMENT_DESC* const desc,
        uint32_t numDescElements, eVertexFormat type, ID3D11InputLayout** const outInputLayout)
    {

        ASSERT(outInputLayout != nullptr, "do not pass nullptr");
        ID3D11VertexShader* dummyShader = nullptr;
        ID3DBlob* blob = nullptr;
        bool result = compileShaderFromFile(path, "main", "vs_5_0", &blob);
        if (!result)
        {
            ASSERT(false, "failed to compile vertex shader : compileShaderFromFile");
            return E_FAIL;
        }

        result = SUCCEEDED(mDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &dummyShader));
        if (!result)
        {
            ASSERT(false, "failed to create vertexShader (CreateInputLayout)");
            return E_FAIL;
        }

        result = SUCCEEDED(mDevice->CreateInputLayout(desc, numDescElements, blob->GetBufferPointer(), blob->GetBufferSize(), &(*outInputLayout)));
        if (!result)
        {
            ASSERT(false, "failed to create InputLayout");
            return E_FAIL;
        }

        blob->Release();
        SAFETY_RELEASE(dummyShader);

        return result;
    }

    bool ShaderManager::createVertexShader(const WCHAR* const path, ID3D11VertexShader** const outVertexShader)
    {
        ASSERT(outVertexShader != nullptr, "do not pass nullptr");

        ID3DBlob* blob = nullptr;
        bool result = compileShaderFromFile(path, "main", "vs_5_0", &blob);
        if (!result)
        {
            ASSERT(false, "failed to compile vertex shader : compileShaderFromFile");
            return result;
        }

        result = SUCCEEDED(mDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &(*outVertexShader)));
        blob->Release();

        return result;
    }

    bool ShaderManager::createPixelShader(const WCHAR* const path, ID3D11PixelShader** const outPixelShader)
    {
        ASSERT(outPixelShader != nullptr, "do not pass nullptr");

        ID3DBlob* blob = nullptr;
        // PS_Lighting
        bool result = compileShaderFromFile(path, "main", "ps_5_0", &blob);
        if (!result)
        {
            ASSERT(false, "failed to compile pixel shader : compileShaderFromFile");
            return result;
        }

        result = SUCCEEDED(mDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &(*outPixelShader)));
        blob->Release();
    
        return result;
    }

    bool ShaderManager::compileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
        ID3DBlob** ppBlobOut)
{
        bool result = true;

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
        result = SUCCEEDED(D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
            dwShaderFlags, 0, ppBlobOut, &pErrorBlob));

        if (!result)
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

        return result;
    }

    bool ShaderManager::CreatePresetConstantBuffers()
    {
        constexpr ConstantBufferMap cbMapTable[static_cast<uint8_t>(eCbType::ConstantBufferCount)] =
        {
            {eCbType::CbWorld, sizeof(CbWorld), D3D11_USAGE_DYNAMIC},
            {eCbType::CbViewProj, sizeof(CbViewProj), D3D11_USAGE_DYNAMIC},
            {eCbType::CbLightViewProjMatrix, sizeof(CbLightViewProjMatrix), D3D11_USAGE_DYNAMIC},
            {eCbType::CbCameraPosition, sizeof(CbCameraPosition), D3D11_USAGE_DYNAMIC},
            {eCbType::CbOutlineProperty, sizeof(CbOutlineProperty), D3D11_USAGE_DEFAULT},
            {eCbType::CbLightProperty, sizeof(CbLightProperty), D3D11_USAGE_DYNAMIC},
            {eCbType::CbMaterialFactors, sizeof(CbMaterialFactors), D3D11_USAGE_DYNAMIC},
            {eCbType::CbColor, sizeof(CbColor), D3D11_USAGE_DYNAMIC},
            {eCbType::CbOrthoMatrix, sizeof(CbScreenSpaceMatrix), D3D11_USAGE_DYNAMIC},
        };
        static_assert(sizeof(cbMapTable) / sizeof(ConstantBufferMap) == static_cast<uint8_t>(eCbType::ConstantBufferCount));
        D3D11_BUFFER_DESC desc = {};
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        HRESULT result = {};
        for (uint8_t index = 0; index < static_cast<uint8_t>(eCbType::ConstantBufferCount); ++index)
        {
            desc.Usage = cbMapTable[index].Usage;
            desc.CPUAccessFlags = (cbMapTable[index].Usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
            desc.ByteWidth = cbMapTable[index].ByteWidth;

            ASSERT(desc.BindFlags & static_cast<uint32_t>(D3D11_BIND_CONSTANT_BUFFER), "desc.BindFlags not bind as Constant-buffer");
            ASSERT(desc.ByteWidth != 0, "desc.ByteWidth is zero");

            result = mDevice->CreateBuffer(&desc, nullptr, &mCbList[index].Buffer);

            if (FAILED(result))
            {
                break;
            }

            mCbList[index].Usage = desc.Usage;
            mCbList[index].ByteWidth = desc.ByteWidth;
        }
        return SUCCEEDED(result);
    }
}
