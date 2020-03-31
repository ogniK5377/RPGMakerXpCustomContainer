#pragma once
#include <cstdint>

namespace Memory {
struct CRxInput;
}

namespace Input {
inline uintptr_t ChangeScreenModeAddress{};
void Poll(Memory::CRxInput* input);
} // namespace Input
