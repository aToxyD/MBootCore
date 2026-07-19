#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>

namespace mbootcore {
namespace dsp {

// Package state
enum class DSPState : uint32_t {
    Unknown   = 0,
    Installed = 1,
    Enabled   = 2,
    Disabled  = 3,
    Corrupted = 4,
    Incompatible = 5
};

// Package origin
enum class DSPOrigin : uint32_t {
    Unknown  = 0xFF,
    System   = 0,
    User     = 1,
    Portable = 2,
    Online   = 3,
    Builtin  = 4
};

// Validation result
enum class DSPValidationLevel : uint32_t {
    None    = 0,
    Basic   = 1,
    Full    = 2,
    Strict  = 3
};

// Package format version
struct DSPVersion {
    uint32_t major{1};
    uint32_t minor{0};
    uint32_t patch{0};

    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    bool operator==(const DSPVersion& o) const noexcept {
        return major == o.major && minor == o.minor && patch == o.patch;
    }
    bool operator<(const DSPVersion& o) const noexcept {
        if (major != o.major) return major < o.major;
        if (minor != o.minor) return minor < o.minor;
        return patch < o.patch;
    }
};

// A single chipset identifier
struct ChipsetId {
    std::string vendor;
    std::string family;
    std::string variant;
    std::string toString() const {
        return vendor + "/" + family + (variant.empty() ? "" : "/" + variant);
    }
};

// Storage type abstraction
enum class StorageType : uint32_t {
    Unknown   = 0,
    UFS       = 1,
    eMMC      = 2,
    NAND      = 3,
    NOR       = 4,
    SPI       = 5,
    SD        = 6,
    NVMe      = 7
};

// DSP error codes
enum class DSPError : uint32_t {
    PackageNotFound         = 0x1000,
    PackageCorrupted        = 0x1001,
    PackageIncompatible     = 0x1002,
    PackageConflict         = 0x1003,
    PackageAlreadyInstalled = 0x1004,
    PackageNotInstalled     = 0x1005,
    PackageDisabled         = 0x1006,
    ValidationFailed        = 0x1007,
    ChecksumMismatch        = 0x1008,
    SignatureInvalid        = 0x1009,
    ManifestInvalid         = 0x100A,
    MetadataMissing         = 0x100B,
    LoaderMissing           = 0x100C,
    DependencyMissing       = 0x100D,
    CircularDependency      = 0x100E,
    VersionMismatch         = 0x100F,
    InstallFailed           = 0x1010,
    UninstallFailed         = 0x1011,
    UpdateFailed            = 0x1012,
    EnableFailed            = 0x1013,
    DisableFailed           = 0x1014,
    RepairFailed            = 0x1015,
    CacheCorrupted          = 0x1016,
    RepositoryUnavailable   = 0x1017,
    DatabaseError           = 0x1018,
    ProfileNotFound         = 0x1019,
    QuirkNotFound           = 0x101A,
    QuirkConflict           = 0x101B,
    LoaderRejected          = 0x101C,
    LoaderInvalid           = 0x101D,
    ChipsetNotFound         = 0x101E,
    ChipsetConflict         = 0x101F,
    BuilderError            = 0x1020,
    MetadataError           = 0x1021,
    DatabaseCorrupted       = 0x1022
};

// Forward declarations
struct DSPMetadata;
struct VendorMetadata;
struct ChipsetMetadata;
struct LoaderMetadata;
struct BootModeMetadata;
struct TransportMetadata;
struct ProtocolMetadata;
struct DeviceProfile;
struct CompatibilityInfo;
struct PackageStatistics;
struct HardwareProfile;
struct StorageProfile;
struct MemoryProfile;
struct PartitionLayout;
struct FlashGeometry;
struct Capabilities;
struct SecurityProfile;
struct PerformanceProfile;
struct VendorQuirk;
struct QuirkPolicy;

class DSPManager;
class DSPRepository;
class LoaderIndex;
class LoaderDatabase;
class LoaderMatcher;
class LoaderResolver;
class LoaderCache;
class LoaderVerifier;
class HardwareProfileManager;
class QuirkDatabase;
class QuirkResolver;
class DSPValidator;
class DSPDependencyGraph;
class DSPDependencyResolver;
class DSPCache;
class DSPBuilder;
class DSPInspector;

} // namespace dsp
} // namespace mbootcore
