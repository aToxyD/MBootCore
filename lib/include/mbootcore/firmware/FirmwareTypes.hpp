#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore {
namespace firmware {

// ============================================================
// Enums
// ============================================================

enum class ImageFormat : uint32_t {
    Raw     = 0,
    ELF     = 1,
    Sparse  = 2
};

enum class ImageType : uint32_t {
    Programmer   = 0,
    Bootloader   = 1,
    GPT          = 2,
    Boot         = 3,
    System       = 4,
    Vendor       = 5,
    Userdata     = 6,
    Recovery     = 7,
    Cache        = 8,
    Custom       = 99
};

enum class FlashStepType : uint32_t {
    FlashProgrammer = 0,
    UpdateGPT       = 1,
    FlashPartition  = 2,
    VerifyPartition = 3,
    ErasePartition  = 4,
    Reboot          = 5,
    Custom          = 99
};

enum class ValidationLevel : uint32_t {
    None   = 0,
    Basic  = 1,
    Full   = 2,
    Strict = 3
};

enum class FirmwareError : uint32_t {
    InvalidFormat         = 0x0D00,
    ImageNotFound         = 0x0D01,
    HashMismatch          = 0x0D02,
    InvalidManifest       = 0x0D03,
    DependencyConflict    = 0x0D04,
    UnsupportedDevice     = 0x0D05,
    UnsupportedVendor     = 0x0D06,
    UnsupportedStorage    = 0x0D07,
    DuplicatePartition    = 0x0D08,
    MissingProgrammer     = 0x0D09,
    ValidationFailed      = 0x0D0A,
    ExtractionFailed      = 0x0D0B,
    PackageCorrupted      = 0x0D0C,
    VersionMismatch       = 0x0D0D,
    PackageNotFound       = 0x0D0E,
    NotEnoughImages       = 0x0D0F
};

// ============================================================
// Structs — forward declarations
// ============================================================

struct FirmwareVersion {
    uint32_t major{0};
    uint32_t minor{0};
    uint32_t patch{0};
    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
    bool operator==(const FirmwareVersion& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
    bool operator<(const FirmwareVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
};

struct FirmwareDependency {
    std::string id;
    std::string name;
    FirmwareVersion minVersion;
    bool required{true};
    bool resolved{false};
};

struct FirmwareSignature {
    std::string algorithm{"SHA256"};
    ByteBuffer data;
    ByteBuffer certificate;
    bool valid{false};
};

struct FirmwareImageInfo {
    std::string name;
    ImageType type{ImageType::Custom};
    ImageFormat format{ImageFormat::Raw};
    std::string partitionName;
    uint64_t offset{0};
    uint64_t size{0};
    std::string hashAlgorithm;
    ByteBuffer expectedHash;
    std::string sourceFile;
    bool isExecutable{false};
    uint64_t loadAddress{0};
    uint64_t entryPoint{0};
};

struct PackageMetadata {
    std::string vendor;
    std::string platform;
    std::string chipset;
    std::string protocol;
    std::string mode;
    FirmwareVersion version;
    std::string buildDate;
    std::string author;
    std::string packageUuid;
    uint64_t packageSize{0};
    std::string description;
};

struct PackageManifest {
    std::string manifestVersion{"1.0"};
    std::string description;
    std::vector<FirmwareDependency> dependencies;
    std::vector<FirmwareImageInfo> images;
    std::vector<std::string> requiredCapabilities;
};

struct ValidationError {
    FirmwareError error{FirmwareError::ValidationFailed};
    std::string message;
    std::string context; // e.g., image name, partition name
};

struct ValidationResult {
    bool valid{false};
    std::vector<ValidationError> errors;
    std::vector<std::string> warnings;
};

struct FirmwareImage {
    FirmwareImageInfo info;
    bool dataLoaded{false};
    ByteBuffer data;
    bool verified{false};
};

struct FlashStep {
    FlashStepType type{FlashStepType::Custom};
    std::string description;
    std::string partitionName;
    std::string imageName;
    uint64_t offset{0};
    uint64_t size{0};
    uint64_t sectorSize{512};
    ByteBuffer data;
    ByteBuffer originalData;
    bool requireBackup{false};
    bool optional{false};
};

struct FlashPlan {
    std::vector<FlashStep> steps;
    bool requireProgrammer{false};
    bool requireGptUpdate{false};
    uint64_t totalBytes{0};
    uint64_t estimatedTimeMs{0};
};

// Forward declarations
class FirmwarePackage;
class IFirmwareReader;

} // namespace firmware
} // namespace mbootcore
