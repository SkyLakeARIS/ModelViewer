#pragma once
#include <unordered_map>

#include "../../framework.h"

namespace renderer
{
    struct BufferRange;
}

namespace renderer
{
    class BufferManager
    {
    public:
        BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
        ~BufferManager();

        bool Initialize(int32_t vertexBufferByteSize, int32_t indexBufferByteSize);

        void AddVertexData(int8_t* const pData, int32_t dataByteSize, HashID hash);
        void AddIndexData(int8_t* const pData, int32_t dataByteSize, HashID hash);

        void RemoveVertexData(HashID hash);
        void RemoveIndexData(HashID hash);

        // TODO: 해시 테이블에서 데이터 범위 값을 찾을 수 있으므로, 원하는 범위 업데이트는 나중에 확장(크기 확장도?)
        void UpdateVertexData(int8_t* const pData, HashID hash);
        void UpdateIndexData(int8_t* const pData, HashID hash);

        BufferRange& GetVertexRangeByHash(HashID hash);
        BufferRange& GetIndexRangeByHash(HashID hash);

        ID3D11Buffer* GetVertexBuffer() const;
        ID3D11Buffer* GetIndexBuffer() const;
    private:

        // TODO: 하나로 합쳐도 좋을 것 같고, 분리해도 좋을 것 같고.
        void resizeVertexBuffer(uint32_t newSize);
        void resizeIndexBuffer(uint32_t newSize);
    private:
        ID3D11Device* mDevice;
        ID3D11DeviceContext* mDeviceContext;
        ID3D11Buffer* mVertexBuffer;
        ID3D11Buffer* mIndexBuffer;

        // TODO: FlatMap은 일단 구조를 잡고나서 도입하자.
        // MEMO: pair<hash - BufferRange>
        std::unordered_map<HashID, BufferRange> mVertexRanges;
        std::unordered_map<HashID, BufferRange> mIndexRanges;

        // MEMO: in Bytes
        int32_t mVertexBufferTotalSize;
        int32_t mIndexBufferTotalSize;

        int32_t mVertexBufferCursor;
        int32_t mIndexBufferCursor;
    };
}
