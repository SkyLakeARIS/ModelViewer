#pragma once
#include "../framework.h"

namespace util
{
    // TODO: 나중에 충돌 문제가 발견되면 uint64_t 전환 필요
    // TODO: 좀 더 괜찮은 해시 함수로 교체 검토(xxHash, MurmurHash3, FNV-1a)
    inline HashID GetDjb2Hash(const uint8_t* str)
    {
        HashID hash = 5381;
        int32_t c = 0;

        while (c = *str++) {
            // hash = hash * 33 + c
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }

    inline HashID GetDjb2Hash(const int8_t* str)
    {
        HashID hash = 5381;
        int32_t c = 0;

        while (c = *str++) {
            // hash = hash * 33 + c
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }
}
