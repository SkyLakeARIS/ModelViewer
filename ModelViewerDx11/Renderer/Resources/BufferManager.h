#pragma once
#include <unordered_map>
#include "RenderTypes.h"

namespace renderer
{
    enum class eBufferType
    {
	    Vertex,
        Index,
    };

    class BufferManager
    {
    public:
        enum class eIndexListFormat
        {
            UInt32,
            IndexListFormatCount
        };
    private:
        struct SubChunk
        {
            BufferRange Ranges;
            int32_t RefCount;
        };
        struct BufferChunk
        {
            ID3D11Buffer* Buffer;
            // MEMO: store in Bytes
            std::unordered_map<HashID, SubChunk> SubChunks;
            int32_t TotalSizeBytes;
            int32_t CursorBytes;
        };
        struct IndexFormatPair
        {
            int16_t Stride;
            DXGI_FORMAT Format;
        };
    public:
        BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext, eIndexListFormat indexFormat);
        ~BufferManager();

        bool Initialize(int32_t vertexBufferByteSizeStatic, int32_t indexBufferByteSizeStatic, int32_t vertexBufferByteSizeDynamic, int32_t
                        indexBufferByteSizeDynamic);

        // MEMO: data들을 받고, Buffer내의 ElementCount/Offset을 반환
        void AddVertex(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);
        void AddIndex(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);

        void AddVertexDynamic(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);
        void AddIndexDynamic(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer);

        void RemoveVertexData(int16_t stride, HashID hash);
        void RemoveIndexData(int16_t stride, HashID hash);

        // MEMO: pData의 사이즈가 기존에 삽입된 사이즈와 같지 않으면 정의되지 않음
        void UpdateVertexData(int8_t* const pData, int16_t stride, HashID hash);
        // MEMO: pData의 사이즈가 기존에 삽입된 사이즈와 같지 않으면 정의되지 않음
        void UpdateIndexData(int8_t* const pData, int16_t stride, HashID hash);

        // MEMO: Vertex/Index 버퍼 통합 처리
        void MarkInvalidateDynamicBuf();

        BufferRange GetVertexRangeByteByHash(int16_t stride, HashID hash);
        BufferRange GetIndexRangeByteByHash(int16_t stride, HashID hash);

        BufferRange GetVertexRangeCountByHash(int16_t stride, HashID hash);
        BufferRange GetIndexRangeCountByHash(int16_t stride, HashID hash);

        ID3D11Buffer* GetVertexBuffer(int16_t stride) const;
        ID3D11Buffer* GetIndexBuffer(int16_t stride) const;

        ID3D11Buffer* GetVertexBufferDynamic(int16_t stride) const;
        ID3D11Buffer* GetIndexBufferDynamic(int16_t stride) const;

        int16_t GetIndexStrideSize() const;
        DXGI_FORMAT GetIndexFormat() const;
    private:
        void uploadResource(eBufferUsage usage, ID3D11Buffer* const buffer, const int8_t* const pData, int32_t startIndex, int32_t count, bool bIsDiscardDynamicBuffer);
        void addSubChunkToBuffer(eBufferUsage usage, int16_t stride, BufferChunk& bufferChunk, HashID hash, int32_t startIndex, int32_t count, int32_t& outStartIndexInElement, int32_t
                              & outCountInElement);
        void resizeBuffer(uint32_t newSize, uint32_t bindFlag, D3D11_USAGE usageType, uint32_t cpuAccessFlag, std::unordered_map<int16_t, BufferChunk>::iterator& chunkIt);

        void tryGetRecycleSpaceBestFit(const std::unordered_map<int16_t, std::vector<BufferRange>>::iterator& removedRangeIt, int32_t tryByteSize, int32_t& outWriteStartIndex, std::vector<BufferRange>::iterator& outRecycledSpace);

        void mergeRemovedSpace(const std::unordered_map<int16_t, std::vector<BufferRange>>::iterator& removedBufferIt);
    public:
        static constexpr int32_t sVertexBufferDefaultSize = 4096;
        static constexpr int32_t sIndexBufferDefaultSize = 4096;
    private:
        static IndexFormatPair sIndexFormatMap[static_cast<int8_t>(eIndexListFormat::IndexListFormatCount)];
    private:
        ID3D11Device* mDevice;
        ID3D11DeviceContext* mDeviceContext;

        eIndexListFormat mIndexFormat;

        // MEMO: 0 offset bind를 하려면 버퍼 내에 서로 다른 stride를 가진 데이터가 존재하면
        // 안될 것으로 생각되어 Buffer들을 분리한다.
        // MEMO: pair<vertex stride, Buffer> - stride 크기 별로 개별 버퍼를 관리
        std::unordered_map<int16_t, BufferChunk> mVertexBuffers;
        std::unordered_map<int16_t, BufferChunk> mIndexBuffers;

        // MEMO: 데이터 제거 시 빈공간에 대한 데이터 범위를 저장하여 데이터 추가시 체크
        std::unordered_map<int16_t, std::vector<BufferRange>> mVertexRemovedRanges;
        std::unordered_map<int16_t, std::vector<BufferRange>> mIndexRemovedRanges;

        // MEMO: 동적 데이터 전용
        std::unordered_map<int16_t, BufferChunk> mVertexBuffersDynamic;
        std::unordered_map<int16_t, BufferChunk> mIndexBuffersDynamic;

        bool mbNeedDiscardDynamicVertex;
        bool mbNeedDiscardDynamicIndex;
    };
}
