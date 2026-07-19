#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace mbootcore {

struct LoaderMetadata {
    std::string vendor;
    std::string protocol;
    std::string chipset;
    uint32_t msmId{0};
    uint32_t oemId{0};
    uint32_t modelId{0};
    std::vector<uint8_t> pkhash;
    std::string version;
    std::string build;
    uint32_t securityVersion{0};
    std::string storageType;
    std::vector<std::string> supportedMemory;
    uint64_t loaderSize{0};
    std::vector<uint8_t> hash;
    std::string hashAlgorithm;
    bool isSigned{false};
    bool isEncrypted{false};
    std::string signatureState;
    std::vector<std::string> capabilities;
    std::map<std::string, std::string> raw;

    std::string pkhashHex() const;
    static LoaderMetadata fromPkhash(const std::vector<uint8_t>& hashData);
};

inline std::string LoaderMetadata::pkhashHex() const {
    if (pkhash.empty()) return {};
    std::string hex;
    hex.reserve(pkhash.size() * 2);
    static constexpr char table[] = "0123456789ABCDEF";
    for (auto b : pkhash) {
        hex += table[(b >> 4) & 0xF];
        hex += table[b & 0xF];
    }
    return hex;
}

inline LoaderMetadata LoaderMetadata::fromPkhash(const std::vector<uint8_t>& hashData) {
    LoaderMetadata meta;
    meta.pkhash = hashData;
    return meta;
}

} // namespace mbootcore
