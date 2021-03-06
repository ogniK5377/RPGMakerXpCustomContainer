#include <array>
#include "MemoryUtil.h"

namespace MemoryUtil {

void NopSled(uintptr_t address, std::size_t length) {
    ReprotectScope<MemPerm::ReadWriteExecute> protection(address, length);

    // Fill memory with NOPs
    for (std::size_t i = 0; i < length; i++) {
        PatchType<uint8_t>(address + i, 0x90); // The opcode 0x90 is NOP
    }
}

void PatchBytes(uintptr_t address, const char* bytes, std::size_t length) {
    ReprotectScope<MemPerm::ReadWriteExecute> protection(address, length);

    // Overwrite memory region with our data
    for (std::size_t i = 0; i < length; i++) {
        PatchType<uint8_t>(address + i, static_cast<uint8_t>(bytes[i]));
    }
}

void PatchJump(uintptr_t src, uintptr_t dst) {
    ReprotectScope<MemPerm::ReadWriteExecute> protection(src, INS_JMP_LENGTH);

    // Call our helper function which just writes a JMP
    PatchJumpNoProtect(src, dst);
}

void PatchJumpNoProtect(uintptr_t src, uintptr_t dst) {
    // JMP 0xAABBCCDD
    PatchType<uint8_t>(src, 0xe9);                                // JMP
    PatchType<uintptr_t>(src + 1, CalculateJumpOffset(src, dst)); // Address offset to jump to
}

uintptr_t CalculateJumpOffset(uintptr_t src, uintptr_t dst) {
    // (Destination Address - Starting Address) - Size of jmp instruction
    return (dst - src) - INS_JMP_LENGTH;
}

uintptr_t InstructionToDirectAddress(uintptr_t src) {
    ReprotectScope<MemPerm::ReadWriteExecute> protection(src, MAX_INS_LENGTH);
    uintptr_t address{};

    const auto ins = ReadType<char>(src);
    if (ins == '\xE8' || ins == '\xE9') {
        // Base + Offset + instruction size
        address = src + ReadType<uintptr_t>(src + 1) + 5;
    } else {
        DebugBreak();
    }

    return address;
}

uintptr_t SlideAddress(uintptr_t base, uintptr_t offset) {
    return base + offset;
}

uintptr_t SlideAddress(uintptr_t base, uintptr_t old_base, uintptr_t address) {
    return base + (address - old_base);
}

MemPerm Reprotect(uintptr_t address, std::size_t size, MemPerm new_permissions) {
    DWORD memory_protection{};
    VirtualProtect(reinterpret_cast<LPVOID>(address), size, static_cast<DWORD>(new_permissions),
                   &memory_protection);
    return static_cast<MemPerm>(memory_protection);
}
} // namespace MemoryUtil
