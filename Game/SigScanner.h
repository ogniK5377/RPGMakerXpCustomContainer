#pragma once
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <Windows.h>

namespace MemoryUtil {

class SigScanner {
public:
    explicit SigScanner(const char* mod);
    ~SigScanner();

    void AddNewSignature(const char* _name, const char* _signature, std::string _mask,
                         std::size_t _offset = 0);

    void Scan();
    void Reset();

    uintptr_t GetScannedAddress(std::string_view name);
    bool HasFound(std::string_view name);
    bool HasFoundAll() const;

private:
    struct Signature {
        std::string name{};
        std::vector<char> signature{};
        std::string mask{};
        std::size_t sig_length{0};
        std::size_t offset{0};
        Signature() = default;

        // A signature is a collection of raw bytes. A mask is whether the byte should compare or
        // not, the format of a mask is either "x" for if we should compare the byte or "?" if we
        // should skip the byte.
        // signature: "\xAA\xBB\xCC\xDD\xEE\xFF"
        // mask: "xx??xx"
        // Will NOT check the bytes \xCC\xDD
        Signature(std::string _name, const std::vector<char> _signature, std::string _mask,
                  std::size_t _offset = 0) {
            name = _name;
            signature = _signature;
            mask = _mask;
            sig_length = _mask.size();
            offset = _offset;
        }

        inline bool operator==(const Signature& rhs) {
            return name == rhs.name && sig_length == rhs.sig_length && offset == rhs.offset &&
                   !strncmp(signature.data(), rhs.signature.data(), sig_length) &&
                   !strncmp(mask.data(), rhs.mask.data(), sig_length);
        }
        inline bool operator!=(const Signature& rhs) {
            return !operator==(rhs);
        }
    };

    struct SignaturePair {
        Signature signature{};
        uintptr_t address{};
    };

    struct CacheSignature {
        Signature signature{};
        bool found{};
        uintptr_t offset_from_base{};
    };

    struct CacheModule {
        std::string module_name{};
        std::vector<CacheSignature> signatures{};
    };

    void SaveResultToCache(uintptr_t base);
    std::vector<SigScanner::CacheModule> ReadFromCache();

    std::unordered_map<std::string, SignaturePair> found_sigs{};
    std::vector<Signature> sigs_to_find{};
    std::string module_to_scan{};
    std::optional<uintptr_t> GetScannedAddressOpt(std::string name);
};
} // namespace MemoryUtil
