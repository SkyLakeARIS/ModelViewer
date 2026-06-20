#pragma once
#include <unordered_map>
#include "../../framework.h"

namespace renderer
{
    struct BufferRange;

    class BufferManager
    {
    private:
        struct BufferResource
        {
            ID3D11Buffer* Buffer;
            std::unordered_map<HashID, BufferRange> Ranges;
            int32_t TotalSizeBytes;
            int32_t CursorBytes;
        };

    public:
        BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
        ~BufferManager();

        bool Initialize(int32_t vertexBufferByteSize, int32_t indexBufferByteSize);

        // TODO: consider - 유니크 포인터를 쓴다면 소유권 문제와 유효성 문제가 있을 것 같은데 나중에 비동기 상황에 문제가 되지 않을까?
        void AddVertexData(int8_t* const pData, int32_t dataByteSize, HashID hash);
        void AddIndexData(int8_t* const pData, int32_t dataByteSize, HashID hash);

        void RemoveVertexData(HashID hash);
        void RemoveIndexData(HashID hash);

        // TODO: 해시 테이블에서 데이터 범위 값을 찾을 수 있으므로, 원하는 범위 업데이트는 나중에 확장(크기 확장도?)
        void UpdateVertexData(int8_t* const pData, HashID hash);
        void UpdateIndexData(int8_t* const pData, HashID hash);

        BufferRange GetVertexRangeByHash(HashID hash);
        BufferRange GetIndexRangeByHash(HashID hash);

        ID3D11Buffer* GetVertexBuffer() const;
        ID3D11Buffer* GetIndexBuffer() const;

        // stride 별 buffer를 적용한 함수들
        void AddVertexData(int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);
        void AddIndexData(int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);

        void RemoveVertexData(int16_t stride, HashID hash);
        void RemoveIndexData(int16_t stride, HashID hash);

        // MEMO: pData의 사이즈가 기존에 삽입된 사이즈와 같지 않으면 정의되지 않음
        void UpdateVertexData(int8_t* const pData, int16_t stride, HashID hash);
        // MEMO: pData의 사이즈가 기존에 삽입된 사이즈와 같지 않으면 정의되지 않음
        void UpdateIndexData(int8_t* const pData, int16_t stride, HashID hash);

        BufferRange GetVertexRangeByteByHash(int16_t stride, HashID hash);
        BufferRange GetIndexRangeByteByHash(int16_t stride, HashID hash);

        BufferRange GetVertexRangeCountByHash(int16_t stride, HashID hash);
        BufferRange GetIndexRangeCountByHash(int16_t stride, HashID hash);

        ID3D11Buffer* GetVertexBuffer(int16_t stride) const;
        ID3D11Buffer* GetIndexBuffer(int16_t stride) const;

        int16_t GetIndexStrideSize() const;
    private:
        // TODO: 하나로 합쳐도 좋을 것 같고, 분리해도 좋을 것 같고.
        void resizeVertexBuffer(uint32_t newSize);
        void resizeIndexBuffer(uint32_t newSize);

        // stride 별 buffer를 적용한 함수들
        void resizeVertexBufferNew(uint32_t newSize, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt);
        void resizeIndexBufferNew(uint32_t newSize, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt);
    public:
        static constexpr int32_t sVertexBufferDefaultSize = 4096;
        static constexpr int32_t sIndexBufferDefaultSize = 4096;
    private:
        ID3D11Device* mDevice;
        ID3D11DeviceContext* mDeviceContext;
        ID3D11Buffer* mVertexBuffer;
        ID3D11Buffer* mIndexBuffer;
        int16_t mIndexStrideSize;
        // MEMO: 0 offset bind를 하려면 버퍼 내에 서로 다른 stride를 가진 데이터가 존재하면 안될 것으로 생각되어 Buffer들을 분리한다.
        // MEMO: pair<vertex stride, Buffer> - stride 크기 별로 개별 버퍼를 관리
        std::unordered_map<int16_t, BufferResource> mVertexBuffers;
        std::unordered_map<int16_t, BufferResource> mIndexBuffers;
        // TODO: 일단 만들어는 놨으나 Add 함수에서 먼저 추가하기 전에 Ranges 공간을 확인하도록 로직을 수정해야 한다.
        // MEMO: 데이터 제거 시 빈공간에 대한 데이터 범위를 저장하여 데이터 추가시 체크
        std::unordered_map<int16_t, std::vector<BufferRange>> mVertexRemovedRanges;
        std::unordered_map<int16_t, std::vector<BufferRange>> mIndexRemovedRanges;

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
