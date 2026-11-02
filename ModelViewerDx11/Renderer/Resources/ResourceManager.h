#pragma once
#include <unordered_map>
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    enum class eShader : uint8_t;
}

namespace renderer
{
    class ShaderModule;
    class Shader;
    class Texture;

    class ResourceManager
    {
    public:
        ResourceManager(VkDevice device);
        ~ResourceManager();

        bool CreateShaders();

        bool TryAddTexture(HashID hash, Texture* const texture);

        Texture* GetTexture(HashID hash);
        Shader* GetShaderByType(eShader type) const;
    private:
        VkDevice mDevice;
        // MEMO: 주요 리소스-- Texture, shader, sampler, shaderModule, image, buffer
        // 물체를 생성하는 과정은 Loader가 로드 리소스 생성, 초기화 - 각 매니저에 할당 - 오브젝트 매니저에 추가
        // 제거의 경우는 일단 - 앱에서 제거를 요청할 때.
        std::unordered_map<HashID, Texture*> mTextures;
        std::unordered_map<HashID, ShaderModule*> mShaderModules;
        std::vector<Shader*> mShaders;

    };
}
