#pragma once
#include <cstring>
#include <type_traits>
#include <Windows.h>
extern "C" {
#include "ld32.h"
}

namespace MemoryUtil {
constexpr std::size_t MAX_INS_LENGTH = 15;
constexpr std::size_t INS_JMP_LENGTH = 5;
enum class MemPerm : DWORD {
    NoAccess = PAGE_NOACCESS,
    Read = PAGE_READONLY,
    Execute = PAGE_EXECUTE,
    ReadWrite = PAGE_READWRITE,
    ReadWriteExecute = PAGE_EXECUTE_READWRITE,
    ReadExecute = PAGE_EXECUTE_READ,
};

void NopSled(uintptr_t address, std::size_t length);
void PatchBytes(uintptr_t address, const char* bytes, std::size_t length);
void PatchJump(uintptr_t src, uintptr_t dst);
void PatchJumpNoProtect(uintptr_t src, uintptr_t dst);
void* CreateDetour(uintptr_t src, uintptr_t dst);
uintptr_t CalculateJumpOffset(uintptr_t src, uintptr_t dst);
uintptr_t CallToDirectAddress(uintptr_t src);
uintptr_t SlideAddress(uintptr_t base, uintptr_t offset);
uintptr_t SlideAddress(uintptr_t base, uintptr_t old_base, uintptr_t address);
MemPerm Reprotect(uintptr_t address, std::size_t size, MemPerm new_permissions);

template <MemPerm TARGET>
class ReprotectScope {
public:
    ReprotectScope(uintptr_t address, std::size_t size) : address(address), size(size) {
        last_protection = Reprotect(address, size, TARGET);
    }
    ~ReprotectScope() {
        Reprotect(address, size, last_protection);
    }

    ReprotectScope(ReprotectScope const&) = delete;
    void operator=(ReprotectScope const& t) = delete;
    ReprotectScope(ReprotectScope&&) = delete;

private:
    MemPerm last_protection{};
    uintptr_t address{};
    std::size_t size{};
};

template <typename T>
void PatchType(uintptr_t address, T data) {
    // Allow us to read, write and execute the memory region we're modifying
    ReprotectScope<MemPerm::ReadWriteExecute> protection(address, sizeof(T));

    // Write to the region
    T* p = reinterpret_cast<T*>(address);
    *p = data;
}

template <typename T, typename U>
T MakeCallable(U address) {
    return reinterpret_cast<T>(address);
}

enum class CallConvention { CDecl, ClrCall, StdCall, FastCall, ThisCall, VectorCall };
template <CallConvention C, typename T, typename... ARGS>
struct BuildCallable {
    static constexpr auto value = [] {
        if constexpr (C == CallConvention::CDecl) {
            return static_cast<T(_cdecl*)(ARGS...)>(nullptr);
        } else if constexpr (C == CallConvention::ClrCall) {
#ifndef __cplusplus_cli
            static_assert(false, "Requires /clr or /ZW command line option");
#else
            return static_cast<T(__clrcall*)(ARGS...)>(nullptr);
#endif
        } else if constexpr (C == CallConvention::StdCall) {
            return static_cast<T(__stdcall*)(ARGS...)>(nullptr);
        } else if constexpr (C == CallConvention::FastCall) {
            return static_cast<T(__fastcall*)(ARGS...)>(nullptr);
        } else if constexpr (C == CallConvention::ThisCall) {
            return static_cast<T(__thiscall*)(ARGS...)>(nullptr);
        } else if constexpr (C == CallConvention::VectorCall) {
            return static_cast<T(__vectorcall*)(ARGS...)>(nullptr);
        } else {
            static_assert(false, "Invalid call convention");
        }
    }();
};

template <CallConvention C, typename R, typename... ARGS>
R AddressCall(uintptr_t address, ARGS... args) {
    using CALLABLE = decltype(BuildCallable<C, R, ARGS...>::value);
    if constexpr (std::is_void_v<R>) {
        reinterpret_cast<CALLABLE>(address)(args...);
    } else {
        return reinterpret_cast<CALLABLE>(address)(args...);
    }
}

template <typename T>
T CreateDetour(uintptr_t src, uintptr_t dst) {
    // Allow us to modify memory
    ReprotectScope<MemPerm::ReadWriteExecute> protection_main(src, MAX_INS_LENGTH);

    // Get at least 5 bytes of instructions
    std::size_t total_to_copy = 0;
    while (total_to_copy < INS_JMP_LENGTH) {
        total_to_copy += length_disasm(reinterpret_cast<void*>(src + total_to_copy));
    }

    // Code cave for our bytes we copied + a jump instruction
    uintptr_t og_func = reinterpret_cast<uintptr_t>(malloc(total_to_copy + INS_JMP_LENGTH));

    // Mark our region as Read Write Execute
    Reprotect(og_func, total_to_copy + INS_JMP_LENGTH, MemPerm::ReadWriteExecute);

    // Copy our bytes to our code caved section
    std::memcpy(reinterpret_cast<void*>(og_func), reinterpret_cast<void*>(src), total_to_copy);

    // Jump to our original function and continue execution flow
    PatchJumpNoProtect(og_func + total_to_copy, src + total_to_copy);

    // Nop sled if we have more than 5 bytes copied to prevent creating bad instructions
    const std::size_t ins_to_sled = total_to_copy - INS_JMP_LENGTH;
    if (ins_to_sled > 0) {
        NopSled(src + INS_JMP_LENGTH, ins_to_sled);
    }

    // Patch the base of the func to jump to our detour
    PatchJumpNoProtect(src, dst);

    // Return our codecave which jumps to the original function
    return reinterpret_cast<T>(og_func);
}

} // namespace MemoryUtil
