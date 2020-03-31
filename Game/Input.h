#pragma once
#include <cstdint>

namespace Memory {
struct CRxInput;
}

namespace Input {
inline uintptr_t ChangeScreenModeAddress{};

// Addresses
inline uintptr_t POLL_ADDRESS{};

bool KeyRepeat(Memory::CRxInput* input, int key);
bool KeyTrigger(Memory::CRxInput* input, int key);
bool KeyPress(Memory::CRxInput* input, int key);
void Poll(Memory::CRxInput* input);
void Update(Memory::CRxInput* input);
} // namespace Input
