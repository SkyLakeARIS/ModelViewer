#pragma once
#include "Type.h"

namespace util
{
    extern constexpr int16_t MAX_PATH_LENGTH = 260;
    extern constexpr int16_t MAX_NAME_LENGTH = 260;
    // MEMO: 서로 사이즈를 맞추는 것이 오버플로우 방지에 좋을 것 같다.
    extern constexpr int16_t MAX_STRING_LENGTH = 260;
}
