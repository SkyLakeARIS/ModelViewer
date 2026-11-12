#include "Buffer.h"
#include "VertexType.h"
#include "../Uploader.h"
#include "../VkHelper.h"
#include "../../Util/Macro.h"

namespace renderer
{
    Buffer::Buffer(const VkDevice& device)
        : mDevice(device)
        , mBuffer(nullptr)
        , mMemory(nullptr)
        , mAlignment(0)
        , mAllocationSize(0)
        , mCursor(0)
        , mMemoryProperties(0)
    {
    }

    Buffer::~Buffer()
    {
        vkFreeMemory(mDevice, mMemory, nullptr);
        vkDestroyBuffer(mDevice, mBuffer, nullptr);
        mRemovedSpaces.clear();
        std::vector<BufferRange>().swap(mRemovedSpaces);
        mSubResources.clear();
    }

    bool Buffer::Create(uint32_t requestSizeInBytes, eBufferType type, uint32_t memProperties, uint32_t queueFamilyIndex)
    {
        constexpr VkBufferUsageFlags BUFFER_USAGE_FLAG[] =
        {
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        };
        static_assert(std::size(BUFFER_USAGE_FLAG) == static_cast<EnumInt>(eBufferType::BufferTypeCount), "BUFFER_USAGE_FLAG 요소 수와 eBufferType열거형 수가 맞아야 합니다.");
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = requestSizeInBytes;
        createInfo.pQueueFamilyIndices = &queueFamilyIndex;
        createInfo.queueFamilyIndexCount = 1;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.usage = BUFFER_USAGE_FLAG[static_cast<EnumInt>(type)];
        const bool bIsSupportStagingUpload = VkHelper::IsSupportStagingUpload();
        if((memProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && bIsSupportStagingUpload)
        {
            createInfo.usage |=  VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if (bIsSupportStagingUpload == false && (memProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            ASSERT(false, "staging 이 필요하지 않은 GPU일 수 있습니다.");
        }
    
        vkCreateBuffer(mDevice, &createInfo, nullptr, &mBuffer);

        VkBufferMemoryRequirementsInfo2 bufferMemoryRequirements{};
        bufferMemoryRequirements.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
        bufferMemoryRequirements.buffer = mBuffer;

        VkMemoryRequirements2 memoryRequirements2{};
        memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        vkGetBufferMemoryRequirements2(mDevice, &bufferMemoryRequirements, &memoryRequirements2);


        uint32_t memoryType = 0;
        VkHelper::GetMemoryType(memoryRequirements2.memoryRequirements, memProperties, memoryType);
        mMemoryProperties = memProperties;
        mAlignment = memoryRequirements2.memoryRequirements.alignment;
        mAllocationSize = memoryRequirements2.memoryRequirements.size;

        VkHelper::AllocateMemory(mDevice, mAllocationSize, memoryType, mMemory);

        VkBindBufferMemoryInfo bindBufferMemoryInfo{};
        bindBufferMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
        bindBufferMemoryInfo.buffer = mBuffer;
        bindBufferMemoryInfo.memory = mMemory;
        bindBufferMemoryInfo.memoryOffset = 0;
        vkBindBufferMemory2(mDevice, 1, &bindBufferMemoryInfo);

        return true;
    }

    void Buffer::Release()
    {
        // MEMO: Pooling 시에 활용할 리셋 함수
    }

    void Buffer::AddData(const void* const pData, uint32_t count, uint32_t elementSize, HashID hash, BufferRange& outRange)
    {
        // TODO: elementSize가 alignment보다 작을 경우 alignment로 정렬
        if (mSubResources.find(hash) == mSubResources.end())
        {
            const uint32_t vertexSizeInBytes = count * elementSize;
            mCursor = VkHelper::AlignedSize(mCursor, elementSize);
            const BufferRange range{ (int32_t)mCursor, (int32_t)vertexSizeInBytes };

            Uploader::UploadBuffer(pData, vertexSizeInBytes, mCursor, mMemoryProperties, mMemory, mBuffer);

            outRange = BufferRange{ static_cast<int32_t>(mCursor / elementSize) , static_cast<int32_t>(count) };
            mCursor += vertexSizeInBytes;
            mSubResources.insert({ hash, std::move(range) });
        }
    }

    void Buffer::AddDataOffset(const void* const pData, uint32_t count, uint32_t elementSize, uint32_t offset)
    {
        const uint32_t vertexSizeInBytes = count * elementSize;
        mCursor = offset;
        mCursor = VkHelper::AlignedSize(mCursor, elementSize);

        Uploader::UploadBuffer(pData, vertexSizeInBytes, offset, mMemoryProperties, mMemory, mBuffer);
    }

    VkBuffer Buffer::GetBuffer() const
    {
        return mBuffer;
    }

    void Buffer::resize()
    {

    }
}
