#pragma once
#include <cstdint>

namespace Memory {
struct CRxInput;
}

namespace Input {
void Poll(Memory::CRxInput* input);
} // namespace Input
