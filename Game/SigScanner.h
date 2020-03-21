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

    void AddNewSignature(const char* _name, const char* _signature, const char* _mask,
                         std::size_t _offset = 0);

    void Scan();
    void Reset();

    DWORD GetScannedAddress(std::string name);
    bool HasFound(std::string name);
    bool HasFoundAll() const;

private:
    struct Signature {
        std::string name{};
        const char* signature{nullptr};
        const char* mask{nullptr};
        std::size_t sig_length{0};
        std::size_t offset{0};
        Signature() = default;

        // A signature is a collection of raw bytes. A mask is whether the byte should compare or
        // not, the format of a mask is either "x" for if we should compare the byte or "?" if we
        // should skip the byte.
        // signature: "\xAA\xBB\xCC\xDD\xEE\xFF"
        // mask: "xx??xx"
        // Will NOT check the bytes \xCC\xDD
        Signature(std::string _name, const char* _signature, const char* _mask,
                  std::size_t _offset = 0) {
            name = _name;
            signature = _signature;
            mask = _mask;
            sig_length = strlen(_mask);
            offset = _offset;
        }
    };

    std::unordered_map<std::string, DWORD> found_sigs{};
    std::vector<Signature> sigs_to_find{};
    std::string module_to_scan{};
    std::optional<DWORD> GetScannedAddressOpt(std::string name);
};
} // namespace MemoryUtil
