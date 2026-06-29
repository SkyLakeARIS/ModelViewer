#include "BufferManager.h"
#include <algorithm>
#include "RenderTypes.h"
#include "../../Util/Macro.h"

namespace renderer
{
    bool BufferRangeIncrCompare(const BufferRange& left, const BufferRange& right)
    {
        return left.StartIndex < right.StartIndex;
    }

    BufferManager::IndexFormatPair BufferManager::sIndexFormatMap[static_cast<int8_t>(BufferManager::eIndexListFormat::IndexListFormatCount)] =
    {
        {sizeof(uint32_t), DXGI_FORMAT_R32_UINT},
    };

    BufferManager::BufferManager(ID3D11Device* device, ID3D11DeviceContext* deviceContext, eIndexListFormat indexFormat)
        : mDevice(device)
        , mDeviceContext(deviceContext)
        , mIndexFormat(indexFormat)
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
            buffer.second.SubChunks.clear();
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
            buffer.second.SubChunks.clear();
        }
        mIndexBuffers.clear();

        for (auto& strideIt : mIndexRemovedRanges)
        {
            strideIt.second.clear();
            std::vector<BufferRange>().swap(strideIt.second);
        }
        mIndexRemovedRanges.clear();


        for (auto& buffer : mVertexBuffersDynamic)
        {
            SAFETY_RELEASE(buffer.second.Buffer);
            buffer.second.SubChunks.clear();
        }
        mVertexBuffersDynamic.clear();

        for (auto& buffer : mIndexBuffersDynamic)
        {
            SAFETY_RELEASE(buffer.second.Buffer);
            buffer.second.SubChunks.clear();
        }
        mIndexBuffersDynamic.clear();
    }

    bool BufferManager::Initialize(int32_t vertexBufferByteSizeStatic, int32_t indexBufferByteSizeStatic, int32_t vertexBufferByteSizeDynamic, int32_t indexBufferByteSizeDynamic)
    {
        if(vertexBufferByteSizeStatic < 0)
        {
            ASSERT(vertexBufferByteSizeStatic >= 0, "vtx buf size must over 0.");
            return false;
        }

        if (indexBufferByteSizeStatic < 0)
        {
            ASSERT(indexBufferByteSizeStatic >= 0, "idx buf size must over 0.");
            return false;
        }


        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = vertexBufferByteSizeStatic;



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
            bufferRes.SubChunks.reserve(128);
            bufferRes.TotalSizeBytes = vertexBufferByteSizeStatic;
            mVertexBuffers.insert(std::make_pair(stride, std::move(bufferRes)));
            std::vector<BufferRange> ranges;
            ranges.reserve(128);
            mVertexRemovedRanges.insert(std::make_pair(stride, std::move(ranges)));
        }


        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = indexBufferByteSizeStatic;

        mIndexBuffers.reserve(static_cast<int16_t>(eInputLayout::InputlayoutCount));
        const int16_t indexStrideSize = sIndexFormatMap[static_cast<int8_t>(mIndexFormat)].Stride;


        BufferResource indexBufResStatic = {};
        indexBufResStatic.SubChunks.reserve(256);
        if (mDevice->CreateBuffer(&bufferDesc, nullptr, &indexBufResStatic.Buffer) == E_FAIL)
        {
            ASSERT(false, "index buffer creation failed, check the options. layoutType(%d)", indexStrideSize);
            return false;
        }
        indexBufResStatic.TotalSizeBytes = indexBufferByteSizeStatic;
        mIndexBuffers.insert(std::make_pair(indexStrideSize, std::move(indexBufResStatic)));
        std::vector<BufferRange> ranges;
        ranges.reserve(256);
        mIndexRemovedRanges.insert(std::make_pair(indexStrideSize, std::move(ranges)));

        // init Dynamic Buffers
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = vertexBufferByteSizeDynamic;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        mVertexBuffersDynamic.reserve(static_cast<int16_t>(eInputLayout::InputlayoutCount));
        for (int16_t layoutType = 0; layoutType < static_cast<int16_t>(eInputLayout::InputlayoutCount); ++layoutType)
        {
            const int16_t stride = GetVertexStrideSize(static_cast<eInputLayout>(layoutType));
            BufferResource bufferResDynamic = {};
            if (mDevice->CreateBuffer(&bufferDesc, nullptr, &bufferResDynamic.Buffer) == E_FAIL)
            {
                ASSERT(false, "vertex buffer(dynamic) creation failed, check the options. layoutType(%d)", layoutType);
                return false;
            }
            bufferResDynamic.SubChunks.reserve(16);
            bufferResDynamic.TotalSizeBytes = vertexBufferByteSizeDynamic;
            mVertexBuffersDynamic.insert(std::make_pair(stride, std::move(bufferResDynamic)));
        }


        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.ByteWidth = indexBufferByteSizeDynamic;

        mIndexBuffersDynamic.reserve(static_cast<int16_t>(eInputLayout::InputlayoutCount));

        BufferResource indexBufDynamic = {};
        if (mDevice->CreateBuffer(&bufferDesc, nullptr, &indexBufDynamic.Buffer) == E_FAIL)
        {
            ASSERT(false, "index buffer(dynamic) creation failed, check the options. layoutType(%d)", indexStrideSize);
            return false;
        }
        indexBufDynamic.SubChunks.reserve(32);
        indexBufDynamic.TotalSizeBytes = indexBufferByteSizeDynamic;
        mIndexBuffersDynamic.insert(std::make_pair(indexStrideSize, std::move(indexBufDynamic)));
        std::vector<BufferRange> rangesDynamic;
        rangesDynamic.reserve(32);

        return true;
    }

    void BufferManager::AddVertex(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mVertexBuffers.find(stride);

        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        // TODO: 나중에 <파일 명 - 해시> Map을 만들어서 같은 데이터를 중복 삽입하는 건지 해시함수 충돌 발생인지 구분할 필요가 있음.
        if(subChunkIt != bufResIt->second.SubChunks.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
            ++subChunkIt->second.RefCount;
            return;
        }

        // MEMO: 적절한 공간을 가진 빈공간 탐색
        const auto& removedRangeIt = mVertexRemovedRanges.find(stride);
        std::vector<BufferRange>::iterator bestFitSpaceIt = removedRangeIt->second.begin();
        int32_t minRemainSpace = INT32_MAX;
        for(int32_t rangeIndex = 0; rangeIndex < removedRangeIt->second.size(); ++rangeIndex)
        {
            const std::vector<BufferRange>::iterator& element = (removedRangeIt->second.begin() + rangeIndex);
            const int32_t remainSpace = element->Count - dataByteSize;
            if(remainSpace >= 0)
            {
                // MEMO: save best-fit.
                if (remainSpace < minRemainSpace)
                {
                    minRemainSpace = remainSpace;
                    bestFitSpaceIt = element;
                }
            }
        }

        int32_t writeCursorInBuffer  = bufResIt->second.CursorBytes;
        if(bestFitSpaceIt != removedRangeIt->second.end())
        {
            // MEMO: 빈공간 재활용
            if(minRemainSpace == 0)
            {
                *bestFitSpaceIt = removedRangeIt->second.back();
                removedRangeIt->second.pop_back();
            }
            else
            {
                writeCursorInBuffer = bestFitSpaceIt->StartIndex;
                // MEMO: 재활용하고 남은 공간은 또 재활용을 하기 위함.
                bestFitSpaceIt->StartIndex = bestFitSpaceIt->StartIndex + dataByteSize;
                bestFitSpaceIt->Count = minRemainSpace;
            }
        }
        else
        {
            // MEMO: 재활용할 공간이 없음
            if (bufResIt->second.TotalSizeBytes <= writeCursorInBuffer + dataByteSize)
            {
                resizeVertexBuffer(writeCursorInBuffer + dataByteSize, D3D11_USAGE_DEFAULT, 0, bufResIt);
            }

            bufResIt->second.CursorBytes += dataByteSize;
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = writeCursorInBuffer;
        updateRange.right = writeCursorInBuffer + dataByteSize;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);

        SubChunk subChunk = {};
        subChunk.Ranges.StartIndex = writeCursorInBuffer;
        subChunk.Ranges.Count = dataByteSize;
        subChunk.RefCount = 1;

        bufResIt->second.SubChunks.insert(std::make_pair(hash, subChunk));

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = subChunk.Ranges.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = subChunk.Ranges.StartIndex / bufResIt->first;
    }

    void BufferManager::AddIndex(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mIndexBuffers.find(stride);

        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
            ++subChunkIt->second.RefCount;
            return;
        }

        // MEMO: 적절한 공간을 가진 빈공간 탐색
        const auto& removedRangeIt = mIndexRemovedRanges.find(stride);
        std::vector<BufferRange>::iterator bestFitSpaceIt = removedRangeIt->second.begin();
        int32_t minRemainSpace = INT32_MAX;
        for (int32_t rangeIndex = 0; rangeIndex < removedRangeIt->second.size(); ++rangeIndex)
        {
            const std::vector<BufferRange>::iterator& element = (removedRangeIt->second.begin() + rangeIndex);
            const int32_t remainSpace = element->Count - dataByteSize;
            if (remainSpace >= 0)
            {
                // MEMO: save best-fit.
                if (remainSpace < minRemainSpace)
                {
                    minRemainSpace = remainSpace;
                    bestFitSpaceIt = element;
                }
            }
        }

        int32_t writeCursorInBuffer = bufResIt->second.CursorBytes;
        if (bestFitSpaceIt != removedRangeIt->second.end())
        {
            // MEMO: 빈공간 재활용
            if (minRemainSpace == 0)
            {
                *bestFitSpaceIt = removedRangeIt->second.back();
                removedRangeIt->second.pop_back();
            }
            else
            {
                writeCursorInBuffer = bestFitSpaceIt->StartIndex;
                // MEMO: 재활용하고 남은 공간은 또 재활용을 하기 위함.
                bestFitSpaceIt->StartIndex = bestFitSpaceIt->StartIndex + dataByteSize;
                bestFitSpaceIt->Count = minRemainSpace;
            }
        }
        else
        {
            // MEMO: 재활용할 공간이 없음
            if (bufResIt->second.TotalSizeBytes <= writeCursorInBuffer + dataByteSize)
            {
                resizeIndexBuffer(writeCursorInBuffer + dataByteSize, D3D11_USAGE_DEFAULT, 0, bufResIt);
            }

            bufResIt->second.CursorBytes += dataByteSize;
        }

        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = writeCursorInBuffer;
        updateRange.right = writeCursorInBuffer + dataByteSize;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);

        SubChunk subChunk = {};
        subChunk.Ranges.StartIndex = writeCursorInBuffer;
        subChunk.Ranges.Count = dataByteSize;
        subChunk.RefCount = 1;

        bufResIt->second.SubChunks.insert(std::make_pair(hash, subChunk));

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = subChunk.Ranges.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = subChunk.Ranges.StartIndex / bufResIt->first;
    }

    void BufferManager::AddVertexDynamic(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride, BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mVertexBuffersDynamic.find(stride);

        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
            return;
        }

        if (bufResIt->second.TotalSizeBytes <= bufResIt->second.CursorBytes + dataByteSize)
        {
            resizeVertexBuffer(bufResIt->second.CursorBytes + dataByteSize, D3D11_USAGE_DYNAMIC, 0, bufResIt);
        }

        const D3D11_MAP mapType = mbNeedDiscardDynamicVertex ? (D3D11_MAP_WRITE_DISCARD) : D3D11_MAP_WRITE_NO_OVERWRITE;
        mbNeedDiscardDynamicVertex = false;

        D3D11_MAPPED_SUBRESOURCE mappedRes = {};
        mDeviceContext->Map(bufResIt->second.Buffer, 0, mapType, 0, &mappedRes);

        int8_t* gpuBuffer = reinterpret_cast<int8_t*>(mappedRes.pData);
        memcpy(gpuBuffer + bufResIt->second.CursorBytes, pData, dataByteSize);

        mDeviceContext->Unmap(bufResIt->second.Buffer, 0);

        SubChunk subChunk = {};
        subChunk.Ranges.StartIndex = bufResIt->second.CursorBytes;
        subChunk.Ranges.Count = dataByteSize;
        // MEMO: 동적 데이터에는 필요 없음.

        bufResIt->second.CursorBytes += dataByteSize;
        bufResIt->second.SubChunks.insert(std::make_pair(hash, subChunk));

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = subChunk.Ranges.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = subChunk.Ranges.StartIndex / bufResIt->first;
    }

    void BufferManager::AddIndexDynamic(const int8_t* const pData, int32_t dataByteSize, HashID hash, int16_t stride,
        BufferRange& outRangeInBuffer)
    {
        ASSERT(pData, "pData is nullptr,");
        ASSERT(dataByteSize > 0, "dataByteSize is zero or negative");
        ASSERT(stride > 0, "stride is zero or negative");
        if (!pData || dataByteSize <= 0)
        {
            return;
        }

        auto bufResIt = mIndexBuffersDynamic.find(stride);

        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            // MEMO: in vertex count (not bytes). convert bytes -> stride
            outRangeInBuffer.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            outRangeInBuffer.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
            return;
        }

        if (bufResIt->second.TotalSizeBytes <= bufResIt->second.CursorBytes + dataByteSize)
        {
            resizeVertexBuffer(bufResIt->second.CursorBytes + dataByteSize, D3D11_USAGE_DYNAMIC, 0, bufResIt);
        }


        const D3D11_MAP mapType = mbNeedDiscardDynamicIndex ? (D3D11_MAP_WRITE_DISCARD) : D3D11_MAP_WRITE_NO_OVERWRITE;
        mbNeedDiscardDynamicIndex = false;

        D3D11_MAPPED_SUBRESOURCE mappedRes = {};
        mDeviceContext->Map(bufResIt->second.Buffer, 0, mapType, 0, &mappedRes);

        int8_t* gpuBuffer = reinterpret_cast<int8_t*>(mappedRes.pData);
        memcpy(gpuBuffer + subChunkIt->second.Ranges.StartIndex, pData, dataByteSize);

        mDeviceContext->Unmap(bufResIt->second.Buffer, 0);

        SubChunk subChunk = {};
        subChunk.Ranges.StartIndex = bufResIt->second.CursorBytes;
        subChunk.Ranges.Count = dataByteSize;
        // MEMO: 동적 데이터에는 필요 없음.

        bufResIt->second.SubChunks.insert(std::make_pair(hash, subChunk));
        bufResIt->second.CursorBytes += dataByteSize;

        // MEMO: in vertex count (not bytes). convert bytes -> stride
        outRangeInBuffer.Count = subChunk.Ranges.Count / bufResIt->first;
        outRangeInBuffer.StartIndex = subChunk.Ranges.StartIndex / bufResIt->first;
    }

    void BufferManager::RemoveVertexData(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        // TODO: RefCount를 통해서 바로 제거되지 않도록 작업해야 한다. (Add 함수도 마찬가지로 중복 데이터가 삽입되면 RefUp)
        if(subChunkIt != bufResIt->second.SubChunks.end())
        {
            --subChunkIt->second.RefCount;
            if (subChunkIt->second.RefCount <= 0)
            {
                const auto& removedRangeIt = mVertexRemovedRanges.find(stride);
                removedRangeIt->second.push_back(subChunkIt->second.Ranges);

                bufResIt->second.SubChunks.erase(subChunkIt);
                // TODO: 나중에 별도 Merge 함수로 분리하여, 정한 기준에 따라서 주기적으로 병합을 시도하는 것이 필요함.
                // MEMO: 연속된 빈공간 병합 시도
                if (removedRangeIt->second.size() >= 2)
                {
                    std::sort(removedRangeIt->second.begin(), removedRangeIt->second.end(), BufferRangeIncrCompare);
                    auto cursorIt = removedRangeIt->second.begin();
                    // 1. 병합되고 나서 vector size가 1개 일 때.
                    // 2. nextRangeIt이 end 일 때.
                    while ((cursorIt + 1) != removedRangeIt->second.end())
                    {
                        const auto nextRangeIt = cursorIt + 1;
                        if ((cursorIt->StartIndex + cursorIt->Count) == nextRangeIt->StartIndex)
                        {
                            cursorIt->Count += nextRangeIt->Count;
                            // TODO: optimize - 현재 로직 구조로는 제거를 빠르게 할 수 없는 것 같다. 다른 좋은 방안 찾는게 필요
                            removedRangeIt->second.erase(nextRangeIt);
                            // TODO: optimize - 이렇게하면 항상 처음으로 돌아가므로 중간에 병합된 경우 다시 처음부터 순회해야하는 비효율 존재. 빠른 구현을 위해 우선 이렇게 함.
                            // RemoveIndexData도 마찬가지로 작업
                            cursorIt = removedRangeIt->second.begin();
                        }
                        else
                        {
                            ++cursorIt;
                        }
                    }
                }
            }
        }
    }

    void BufferManager::RemoveIndexData(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            --subChunkIt->second.RefCount;
            if(subChunkIt->second.RefCount <= 0)
            {
                const auto& removedRangeIt = mIndexRemovedRanges.find(stride);
                removedRangeIt->second.push_back(subChunkIt->second.Ranges);

                bufResIt->second.SubChunks.erase(subChunkIt);
                if (removedRangeIt->second.size() >= 2)
                {
                    auto cursorIt = removedRangeIt->second.begin();

                    while ((cursorIt + 1) != removedRangeIt->second.end())
                    {
                        const auto nextRangeIt = cursorIt + 1;
                        if ((cursorIt->StartIndex + cursorIt->Count) == nextRangeIt->StartIndex)
                        {
                            cursorIt->Count += nextRangeIt->Count;
                            removedRangeIt->second.erase(nextRangeIt);
                            cursorIt = removedRangeIt->second.begin();
                        }
                        else
                        {
                            ++cursorIt;
                        }
                    }
                }
            }
        }
    }

    void BufferManager::UpdateVertexData(int8_t* const pData, int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        ASSERT(pData, "pData is nullptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt == bufResIt->second.SubChunks.end())
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
        updateRange.left = subChunkIt->second.Ranges.StartIndex;
        updateRange.right = subChunkIt->second.Ranges.StartIndex + subChunkIt->second.Ranges.Count;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);
    }

    void BufferManager::UpdateIndexData(int8_t* const pData, int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        ASSERT(pData, "pData is nullptr. nullptr이 아닌 유효한 pData를 전달해야 합니다.");

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt == bufResIt->second.SubChunks.end())
        {
            return;
        }
        
        D3D11_BOX updateRange = {};
        updateRange.front = 0;
        updateRange.back = 1;
        updateRange.top = 0;
        updateRange.bottom = 1;
        updateRange.left = subChunkIt->second.Ranges.StartIndex;
        updateRange.right = subChunkIt->second.Ranges.StartIndex + subChunkIt->second.Ranges.Count;
        mDeviceContext->UpdateSubresource(bufResIt->second.Buffer, 0, &updateRange, pData, 0, 0);
    }

    void BufferManager::MarkInvalidateDynamicBuf()
    {
        mbNeedDiscardDynamicVertex = true;

        for (auto& bufStrideIt : mVertexBuffersDynamic)
        {
            bufStrideIt.second.CursorBytes = 0;
            bufStrideIt.second.SubChunks.clear();
        }

        mbNeedDiscardDynamicIndex = true;
        for (auto& bufStrideIt : mIndexBuffersDynamic)
        {
            bufStrideIt.second.CursorBytes = 0;
            bufStrideIt.second.SubChunks.clear();
        }
    }

    BufferRange BufferManager::GetVertexRangeByteByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            range = subChunkIt->second.Ranges;
        }

        return range;
    }

    BufferRange BufferManager::GetIndexRangeByteByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            range = subChunkIt->second.Ranges;
        }

        return range;
    }

    BufferRange BufferManager::GetVertexRangeCountByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mVertexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            range.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            range.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
        }

        return range;
    }

    BufferRange BufferManager::GetIndexRangeCountByHash(int16_t stride, HashID hash)
    {
        ASSERT(stride > 0, "stride is zero or negative");
        ASSERT(hash > 0, "hash is zero or negative");
        BufferRange range = { -1, -1 };

        const auto& bufResIt = mIndexBuffers.find(stride);
        const auto& subChunkIt = bufResIt->second.SubChunks.find(hash);
        if (subChunkIt != bufResIt->second.SubChunks.end())
        {
            range.Count = subChunkIt->second.Ranges.Count / bufResIt->first;
            range.StartIndex = subChunkIt->second.Ranges.StartIndex / bufResIt->first;
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

    ID3D11Buffer* BufferManager::GetVertexBufferDynamic(int16_t stride) const
    {
        const auto& bufResIt = mVertexBuffersDynamic.find(stride);
        return bufResIt->second.Buffer;
    }

    ID3D11Buffer* BufferManager::GetIndexBufferDynamic(int16_t stride) const
    {
        const auto& bufResIt = mIndexBuffersDynamic.find(stride);
        return bufResIt->second.Buffer;
    }

    int16_t BufferManager::GetIndexStrideSize() const
    {
        return sIndexFormatMap[static_cast<int8_t>(mIndexFormat)].Stride;
    }

    DXGI_FORMAT BufferManager::GetIndexFormat() const
    {
        return sIndexFormatMap[static_cast<int8_t>(mIndexFormat)].Format;
    }

    void BufferManager::resizeVertexBuffer(uint32_t newSize, D3D11_USAGE usageType, uint32_t cpuAccessFlag, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt)
    {
        ID3D11Buffer* resizedBuffer = nullptr;
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = usageType;
        bufferDesc.ByteWidth = newSize * 2;
        bufferDesc.CPUAccessFlags = cpuAccessFlag;
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

    void BufferManager::resizeIndexBuffer(uint32_t newSize, D3D11_USAGE usageType, uint32_t cpuAccessFlag, std::unordered_map<int16_t, BufferResource>::iterator& bufResIt)
    {
        ID3D11Buffer* resizedBuffer = nullptr;
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.Usage = usageType;
        bufferDesc.ByteWidth = newSize * 2;
        bufferDesc.CPUAccessFlags = cpuAccessFlag;
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
