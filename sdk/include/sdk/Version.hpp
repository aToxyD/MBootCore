#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <chrono>

namespace mbootcore {
namespace sdk {

struct SemanticVersion {
    uint32_t major{0};
    uint32_t minor{1};
    uint32_t patch{0};
    std::string preRelease;
    std::string buildMetadata;

    std::string toString() const;
    static SemanticVersion fromString(const std::string& str);
    int compare(const SemanticVersion& other) const noexcept;
    bool operator==(const SemanticVersion& other) const noexcept;
    bool operator!=(const SemanticVersion& other) const noexcept;
    bool operator<(const SemanticVersion& other) const noexcept;
    bool operator>(const SemanticVersion& other) const noexcept;
    bool operator<=(const SemanticVersion& other) const noexcept;
    bool operator>=(const SemanticVersion& other) const noexcept;
};

struct BuildInfo {
    std::string buildDate;
    std::string buildTime;
    std::string buildType;
    std::string buildSystem;
    uint64_t buildCounter{0};
};

struct GitInfo {
    std::string commitHash;
    std::string commitShortHash;
    std::string branch;
    std::string tag;
    bool dirty{false};
    uint32_t commitCount{0};
    std::string commitDate;
    std::string commitMessage;
};

struct CompilerInfo {
    std::string compilerName;
    std::string compilerVersion;
    std::string compilerId;
    std::string compilerArchitecture;
    std::string cppStandard;
};

struct PlatformInfo {
    std::string osName;
    std::string osVersion;
    std::string osArchitecture;
    uint32_t pointerSize{sizeof(void*)};
    bool isLittleEndian{true};
    uint32_t numberOfCores{0};
};

struct VersionInfo {
    SemanticVersion sdkVersion{1, 0, 0, "", ""};
    SemanticVersion coreVersion{1, 0, 0, "", ""};
    BuildInfo buildInfo;
    GitInfo gitInfo;
    CompilerInfo compilerInfo;
    PlatformInfo platformInfo;

    std::string toJson() const;
    std::string toYaml() const;
    std::string toString() const;
};

const VersionInfo& getVersionInfo() noexcept;
const SemanticVersion& getSDKVersion() noexcept;
const SemanticVersion& getCoreVersion() noexcept;

} // namespace sdk
} // namespace mbootcore
