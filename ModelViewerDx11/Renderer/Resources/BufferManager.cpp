#include "BufferManager.h"
#include "RenderTypes.h"
#include "../../Util/Macro.h"

namespace renderer
{
    BufferManager::BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
        : mDevice(device)
        , mDeviceContext(deviceContext)
        , mIndexStrideSize(sizeof(uint32_t))
    {
        ASSERT(mDevice, "device is nullptr. pass the valid device");
        ASSERT(mDeviceContext, "deviceContext is nullptr. pass the valid deviceContext");
    }

    BufferManager::~BufferManager()
    {
        mDevice = nullptr;
        mDeviceContext = nullptr;

        for (auto& buffer : mVertexBuffers)
        {
            SAFETY_RELEASE(buffer.second.Buffer);
            buffer.second.Ranges.clear();
        }
        mVertexBuffers.clear();

        for (auto& strideIt : mVertexRemovedRanges)
        {
            strideIt.second.clear();
            std::vector<BufferRange>().swap(strideIt.second);
        }
        mVertexRemovedRanges.clear();

        for (auto& buffer : mIndexBuffers)
        {
            SAFETY_RELEASE(buffer.second.Buffer);
            buffer.second.Ranges.clear();
        }
        mIndexBuffers.clear();

        for (auto& strideIt : mIndexRemovedRanges)
        {
            strideIt.second.clear();
            std::vector<BufferRange>().swap(strideIt.second);
        }
        mIndexRemovedRanges.clear();
    }

    bool BufferManager::Initialize(int32_t vertexBufferByteSize, int32_t indexBufferByteSize)
    {
        if(vertexBufferByteSize < 0)
        {
            ASSERT(vertexBufferByteSize >= 0, "vtx buf size must over 0.");
            return false;
        }

        if (indexBufferByteSize < 0)
        {
            ASSERT(indexBufferByteSize >= 0, "idx buf size must over 0.");
            return false;
        }


        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = vertexBufferByteSize;



        mVertexBuffers.reserve(static_cast<int16_t>(eInputLayout::InputlayoutCount));
        for(int16_t layoutType = 0; layoutType < static_cast<int16_t>(eInputLayout::InputlayoutCount); ++layoutType)
        {
            const int16_t stride = GetVertexStrideSize(static_cast<eInputLayout>(layoutType));
            BufferResource bufferRes = {};
            if (mDevice->CreateBuffer(&bufferDesc, nullptr, &bufferRes.Buffer) == E_FAIL)
            {
                ASSERT(false, "vertex buffer creation failed, check the options. layoutType(%d)", layoutType);
                return false;
            }
            bufferRes.TotalSizeBytes = vertexBufferByteSize;
            mVertexBuffers.insert(std::make_pair(stride, bufferRes));
            mVertexRemovedRanges.insert(std::make_pair(stride, std::vector<BufferRange>()));
        }


        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = indexBufferByteSize;

        mIndexBuffers.reserve(static_cast<int16_t>(eInputLayout::InputlayoutCount));

        BufferResource bufferRes = {};
        // TODO: 테스트 해보기.
        if (mDevice->CreateBuffer(&bufferDesc, nullptr, &bufferRes.Buffer) == E_FAIL)
        {
            ASSERT(false, "index buffer creation failed, check the options. layoutType(%d)", mIndexStrideSize);
            return false;
        }
        bufferRes.TotalSizeBytes = indexBufferByteSize;
        mIndexBuffers.insert(std::make_pair(mIndexStrideSize, bufferRes));
        mIndexRemovedRanges.insert(std::make_pair(mIndexStrideSize, std::vector<BufferRange>()));

        return true;
    }

    void BufferManager::AddVertexData(int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mVertexBuffers.find(stride);

        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        // TODO: 나중에 <파일 명 - 해시> Map을 만들어서 같은 데이터를 중복 삽입하는 건지 해시함수 충돌 발생인지 구분할 필요가 있음.
        if(rangeIt != bufResIt->second.Ranges.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = rangeIt->second.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = rangeIt->second.StartIndex / bufResIt->first;
            return;
        }

        if (bufResIt->second.TotalSizeBytes <= bufResIt->second.CursorBytes + dataByteSize)
        {
            resizeVertexBuffer(bufResIt->second.CursorBytes + dataByteSize, bufResIt);
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = bufResIt->second.CursorBytes;
        updateRange.right = bufResIt->second.CursorBytes + dataByteSize;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);

        BufferRange range = {};
        range.StartIndex = bufResIt->second.CursorBytes;
        range.Count = dataByteSize;

        bufResIt->second.Ranges.insert(std::make_pair(hash, range));
        bufResIt->second.CursorBytes += dataByteSize;

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = range.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = range.StartIndex / bufResIt->first;
    }

    void BufferManager::AddIndexData(int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mIndexBuffers.find(stride);

        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = rangeIt->second.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = rangeIt->second.StartIndex / bufResIt->first;
            return;
        }

        if (bufResIt->second.TotalSizeBytes <= bufResIt->second.CursorBytes + dataByteSize)
        {
            resizeIndexBuffer(bufResIt->second.CursorBytes + dataByteSize, bufResIt);
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = bufResIt->second.CursorBytes;
        updateRange.right = bufResIt->second.CursorBytes + dataByteSize;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);

        BufferRange range = {};
        range.StartIndex = bufResIt->second.CursorBytes;
        range.Count = dataByteSize;

