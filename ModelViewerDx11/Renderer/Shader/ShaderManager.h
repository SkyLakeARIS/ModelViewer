#pragma once
#include "ShaderType.h"
#include "../GfxPrerequisites.h"
#include "../Resources/VertexType.h"

namespace renderer
{
	class VkRenderer;
	struct SemiMaterial;
    class Renderer;

    class ShaderManager
    {
    private:

        enum class eVertexShader : uint32_t
        {
            VsBasicWithShadow,
            VsOutline,
            VsTexture,
            VsSimple,
            VsSkybox,
            VsScreen,
            VsShadow,
            VsDeferred,
            VsDebugColor,
            VertexShaderCount
        };

        enum class ePixelShader : uint32_t
        {
            PsBasicWithShadow,
            PsOutline,
            PsShadow,
            PsSkybox,
            PsTexture,
            PsColor,
            PsDeferred,
            PsTextureMRT,
            PsSimple,
            PixelShaderCount
        };

        struct ShaderMap
        {
            eShader Type;
            eVertexShader VsIndex;
            ePixelShader PsIndex;
        };

        struct ConstantBufferEntry
        {
            ID3D11Buffer* Buffer;
            uint32_t ByteWidth;
            D3D11_USAGE Usage;
        };

        struct ConstantBufferMap
        {
            eCbType Index; // added for easy to see.
            uint32_t ByteWidth;
            D3D11_USAGE Usage;
        };

        struct ShaderStageHash
        {
            eShader Type;
            HashID VertexShaderHash;
            HashID PixelShaderHash;
        };

    public:

        ShaderManager(ID3D11Device& device, ID3D11DeviceContext& deviceContext);
        ShaderManager();
        ~ShaderManager();
        bool SetupShadersVk(VkRenderer& renderer);
        void GetStageInfoByType(eShader shader, HashID& outVsHash, HashID& outPsHash);
        bool SetupShaders();
        bool CreatePresetConstantBuffers();

        void UpdateCB(eCbType type, const void* const data) const;
        void               UpdateMaterial(eCbType type, const SemiMaterial& material);
        void               GetShadersByType(eShader type, ID3D11VertexShader** vs, ID3D11PixelShader** ps) const;
        ID3D11InputLayout* GetInputLayoutByType(eVertexFormat type) const;
        ID3D11Buffer* GetConstantBufferByType(eCbType type) const;
    public:
        // 1. Renderer가 예약한 Cb는 ShaderManager가 관리하지 않고 앱 내부적으로 혹은 Renderer가 업데이트/바인드 처리
        // 2. Material만 구조가 잡히기 전까지 ShaderManager에게 바인딩 정보와 업데이트를 맡김. (수작업을 대신함)
        // MEMO: 머티리얼당 셰이더라는 가정이므로, 동일 셰이더에 다른 머티리얼은 대응이 안되는 문제는 나중에 해결해야 함.
        static void GetMaterialCbBindingDesc(eShader type, MaterialCbBinding& outBindingDesc);
        // MEMO: 텍스처를 여러 개 쓸 계획은 있지만, 아직 샘플러를 여러 개 쓸 계획은 없음(셰이더당 샘플러는 하나). Sampler Slot은 나중에 상황에 따라 확장.
        static void GetMaterialTextureBindSlots(eShader type, int8_t* const outBindingSlots);
        static void GetMaterialSamplerBindSlot(eShader type, int8_t& outBindingSlot);

    private:
        bool createInputLayout(const WCHAR* const path, D3D11_INPUT_ELEMENT_DESC* const desc,
            uint32_t numDescElements, eVertexFormat type, ID3D11InputLayout** const outInputLayout);


        bool createVertexShader(const WCHAR* const path, ID3D11VertexShader** const outVertexShader);

        bool createPixelShader(const WCHAR* const path, ID3D11PixelShader** const outPixelShader);

        bool compileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

    private:

        ID3D11Device* mDevice;
        ID3D11DeviceContext* mDeviceContext;

        // CB
        ConstantBufferEntry mCbList[static_cast<uint8_t>(eCbType::ConstantBufferCount)];

        ShaderMap           mShaderMapTable[static_cast<uint32_t>(eShader::ShaderCount)]; // combine vs-ps pairs
        ID3D11VertexShader* mVertexShadersList[static_cast<uint32_t>(eVertexShader::VertexShaderCount)];
        ID3D11PixelShader* mPixelShaderList[static_cast<uint32_t>(ePixelShader::PixelShaderCount)];
        ID3D11InputLayout* mInputLayoutList[static_cast<uint32_t>(eVertexFormat::FormatCount)];

        //
        ShaderStageHash mShaderHashes[static_cast<uint8_t>(eShader::ShaderCount)];
    };
}

