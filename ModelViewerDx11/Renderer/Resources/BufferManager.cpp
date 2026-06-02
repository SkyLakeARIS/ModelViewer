#include "BufferManager.h"

#include "RenderTypes.h"
#include "../../Util/Macro.h"

namespace renderer
{
    BufferManager::BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
        : mDevice(device)
        , mDeviceContext(deviceContext)
        , mVertexBuffer(nullptr)
        , mIndexBuffer(nullptr)
        , mVertexBufferTotalSize(0)
        , mIndexBufferTotalSize(0)
        , mVertexBufferCursor(0)
        , mIndexBufferCursor(0)
    {
        ASSERT(mDevice, "device is nullptr. pass the valid device");
        ASSERT(mDeviceContext, "deviceContext is nullptr. pass the valid deviceContext");
    }

    BufferManager::~BufferManager()
    {
        SAFETY_RELEASE(mVertexBuffer);
        SAFETY_RELEASE(mIndexBuffer);
        mDevice = nullptr;
        mDeviceContext = nullptr;
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

        mVertexBufferTotalSize = vertexBufferByteSize;
        mIndexBufferTotalSize = indexBufferByteSize;

        // TODO: 여기에서 ID3D11Buffer초기화. 즉, 무거운 생성 작업은 여기에서 전부 처리.
        // TODO: 이 클래스의 객체는  Renderer 클래스가 관리한다.
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = vertexBufferByteSize;
        if(mDevice->CreateBuffer(&bufferDesc, nullptr, &mVertexBuffer) == E_FAIL)
        {
            ASSERT(false, "vertex buffer creation failed, check the options");
            return false;
        }

        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = indexBufferByteSize;
        if(mDevice->CreateBuffer(&bufferDesc, nullptr, &mIndexBuffer) == E_FAIL)
        {
            ASSERT(false, "index buffer creation failed, check the options");
            return false;
        }

        return true;
    }

    void BufferManager::AddVertexData(int8_t* const pData, int32_t dataByteSize, HashID hash)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        if(!pData || dataByteSize <= 0)
        {
            return;
        }

        if(mVertexBufferTotalSize <= mVertexBufferCursor + dataByteSize)
        {
            resizeVertexBuffer(mVertexBufferCursor + dataByteSize);
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = mVertexBufferCursor;
        updateRange.right = mVertexBufferCursor + dataByteSize;
        mDeviceContext->UpdateSubresource(mVertexBuffer, 0, &updateRange, pData, 0, 0);

        BufferRange range = {};
        range.StartIndex = mVertexBufferCursor;
        range.Count = dataByteSize;

        // TODO: 나중에 <파일 명 - 해시> Map을 만들어서 같은 데이터를 중복 삽입하는 건지 해시함수 충돌 발생인지 구분할 필요가 있음.
        ASSERT(mVertexRanges.find(hash) == mVertexRanges.end(), "(vtx) Double-Insertion or Hash-Collision Detected.");

        mVertexRanges.insert(std::make_pair(hash, range));

        mVertexBufferCursor += dataByteSize;
    }

    void BufferManager::AddIndexData(int8_t* const pData, int32_t dataByteSize, HashID hash)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        if (mIndexBufferTotalSize <= mIndexBufferCursor + dataByteSize)
        {
            resizeIndexBuffer(mIndexBufferCursor + dataByteSize);
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = mIndexBufferCursor;
        updateRange.right = mIndexBufferCursor + dataByteSize;
        mDeviceContext->UpdateSubresource(mIndexBuffer, 0, &updateRange, pData, 0, 0);

        BufferRange range = {};
        range.StartIndex = mIndexBufferCursor;
        range.Count = dataByteSize;

        // TODO: 나중에 <파일 명 - 해시> Map을 만들어서 같은 데이터를 중복 삽입하는 건지 해시함수 충돌 발생인지 구분할 필요가 있음.
        ASSERT(mIndexRanges.find(hash) == mIndexRanges.end(), "(idx) Double-Insertion or Hash-Collision Detected.");

        mIndexRanges.insert(std::make_pair(hash, range));

        mIndexBufferCursor += dataByteSize;
    }

