#include <Windows.h>
#include <psapi.h>
#include "SigScanner.h"

namespace MemoryUtil {
SigScanner::SigScanner(const char* mod) : module_to_scan(mod) {}
SigScanner::~SigScanner() = default;

void SigScanner::AddNewSignature(const char* _name, const char* _signature, const char* _mask,
                                 std::size_t _offset) {
    sigs_to_find.push_back(Signature{_name, _signature, _mask, _offset});
}

void SigScanner::Scan() {
    // Get the module base address
    const auto base = GetModuleHandle(module_to_scan.c_str());
    if (base == nullptr) {
        return;
    }
    MODULEINFO module_info{};
    // Query the module to get the size of the image so we can find the end address
    if (!GetModuleInformation(GetCurrentProcess(), base, &module_info, sizeof(MODULEINFO))) {
        return;
    }

    char* p = reinterpret_cast<char*>(base);
    char* end = reinterpret_cast<char*>(p + module_info.SizeOfImage);

    // While we're not at the end of our image and we still need to look for signatures
    while (p < end && !sigs_to_find.empty()) {
        // Check all signatures we have left to see if we match
        for (auto it = sigs_to_find.begin(); it != sigs_to_find.end();) {
            bool found = true;
            // For our signature size
            for (std::size_t i = 0; i < it->sig_length; i++) {
                // If our mask is "?" skip over to the next byte and assume this is a "valid match"
                if (it->mask[i] == '?') {
                    continue;
                }
                // Compare if our signature byte matches the memory byte
                if (it->signature[i] != *(p + i)) {
                    found = false;
                    break;
                }
            }
            // If we scanned our whole signature without failing, we found our signature
            if (found) {
                found_sigs[it->name] = reinterpret_cast<uintptr_t>(p) + it->offset;
                it = sigs_to_find.erase(it);
            } else {
                it++;
            }
        }
        p++;
    }

    // Any remaining signatures should considered not found and have a
    for (auto& sig : sigs_to_find) {
        found_sigs[sig.name] = 0; // An address of 0 is considered invalid
    }
    sigs_to_find.clear();
}

void SigScanner::Reset() {
    // Clear any sigs we need to find and the sigs we already found
    found_sigs.clear();
    sigs_to_find.clear();
}

std::optional<uintptr_t> SigScanner::GetScannedAddressOpt(std::string name) {
    // Check if the signature name exists
    auto it = found_sigs.find(name);
    if (it == found_sigs.end()) {
        return {};
    }
    // If we have a zero address, assume it's invalid
    if (it->second == 0) {
        return {};
    }

    // Return the found address
    return it->second;
}

uintptr_t SigScanner::GetScannedAddress(std::string_view name) {
    return GetScannedAddressOpt(name.data()).value_or(0);
}

bool SigScanner::HasFound(std::string_view name) {
    // Check if our address has a non zero value
    return GetScannedAddressOpt(name.data()).has_value();
}

bool SigScanner::HasFoundAll() const {
    // Check every signature
    for (const auto sig : found_sigs) {
        // If our signature failed to be found, bail
        if (sig.second == 0) {
            return false;
        }
    }
    // All sigs are found
    return true;
}

} // namespace MemoryUtil
