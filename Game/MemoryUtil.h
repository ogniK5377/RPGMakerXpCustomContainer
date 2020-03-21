#pragma once
#include <cstring>
#include <Windows.h>
extern "C" {
#include "ld32.h"
}

namespace MemoryUtil {
constexpr std::size_t MAX_INS_LENGTH = 15;
constexpr std::size_t INS_JMP_LENGTH = 5;

void NopSled(DWORD address, std::size_t length);
void PatchBytes(DWORD address, char* bytes, std::size_t length);
void PatchJump(DWORD src, DWORD dst);
void PatchJumpNoProtect(DWORD src, DWORD dst);
void* CreateDetour(DWORD src, DWORD dst);
DWORD CalculateJumpOffset(DWORD src, DWORD dst);
DWORD CallToDirectAddress(DWORD src);

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

template <typename T>
T MakeCallable(DWORD address) {
    return reinterpret_cast<T>(address);
}

template <typename T>
T CreateDetour(DWORD src, DWORD dst) {
    DWORD memory_protection{};
    DWORD memory_protection2{};

    // Allow us to modify memory
    VirtualProtect(reinterpret_cast<LPVOID>(src), MAX_INS_LENGTH, PAGE_EXECUTE_READWRITE,
                   &memory_protection);

    // Get at least 5 bytes of instructions
    std::size_t total_to_copy = 0;
    while (total_to_copy < INS_JMP_LENGTH) {
        total_to_copy += length_disasm(reinterpret_cast<void*>(src + total_to_copy));
    }

    // Code cave for our bytes we copied + a jump instruction
    void* og_func = malloc(total_to_copy + INS_JMP_LENGTH);
    // FUNCTION_BLOCKS.push_back(og_func);

    // Mark our region as Read Write Execute
    VirtualProtect(reinterpret_cast<LPVOID>(og_func), total_to_copy + INS_JMP_LENGTH,
                   PAGE_EXECUTE_READWRITE, &memory_protection2);

    // Copy our bytes to our code caved section
    std::memcpy(og_func, reinterpret_cast<void*>(src), total_to_copy);

    // Jump to our original function and continue execution flow
    PatchJumpNoProtect(reinterpret_cast<DWORD>(og_func) + total_to_copy, src + total_to_copy);

    // Nop sled if we have more than 5 bytes copied to prevent creating bad instructions
    const std::size_t ins_to_sled = total_to_copy - INS_JMP_LENGTH;
    if (ins_to_sled > 0) {
        NopSled(src + INS_JMP_LENGTH, ins_to_sled);
    }

    // Patch the base of the func to jump to our detour
    PatchJumpNoProtect(src, dst);

    // Reset permissions
    VirtualProtect(reinterpret_cast<LPVOID>(src), MAX_INS_LENGTH, memory_protection,
                   &memory_protection);

    // Return our codecave which jumps to the original function
    return static_cast<T>(og_func);
}

} // namespace MemoryUtil