    void BufferManager::RemoveVertexData(HashID hash)
    {
        // TODO: 나중에 삭제시 빈공간(단편화)처리는 어떻게 할지도 고민과 방안 탐색이 필요함. - 자체 메모리 풀을 만들 때 같이 작업이 되면 좋을 것 같다.
        // 우선은 고정된 수의 모델만 출력하므로 확장만.하도록 하자.
        mVertexRanges.erase(hash);
    }

    void BufferManager::RemoveIndexData(HashID hash)
    {
        mIndexRanges.erase(hash);
    }

    void BufferManager::UpdateVertexData(int8_t* const pData, HashID hash)
    {
        ASSERT(pData, "pData is nillptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");
        const auto& rangeIt = mVertexRanges.find(hash);
        if(rangeIt == mVertexRanges.end())
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
        mDeviceContext->UpdateSubresource(mVertexBuffer, 0, &updateRange, pData, 0, 0);
    }

    void BufferManager::UpdateIndexData(int8_t* const pData, HashID hash)
    {
        ASSERT(pData, "pData is nillptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");
        const auto& rangeIt = mIndexRanges.find(hash);
        if (rangeIt == mIndexRanges.end())
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
        mDeviceContext->UpdateSubresource(mIndexBuffer, 0, &updateRange, pData, 0, 0);
    }

    BufferRange BufferManager::GetVertexRangeByHash(HashID hash)
    {
        BufferRange range = {-1, -1};

        const auto& rangeIt = mVertexRanges.find(hash);
        if(rangeIt != mVertexRanges.end())
        {
            range = rangeIt->second;
        }

        return range;
    }

    BufferRange BufferManager::GetIndexRangeByHash(HashID hash)
    {
        BufferRange range = { -1, -1 };

        const auto& rangeIt = mIndexRanges.find(hash);
        if (rangeIt != mIndexRanges.end())
        {
            range = rangeIt->second;
        }

        return range;
    }

    ID3D11Buffer* BufferManager::GetVertexBuffer() const
    {
        return mVertexBuffer;
    }

    ID3D11Buffer* BufferManager::GetIndexBuffer() const
    {
        return mIndexBuffer;
    }

    void BufferManager::resizeVertexBuffer(uint32_t newSize)
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

        if(mVertexBufferCursor > 0)
        {
            D3D11_BOX updateRange = {};
            updateRange.front = 0;
            updateRange.back = 1;
            updateRange.top = 0;
            updateRange.bottom = 1;
            updateRange.left = 0;
            updateRange.right = mVertexBufferCursor;
            mDeviceContext->CopySubresourceRegion(resizedBuffer, 0, 0, 0, 0, mVertexBuffer, 0, &updateRange);
        }

        std::swap(mVertexBuffer,resizedBuffer);
        SAFETY_RELEASE(resizedBuffer);
        mVertexBufferTotalSize = bufferDesc.ByteWidth;
    }

    void BufferManager::resizeIndexBuffer(uint32_t newSize)
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

        if(mIndexBufferCursor > 0)
        {
            D3D11_BOX updateRange = {};
            updateRange.front = 0;
            updateRange.back = 1;
            updateRange.top = 0;
            updateRange.bottom = 1;
            updateRange.left = 0;
            updateRange.right = mIndexBufferCursor;
            mDeviceContext->CopySubresourceRegion(resizedBuffer, 0, 0, 0, 0, mIndexBuffer, 0, &updateRange);
        }

        std::swap(mIndexBuffer, resizedBuffer);
        SAFETY_RELEASE(resizedBuffer);
        mIndexBufferTotalSize = bufferDesc.ByteWidth;
    }
}