        bufResIt->second.Ranges.insert(std::make_pair(hash, range));
        bufResIt->second.CursorBytes += dataByteSize;

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = range.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = range.StartIndex / bufResIt->first;
    }

    void BufferManager::RemoveVertexData(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if(rangeIt != bufResIt->second.Ranges.end())
        {
            const auto& removedRangeIt = mVertexRemovedRanges.find(stride);
            removedRangeIt->second.push_back(rangeIt->second);

            bufResIt->second.Ranges.erase(rangeIt);
        }
    }

    void BufferManager::RemoveIndexData(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            const auto& removedRangeIt = mIndexRemovedRanges.find(stride);
            removedRangeIt->second.push_back(rangeIt->second);

            bufResIt->second.Ranges.erase(rangeIt);
        }
    }

    void BufferManager::UpdateVertexData(int8_t* const pData, int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        ASSERT(pData, "pData is nullptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt == bufResIt->second.Ranges.end())
        {
            return;
        }

        // MEMO: 업데이트이므로 기존에 삽입된 사이즈만큼의 데이터를 복사하도록 함.
        // 현재 구조에서는 사이즈를 키우거나 줄이려면, 제거 후 다시 추가해야 함.
        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = rangeIt->second.StartIndex;
        updateRange.right = rangeIt->second.StartIndex + rangeIt->second.Count;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);
    }

    void BufferManager::UpdateIndexData(int8_t* const pData, int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        ASSERT(pData, "pData is nullptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt == bufResIt->second.Ranges.end())
        {
            return;
        }
        
        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = rangeIt->second.StartIndex;
        updateRange.right = rangeIt->second.StartIndex + rangeIt->second.Count;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);
    }

    BufferRange BufferManager::GetVertexRangeByteByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            range = rangeIt->second;
        }

        return range;
    }

    BufferRange BufferManager::GetIndexRangeByteByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            range = rangeIt->second;
        }

        return range;
    }

    BufferRange BufferManager::GetVertexRangeCountByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            range.Count = rangeIt->second.Count / bufResIt->first;
            range.StartIndex = rangeIt->second.StartIndex / bufResIt->first;
        }

        return range;
    }

    BufferRange BufferManager::GetIndexRangeCountByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& rangeIt = bufResIt->second.Ranges.find(hash);
        if (rangeIt != bufResIt->second.Ranges.end())
        {
            range.Count = rangeIt->second.Count / bufResIt->first;
            range.StartIndex = rangeIt->second.StartIndex / bufResIt->first;
        }

        return range;
    }

    ID3D11Buffer* BufferManager::GetVertexBuffer(int16_t stride) const
    {
        const auto& bufResIt = mVertexBuffers.find(stride);
        return bufResIt->second.Buffer;
    }

    ID3D11Buffer* BufferManager::GetIndexBuffer(int16_t stride) const
    {
        const auto& bufResIt = mIndexBuffers.find(stride);
        return bufResIt->second.Buffer;
    }

    int16_t BufferManager::GetIndexStrideSize() const
    {
        return mIndexStrideSize;
    }

    void BufferManager::resizeVertexBuffer(uint32_t newSize, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt)
    {
        ID3D11Buffer* resizedBuffer = nullptr;
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = newSize * 2;
        if (mDevice->CreateBuffer(&bufferDesc, nullptr, &resizedBuffer) == E_FAIL)
        {
            ASSERT(false, "vertex buffer creation failed while resizing. check the options. tried buffer type (%d)", bufferDesc.BindFlags);
            return;
        }

        if (bufResIt->second.CursorBytes > 0)
        {
            D3D11_BOX updateRange = {};
            updateRange.front = 0;
            updateRange.back = 1;
            updateRange.top = 0;
            updateRange.bottom = 1;
            updateRange.left = 0;
            updateRange.right = bufResIt->second.CursorBytes;
            mDeviceContext->CopySubresourceRegion(resizedBuffer, 0, 0, 0, 0, bufResIt->second.Buffer, 0, &updateRange);
        }

        std::swap(bufResIt->second.Buffer, resizedBuffer);
        SAFETY_RELEASE(resizedBuffer);
        bufResIt->second.TotalSizeBytes = bufferDesc.ByteWidth;
    }

    void BufferManager::resizeIndexBuffer(uint32_t newSize, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt)
    {
        ID3D11Buffer* resizedBuffer = nullptr;
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = newSize * 2;
        if (mDevice->CreateBuffer(&bufferDesc, nullptr, &resizedBuffer) == E_FAIL)
        {
            ASSERT(false, "index buffer creation failed while resizing. check the options. tried buffer type (%d)", bufferDesc.BindFlags);
            return;
        }

        if (bufResIt->second.CursorBytes > 0)
        {
            D3D11_BOX updateRange = {};
            updateRange.front = 0;
            updateRange.back = 1;
            updateRange.top = 0;
            updateRange.bottom = 1;
            updateRange.left = 0;
            updateRange.right = bufResIt->second.CursorBytes;
            mDeviceContext->CopySubresourceRegion(resizedBuffer, 0, 0, 0, 0, bufResIt->second.Buffer, 0, &updateRange);
        }

        std::swap(bufResIt->second.Buffer, resizedBuffer);
        SAFETY_RELEASE(resizedBuffer);
        bufResIt->second.TotalSizeBytes = bufferDesc.ByteWidth;
    }
}
