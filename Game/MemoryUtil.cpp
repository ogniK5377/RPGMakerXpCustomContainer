#include "MemoryUtil.h"

namespace MemoryUtil {

void NopSled(DWORD address, std::size_t length) {
    DWORD memory_protection{};
    VirtualProtect(reinterpret_cast<LPVOID>(address), length, PAGE_EXECUTE_READWRITE,
                   &memory_protection);

    unsigned char* p = reinterpret_cast<unsigned char*>(address);
    // Fill memory with NOPs
    for (std::size_t i = 0; i < length; i++) {
        *(unsigned char*)(address + i) = 0x90; // The opcode 0x90 is NOP
    }

    VirtualProtect(reinterpret_cast<LPVOID>(address), length, memory_protection,
                   &memory_protection);
}

void PatchBytes(DWORD address, char* bytes, std::size_t length) {
    DWORD memory_protection{};
    VirtualProtect(reinterpret_cast<LPVOID>(address), length, PAGE_EXECUTE_READWRITE,
                   &memory_protection);

    char* p = reinterpret_cast<char*>(address);

    // Overwrite memory region with our data
    for (std::size_t i = 0; i < length; i++) {
        *(p++) = *(bytes++);
    }

    VirtualProtect(reinterpret_cast<LPVOID>(address), length, memory_protection,
                   &memory_protection);
}

void PatchJump(DWORD src, DWORD dst) {
    DWORD memory_protection{};
    VirtualProtect(reinterpret_cast<LPVOID>(src), 5, PAGE_EXECUTE_READWRITE, &memory_protection);

    // Call our helper function which just writes a JMP
    PatchJumpNoProtect(src, dst);

    VirtualProtect(reinterpret_cast<LPVOID>(src), 5, memory_protection, &memory_protection);
}

void PatchJumpNoProtect(DWORD src, DWORD dst) {
    // JMP 0xAABBCCDD
    *(unsigned char*)(src) = 0xe9;                      // JMP
    *(DWORD*)(src + 1) = CalculateJumpOffset(src, dst); // Address offset to jump to
}
DWORD CalculateJumpOffset(DWORD src, DWORD dst) {
    // (Destination Address - Starting Address) - Size of jmp instruction
    return (dst - src) - 5;
}

DWORD CallToDirectAddress(DWORD src) {
    DWORD address{};
    DWORD memory_protection{};
    VirtualProtect(reinterpret_cast<LPVOID>(src), MAX_INS_LENGTH, PAGE_EXECUTE_READWRITE,
                   &memory_protection);

    const char ins = *(char*)(src);
    if (ins == '\xE8') {
        // Base + Offset + instruction size
        address = src + *(DWORD*)(src + 1) + 5;
    } else {
        DebugBreak();
    }

    VirtualProtect(reinterpret_cast<LPVOID>(src), MAX_INS_LENGTH, memory_protection,
                   &memory_protection);
    return address;
}

} // namespace MemoryUtil
