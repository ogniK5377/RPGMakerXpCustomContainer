#pragma once
#include <cstdint>
#include "RubyCommon.h"

namespace Ruby {
class Common;
}

namespace RubyModule {
inline RB_VALUE* InputModule{};
void RegisterCustomInput();
} // namespace RubyModule
