#pragma once
#include <unordered_map>
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    class Buffer;
    class Sampler;
    enum class eSamplerType : uint8_t;
    enum class eShader : uint8_t;
    class ShaderModule;
    class Shader;
    class Texture;

    class ResourceManager
    {
    public:
        ResourceManager(VkDevice device);
        ~ResourceManager();

        bool CreateShaders();
        bool CreateSamplers();
        bool CreateBuffers();

        void AddBufferData(const void* const pData, uint32_t elementSize, uint32_t count, HashID hash, BufferRange& outRange, bool bVertexData);
        void AddTexture(HashID hash, Texture* const texture);
        void AddPipeline(HashID hash, VkPipeline pipeline);

        Texture* TryGetTexture(HashID hash);
        Shader* GetShaderByType(eShader type) const;
        Sampler* GetSamplerByType(eSamplerType type) const;
        VkPipeline TryGetPipeline(HashID hash) const;
        Buffer* GetBuffer(bool bVertexBuffer) const;
    private:
        VkDevice mDevice;
        // MEMO: 주요 리소스-- Texture, shader, sampler, shaderModule, image, buffer
        // 물체를 생성하는 과정은 Loader가 로드 리소스 생성, 초기화 - 각 매니저에 할당 - 오브젝트 매니저에 추가
        // 제거의 경우는 일단 - 앱에서 제거를 요청할 때.
        std::unordered_map<HashID, Texture*> mTextures;
        std::unordered_map<HashID, ShaderModule*> mShaderModules;
        std::vector<Shader*> mShaders;
        std::unordered_map<eSamplerType, Sampler*> mSamplers;
        std::unordered_map<HashID, VkPipeline> mPipelines;
        // TODO: 고도화 작업 시 Vtx/Idx 통합 버퍼로 변경
        Buffer* mVertexBuffer;
        Buffer* mIndexBuffer;
    };
}
