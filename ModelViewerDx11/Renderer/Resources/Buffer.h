#pragma once
#include "../GfxPrerequisites.h"
#include "../../Util/Type.h"

namespace renderer
{
    struct BufferRange;

    class Buffer
    {
    public:
        enum class eBufferType
        {
            Vertex,
            Index,
            Uniform,
            Integrate,
            Staging,
            BufferTypeCount
        };

    public:
        Buffer(const VkDevice& device);
        ~Buffer();

        bool Create(uint32_t requestSizeInBytes, eBufferType type, uint32_t memProperties, uint32_t queueFamilyIndex);
        void Release();
        void AddData(const void* const pData, uint32_t count, uint32_t elementSize, HashID hash, BufferRange& outRange);
        void AddDataOffset(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t offset);

        VkBuffer GetBuffer() const;
    private:

        void resize();
    private:
        VkDevice mDevice;
        VkBuffer mBuffer;
        VkDeviceMemory mMemory;
        uint32_t mAlignment;
        uint32_t mAllocationSize;
        uint32_t mCursor;
        std::unordered_map<HashID, BufferRange> mSubResources;
        std::vector<BufferRange> mRemovedSpaces;
        uint32_t mMemoryProperties;
    };
}
