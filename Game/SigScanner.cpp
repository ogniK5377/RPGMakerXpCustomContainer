#include <array>
#include <vector>
#include <Windows.h>
#include <psapi.h>
#include "FileIO.h"
#include "SigScanner.h"

namespace MemoryUtil {
SigScanner::SigScanner(const char* mod) : module_to_scan(mod) {}
SigScanner::~SigScanner() = default;

void SigScanner::AddNewSignature(const char* _name, const char* _signature, std::string _mask,
                                 std::size_t _offset) {
    std::vector<char> signature_bytes(_mask.length());
    std::memcpy(signature_bytes.data(), _signature, signature_bytes.size());
    sigs_to_find.push_back(Signature{_name, signature_bytes, _mask, _offset});
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

    auto cache = ReadFromCache();
    for (auto& cache_module : cache) {
        if (cache_module.module_name != module_to_scan) {
            continue;
        }
        for (auto it = sigs_to_find.begin(); it != sigs_to_find.end();) {
            for (auto& cache_sig : cache_module.signatures) {
                if (cache_sig.signature != *it) {
                    continue;
                }
                if (cache_sig.found) {
                    // TODO(David M): Validate signature to see if it's still valid
                    found_sigs[cache_sig.signature.name].address =
                        reinterpret_cast<uintptr_t>(base) + cache_sig.offset_from_base;
                    found_sigs[cache_sig.signature.name].signature = cache_sig.signature;
                    it = sigs_to_find.erase(it);
                    break;
                }
            }
        }
    }

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
                found_sigs[it->name].signature = *it;
                found_sigs[it->name].address = reinterpret_cast<uintptr_t>(p) + it->offset;
                it = sigs_to_find.erase(it);
            } else {
                it++;
            }
        }
        p++;
    }

    // Any remaining signatures should considered not found and have a
    for (auto& sig : sigs_to_find) {
        found_sigs[sig.name].address = 0; // An address of 0 is considered invalid
        found_sigs[sig.name].signature = sig;
    }
    sigs_to_find.clear();

    SaveResultToCache(reinterpret_cast<uintptr_t>(base));
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
    if (it->second.address == 0) {
        return {};
    }

    // Return the found address
    return it->second.address;
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
        if (sig.second.address == 0) {
            return false;
        }
    }
    // All sigs are found
    return true;
}

// Magic header for cache container
static constexpr std::array<char, 4> MAGIC = {'C', 'C', 'H', 'E'};

void SigScanner::SaveResultToCache(uintptr_t base) {
    // Get the current cache result
    auto saved_cache = ReadFromCache();
    bool is_module_in_cache = false;

    // Check our cache
    for (auto& cache_module : saved_cache) {
        // Only update our current scanned module
        if (cache_module.module_name != module_to_scan) {
            continue;
        }
        is_module_in_cache = true;

        // Check all our signatures after the scan
        for (auto& sig : found_sigs) {
            bool found = false;
            // Check our cache
            for (auto& cache_sig : cache_module.signatures) {

                // Find the matching signature
                if (cache_sig.signature == sig.second.signature) {
                    found = true;
                    // Check if it's a valid signature or needs to be updated
                    if ((cache_sig.offset_from_base != (sig.second.address - base) &&
                         sig.second.address != 0) ||
                        (!cache_sig.found && sig.second.address != 0)) {
                        // Update our signature
                        cache_sig.found = true;
                        cache_sig.offset_from_base = sig.second.address - base;
                    }
                    break;
                }
            }

            // If the signature doesn't exist, add it
            if (!found) {
                CacheSignature cache_signature{};
                cache_signature.signature = sig.second.signature;
                cache_signature.found = sig.second.address != 0;
                cache_signature.offset_from_base = sig.second.address - base;
                cache_module.signatures.push_back(cache_signature);
            }
        }
    }

    // If our module doesn't exist, add it to the cache and our signatures
    if (!is_module_in_cache) {
        SigScanner::CacheModule cache_module{};
        cache_module.module_name = module_to_scan;
        for (auto& sig : found_sigs) {
            SigScanner::CacheSignature cache_signature{};
            cache_signature.found = sig.second.address != 0;
            if (cache_signature.found) {
                cache_signature.offset_from_base = sig.second.address - base;
            }
            cache_signature.signature = sig.second.signature;
            cache_module.signatures.push_back(cache_signature);
        }
        saved_cache.push_back(cache_module);
    }

    // Cache saving
    IO::File fp{"signatures.cache", "wb"};
    if (!fp.Open()) {
        return;
    }

    // Magic
    fp.WriteBuffer(MAGIC.data(), 4);

    // Module Count
    fp.Write<std::size_t>(saved_cache.size());

    for (const auto& cache_module : saved_cache) {
        // Module Name
        fp.Write(cache_module.module_name);

        // Signature count
        std::size_t signature_count{};
        for (const auto& signature : cache_module.signatures) {
            if (signature.found) {
                signature_count++;
            }
        }
        fp.Write<std::size_t>(signature_count);

        for (const auto& signature : cache_module.signatures) {
            if (!signature.found) {
                continue;
            }

            const auto& sig = signature.signature;
            // Signature name
            fp.Write(sig.name);

            // Signature Length
            fp.Write<std::size_t>(sig.sig_length);

            // Signature
            fp.WriteBuffer(sig.signature.data(), sig.sig_length);

            // Mask
            fp.WriteBuffer(sig.mask.data(), sig.sig_length);

            // Module Offset
            fp.Write<std::size_t>(sig.offset);

            // Offset from base
            fp.Write<uintptr_t>(signature.offset_from_base);
        }
    }
}

std::vector<SigScanner::CacheModule> SigScanner::ReadFromCache() {
    std::vector<SigScanner::CacheModule> cache{};
    IO::File fp{"signatures.cache", "rb"};
    if (!fp.Open()) {
        return cache;
    }

    // Read magic
    std::vector<char> magic(4);
    fp.ReadBuffer(magic.data(), 4);
    if (memcmp(MAGIC.data(), magic.data(), 4) != 0) {
        return cache;
    }

    // Loop all modules
    const auto module_count = fp.Read<std::size_t>();
    for (std::size_t i = 0; i < module_count; i++) {
        const auto module_name = fp.Read<std::string>();
        const auto signature_count = fp.Read<std::size_t>();

        SigScanner::CacheModule cache_module{};
        cache_module.module_name = module_name;
        // Loop all signatures for module
        for (std::size_t j = 0; j < signature_count; j++) {
            const auto signature_name = fp.Read<std::string>();
            const auto signature_length = fp.Read<std::size_t>();
            std::vector<char> signature(signature_length);
            std::vector<char> mask(signature_length);

            fp.ReadBuffer(signature.data(), signature.size());
            fp.ReadBuffer(mask.data(), mask.size());

            const auto signature_offset = fp.Read<std::size_t>();
            const auto signature_offset_from_base = fp.Read<std::size_t>();

            SigScanner::CacheSignature cache_signatures{};
            cache_signatures.found = true;
            cache_signatures.offset_from_base = signature_offset_from_base;

            cache_signatures.signature.name = signature_name;
            cache_signatures.signature.mask = std::string(mask.data());
            cache_signatures.signature.signature = signature;

            cache_signatures.signature.sig_length = signature_length;
            cache_signatures.signature.offset = signature_offset;

            cache_module.signatures.push_back(cache_signatures);
        }
        cache.push_back(cache_module);
    }
    return cache;
}

} // namespace MemoryUtil
