#include <array>
#include <Windows.h>
#include "AriMath.h"
#include "Common.h"
#include "EnginePatches.h"
#include "MemoryUtil.h"
#include "RubyCommon.h"
#include "SigScanner.h"

namespace Patches {

// TODO(David): Custom Container loader
/*
using PrepareRGSSADType = bool (*)(LPCSTR lpFileName);
using ReadRGSSADType = bool (*)(char* filename, char** file_buffer, int* size);
using MallocType = void* (*)(unsigned int size);

PrepareRGSSADType ORIGINAL_PREPARE_RGSSAD = nullptr;
ReadRGSSADType ORIGINAL_READ_RGSSAD = nullptr;
MallocType RGSSAD_MALLOC = nullptr;
*/

void SetupDetours(const char* library_path) {
    Ruby::Common::Get()->AddNewModule(&RubyModule::RegisterAriMath);
    Ruby::Common::Get()->Initialize(library_path);
}

/* Encryption key swap */
constexpr std::array<char, 8> PATCHED_HEADER{'\x31', '\xac', '\x3e', '\x2d',
                                             '\x9b', '\x23', '\xda', '\x11'};
constexpr unsigned int NEW_KEY = 0xBCF33B95;
void SwapRgssadEncryption(const char* library_path) {
    MemoryUtil::SigScanner scanner(library_path);
    // Locate the RGSSAD header to change the bytes to something else. The fields for the header are
    // not actually meaningful and are just checked against a block of memory to make sure they
    // match
    scanner.AddNewSignature(
        "RGSSAD_Header",
        "\x52\x00\x00\x00\x47\x00\x00\x00\x53\x00\x00\x00\x53\x00\x00\x00\x41\x00\x00"
        "\x00\x44\x00\x00\x00\x00\x00\x00\x00\x01",
        "x???x???x???x???x???x???x???x");

    // Locate the base RGSSAD encryption key
    scanner.AddNewSignature("RGSSAD_Key", "\xFE\xCA\xAD\xDE", "xxxx");

    scanner.Scan();

    // Only sig patch if we've found both the header and the key
    if (scanner.HasFoundAll()) {
        auto header_address = scanner.GetScannedAddress("RGSSAD_Header");
        auto key_address = scanner.GetScannedAddress("RGSSAD_Key");

        // Copy our new header bytes and write
        std::array<char, 32> header;
        std::memcpy(header.data(), reinterpret_cast<void*>(header_address), header.size());
        for (std::size_t i = 0; i < PATCHED_HEADER.size(); i++) {
            header[i * 4] = PATCHED_HEADER[i];
        }

        // Overwrite the header to the new header
        MemoryUtil::PatchBytes(header_address, header.data(), header.size());

        // Change the encryption key used for """decrypting""" RGSSADs
        MemoryUtil::PatchType<unsigned int>(key_address, NEW_KEY);
    } else {
        LOG("Incorrect RGSSAD dll supplied!");
    }
}

/* Allow debugger patch */
// If we're running from a debugger or within MSVC, we'll get a debugger is attached error. We
// patch out the IsDebuggerPresent() check just to allow the ability to debug
void PatchDebugPresent() {
    const auto kernel32 = GetModuleHandle("Kernel32.dll");
    if (kernel32 == 0) {
        return;
    }
    auto addr = GetProcAddress(kernel32, "IsDebuggerPresent");
    if (addr == 0) {
        return;
    }

    // EAX stores the return of IsDebuggerPresent()
    std::array<char, 3> patch{
        '\x31', '\xC0', // xor eax, eax
        '\xC3'          // ret
    };
    MemoryUtil::PatchBytes(reinterpret_cast<DWORD>(addr), patch.data(), patch.size());
}

} // namespace Patches
