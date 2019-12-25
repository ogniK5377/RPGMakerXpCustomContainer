#include "MemoryUtil.h"

namespace MemoryUtil {

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

    // JMP 0xAABBCCDD
    *(char*)(src) = 0xe9;                               // JMP
    *(DWORD*)(src + 1) = CalculateJumpOffset(src, dst); // Address offset to jump to

    VirtualProtect(reinterpret_cast<LPVOID>(src), 5, memory_protection, &memory_protection);
}

DWORD CalculateJumpOffset(DWORD src, DWORD dst) {
    // (Destination Address - Starting Address) - Size of jmp instruction
    return (dst - src) - 5;
}

} // namespace MemoryUtil
