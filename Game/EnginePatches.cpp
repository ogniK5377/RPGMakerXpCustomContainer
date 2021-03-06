#include <array>
#include <Windows.h>
#include "AriMath.h"
#include "Common.h"
#include "EnginePatches.h"
#include "Input.h"
#include "MemoryUtil.h"
#include "NewInput.h"
#include "RPG_Game.h"
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

using GameClassConstructorType = Memory::Game*(__thiscall*)(Memory::Game*);
GameClassConstructorType O_GameClassConstructor{};

Memory::Game* __fastcall ConstructGameClass(Memory::Game* game) {
    RPGGameClass = O_GameClassConstructor(game);
    return RPGGameClass;
}

void GrabGameClassAddress(const char* library_path) {
    MemoryUtil::SigScanner scanner(library_path);
    scanner.AddNewSignature("InitializeGameClass", "\xE8\x00\x00\x00\x00\x89\x45\xE4\xEB\x07",
                            "x????xxxxx");
    scanner.Scan();
    if (scanner.HasFoundAll()) {
        const auto init_addr = MemoryUtil::InstructionToDirectAddress(
            scanner.GetScannedAddress("InitializeGameClass"));
        O_GameClassConstructor = MemoryUtil::CreateDetour<GameClassConstructorType>(
            init_addr, reinterpret_cast<uintptr_t>(&ConstructGameClass));
    }
}

void SetupDetours(const char* library_path) {
    auto* common = Ruby::Common::Get();
    common->AddNewModule(&RubyModule::RegisterAriMath);
    common->AddNewModule(&RubyModule::RegisterCustomInput);
    common->Initialize(library_path);
}

/* Encryption key swap */
constexpr std::array PATCHED_HEADER{'\x31', '\xac', '\x3e', '\x2d', '\x9b', '\x23', '\xda', '\x11'};
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
        std::array<char, 32> header{};
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

void PatchBindings(const char* library_path) {
    MemoryUtil::SigScanner scanner(library_path);
    scanner.AddNewSignature("CRxInput::Poll", "\xE8\x00\x00\x00\x00\x6A\x1E\x8B\x45\xF0",
                            "x????xxxxx");
    scanner.AddNewSignature("ChangeScreenMode", "\x8B\x44\x24\x04\x8B\x91\x00\x00\x00\x00",
                            "xxxxxx????");
    scanner.AddNewSignature("RegisterInputModule",
                            "\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5D\xC3\xCC\xCC\xCC\xCC",
                            "x????x????xxxxxx");
    scanner.Scan();
    if (scanner.HasFoundAll()) {
        const auto poll_address =
            MemoryUtil::InstructionToDirectAddress(scanner.GetScannedAddress("CRxInput::Poll"));
        const auto register_input_address = MemoryUtil::InstructionToDirectAddress(
            scanner.GetScannedAddress("RegisterInputModule"));
        const auto global_input_module_address = register_input_address + 5;
        using PollType = void(__thiscall*)(Memory::CRxInput*);
        using RegisterInputType = void(__cdecl*)();
        // We're completely overriding the function, we don't need the original address
        Input::POLL_ADDRESS = poll_address;
        RubyModule::InputModule = reinterpret_cast<RB_VALUE*>(
            MemoryUtil::ReadType<uintptr_t>(global_input_module_address));

        // MemoryUtil::CreateDetour<PollType>(poll_address,
        // reinterpret_cast<uintptr_t>(&Input::Poll));
        MemoryUtil::CreateDetour<RegisterInputType>(
            register_input_address, reinterpret_cast<uintptr_t>(&RubyModule::RegisterCustomInput));

        Input::ChangeScreenModeAddress = scanner.GetScannedAddress("ChangeScreenMode");
    }
}

/* Allow the game to run out of focus */
// This patch specifically patches the WM_ACTIVATEAPP to always specify that
// the window is always in focus
// 8B 4D 10            mov ecx, [ebp+wParam]
// 89 88 14 01 00 00   mov[eax + window_active], ecx
void RunOutOfFocus(const char* library_path) {
    MemoryUtil::SigScanner scanner(library_path);
    scanner.AddNewSignature("WndProc::WindowActive",
                            "\x8B\x4D\x10\x89\x88\x00\x00\x00\x00\x8B\x55\x9C\x8B\x8A\x00\x00\x00"
                            "\x00\xE8\x00\x00\x00\x00\x33\xC0\xEB\x5E",
                            "xxxxx????xxxxx????x????xxxx");
    scanner.Scan();
    if (scanner.HasFoundAll()) {
        // We're doing only a 3 byte patch to avoid code caving as
        // mov ecx, [ebp+wParam]
        // is 3 bytes long
        static constexpr std::array patch{
            '\x31', '\xc9', // XOR ECX, ECX
            '\x41'          // INC ECX
        };
        MemoryUtil::PatchBytes(scanner.GetScannedAddress("WndProc::WindowActive"), patch.data(),
                               patch.size());
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
    auto addr = reinterpret_cast<uintptr_t>(GetProcAddress(kernel32, "IsDebuggerPresent"));
    if (addr == 0) {
        return;
    }

    // EAX stores the return of IsDebuggerPresent()
    static constexpr std::array patch{
        '\x31', '\xC0', // xor eax, eax
        '\xC3'          // ret
    };
    MemoryUtil::PatchBytes(addr, patch.data(), patch.size());
}

} // namespace Patches
