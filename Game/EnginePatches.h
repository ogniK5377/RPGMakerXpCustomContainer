#pragma once

namespace Patches {
void SetupDetours(const char* library_path);
void SwapRgssadEncryption(const char* library_path);
void PatchDebugPresent();
} // namespace Patches
