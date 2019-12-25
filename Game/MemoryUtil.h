#pragma once
#include <cstring>
#include <Windows.h>

namespace MemoryUtil {
void PatchBytes(DWORD address, char* bytes, std::size_t length);
void PatchJump(DWORD src, DWORD dst);
DWORD CalculateJumpOffset(DWORD src, DWORD dst);

template <typename T>
void PatchType(DWORD address, T data) {
    DWORD memory_protection{};
    // Allow us to read, write and execute the memory region we're modifying
    VirtualProtect(reinterpret_cast<LPVOID>(address), sizeof(T), PAGE_EXECUTE_READWRITE,
                   &memory_protection);

    // Write to the region
    T* p = reinterpret_cast<T*>(address);
    *p = data;

    // Revert the memory regions permissions
    VirtualProtect(reinterpret_cast<LPVOID>(address), sizeof(T), memory_protection,
                   &memory_protection);
}

} // namespace MemoryUtil
