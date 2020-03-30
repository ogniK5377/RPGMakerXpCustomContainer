#pragma once
#include <cstdint>

namespace Memory {
struct Game;
}

namespace Patches {
inline Memory::Game* RPGGameClass{};
void GrabGameClassAddress(const char* library_path);
void SetupDetours(const char* library_path);
void SwapRgssadEncryption(const char* library_path);
void PatchBindings(const char* library_path);
void PatchDebugPresent();
} // namespace Patches
