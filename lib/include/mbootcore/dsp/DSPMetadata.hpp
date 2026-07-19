#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <mbootcore/dsp/DSPTypes.hpp>

namespace mbootcore {
namespace dsp {

// A single dependency entry
struct DSPDependency {
    std::string packageId;
    std::string name;
    DSPVersion minVersion;
    DSPVersion maxVersion;
    bool required{true};
};

// Package manifest — maps to manifest.json
struct DSPManifest {
    std::string formatVersion{"1.0"};
    std::string packageId;
    std::string name;
    std::string vendor;
    std::string description;
    DSPVersion version;
    std::vector<std::string> authors;
    std::string license;
    std::vector<std::string> tags;
    std::vector<DSPDependency> dependencies;
    uint64_t fileSize{0};
    uint32_t checksum{0};
    std::string checksumAlgorithm{"none"};
    std::string signatureAlgorithm;
    ByteBuffer signature;
    bool requiresSignature{false};
    bool compressLoaders{false};
    std::string minCoreVersion;
    std::string maxCoreVersion;
    std::chrono::system_clock::time_point buildDate;
    std::chrono::system_clock::time_point releaseDate;
};

// Vendor metadata — maps to vendor.json
struct DSPVendorMetadata {
    discovery::Vendor vendorId{discovery::Vendor::Unknown};
    std::string vendorName;
    std::string displayName;
    std::string website;
    std::vector<std::string> aliases;
    std::vector<std::string> supportedChipsets;
    std::vector<std::string> supportedBootModes;
    std::vector<std::string> supportedProtocols;
    std::vector<std::string> supportedTransports;
    std::string defaultProtocol;
    std::string defaultBootMode;
    std::string defaultTransport;
    std::unordered_map<std::string, std::string> properties;
};

// Chipset metadata — maps to chipsets.json entries
struct DSPChipsetMetadata {
    ChipsetId id;
    std::string displayName;
    std::string manufacturer;
    std::string architecture;
    std::vector<std::string> variants;
    std::vector<std::string> families;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
    std::vector<std::string> compatibleLoaders;
    std::vector<discovery::BootMode> supportedBootModes;
    std::vector<discovery::ProtocolType> supportedProtocols;
    std::vector<StorageType> supportedStorage;
    uint32_t maxMemoryMB{0};
    uint32_t maxFlashSizeMB{0};
    bool hasTrustZone{false};
    bool hasSecureBoot{false};
    bool hasDebugPort{false};
    std::string defaultProgrammer;
    std::chrono::system_clock::time_point releaseDate;
    std::vector<std::string> knownIssues;
    std::unordered_map<std::string, std::string> properties;
};

// Loader metadata — maps to loaders/ entries
struct DSPLoaderMetadata {
    std::string loaderId;
    std::string name;
    std::string fileName;
    std::string filePath;
    std::string description;
    std::vector<ChipsetId> compatibleChipsets;
    std::vector<std::string> compatibleVendors;
    std::vector<discovery::ProtocolType> protocols;
    std::vector<discovery::BootMode> bootModes;
    discovery::BootMode requiredBootMode{discovery::BootMode::Unknown};
    DSPVersion version;
    std::string hashAlgorithm;
    ByteBuffer expectedHash;
    ByteBuffer signature;
    bool isCompressed{false};
    uint64_t originalSize{0};
    uint64_t compressedSize{0};
    uint32_t priority{50};
    bool isFallback{false};
    bool isPreferred{false};
    bool isSigned{false};
    bool requiresAuthentication{false};
    std::vector<std::string> tags;
    std::unordered_map<std::string, std::string> properties;
};

// Boot mode metadata — maps to bootmodes.json entries
struct DSPBootModeMetadata {
    std::string bootModeId;
    discovery::BootMode bootMode{discovery::BootMode::Unknown};
    std::string displayName;
    std::string description;
    std::vector<discovery::ProtocolType> supportedProtocols;
    uint32_t defaultTimeoutMs{30000};
    bool requiresAuth{false};
    bool requireUsb{false};
    bool requireSerial{false};
    bool requireTcp{false};
    std::vector<std::string> requiredKeys;
    std::vector<std::string> quirks;
    std::unordered_map<std::string, std::string> properties;
};

// Transport metadata — maps to transports.json entries
struct DSPTransportMetadata {
    std::string transportId;
    discovery::TransportType transportType{discovery::TransportType::Unknown};
    std::string displayName;
    std::string description;
    std::vector<uint32_t> supportedBaudRates;
    std::vector<uint16_t> supportedVidList;
    std::vector<uint16_t> supportedPidList;
    uint32_t defaultBaudRate{115200};
    uint32_t defaultTimeoutMs{5000};
    uint32_t maxPacketSize{4096};
    bool supportsHotplug{false};
    bool supportsAsync{false};
    std::unordered_map<std::string, std::string> properties;
};

// Protocol metadata — maps to protocols.json entries
struct DSPProtocolMetadata {
    std::string protocolId;
    discovery::ProtocolType protocolType{discovery::ProtocolType::Unknown};
    std::string displayName;
    std::string description;
    DSPVersion minVersion;
    DSPVersion maxVersion;
    std::vector<std::string> requiredLoaders;
    std::vector<std::string> optionalLoaders;
    std::vector<discovery::BootMode> supportedBootModes;
    std::vector<std::string> quirks;
    uint32_t defaultTimeoutMs{30000};
    bool requiresProgrammer{false};
    bool supportsBackup{false};
    bool supportsVerify{false};
    bool supportsResume{false};
    std::unordered_map<std::string, std::string> properties;
};

// Full package metadata — top-level aggregation
struct DSPPackageMetadata {
    DSPManifest manifest;
    DSPVendorMetadata vendor;
    std::vector<DSPChipsetMetadata> chipsets;
    std::vector<DSPLoaderMetadata> loaders;
    std::vector<DSPBootModeMetadata> bootModes;
    std::vector<DSPTransportMetadata> transports;
    std::vector<DSPProtocolMetadata> protocols;
    std::vector<std::string> quirks;
    std::vector<std::string> profiles;
    std::vector<std::string> supportedLocales;
    DSPVersion dspFormatVersion{1,0,0};
    DSPState state{DSPState::Unknown};
    DSPOrigin origin{DSPOrigin::User};
    std::string installPath;
    std::string packageFileName;
    std::chrono::system_clock::time_point installDate;
    std::string repository;
};

// Compatibility info
struct DSPCompatibilityInfo {
    bool coreCompatible{false};
    bool sdkCompatible{false};
    bool osCompatible{false};
    bool architectureCompatible{false};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool isCompatible() const noexcept {
        return coreCompatible && sdkCompatible && osCompatible && architectureCompatible;
    }
};

// Package statistics
struct DSPPackageStatistics {
    std::string packageId;
    std::string name;
    DSPVersion version;
    DSPState state{DSPState::Unknown};
    DSPOrigin origin{DSPOrigin::User};
    uint64_t installedSize{0};
    uint64_t compressedSize{0};
    uint32_t loaderCount{0};
    uint32_t chipsetCount{0};
    uint32_t profileCount{0};
    uint32_t quirkCount{0};
    uint32_t dependentPackages{0};
    uint32_t dependentCount{0};
    std::chrono::system_clock::time_point installDate;
    bool needsUpdate{false};
    std::vector<std::string> tags;
};

} // namespace dsp
} // namespace mbootcore
