#include <sdk/Version.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace mbootcore {
namespace sdk {

std::string SemanticVersion::toString() const {
    std::string result = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (!preRelease.empty()) {
        result += "-" + preRelease;
    }
    if (!buildMetadata.empty()) {
        result += "+" + buildMetadata;
    }
    return result;
}

SemanticVersion SemanticVersion::fromString(const std::string& str) {
    if (str.empty()) return SemanticVersion{0, 0, 0, "", ""};

    SemanticVersion v;

    size_t prePos = std::string::npos;
    size_t buildPos = std::string::npos;

    // Find '+' first (build metadata), then '-' (pre-release) that comes before '+'
    // Valid: "1.2.3-alpha+build" -> pre="alpha", build="build"
    // Build is everything after the first '+'
    buildPos = str.find('+');
    // Pre-release is everything after the first '-' that appears before '+'
    std::string searchRange = (buildPos != std::string::npos) ? str.substr(0, buildPos) : str;
    prePos = searchRange.find('-');

    std::string versionPart;
    std::string prePart;
    std::string buildPart;

    if (prePos != std::string::npos) {
        versionPart = str.substr(0, prePos);
        prePart = searchRange.substr(prePos + 1);
    } else {
        versionPart = (buildPos != std::string::npos) ? str.substr(0, buildPos) : str;
    }

    if (buildPos != std::string::npos) {
        buildPart = str.substr(buildPos + 1);
    }

    // Must have at least one digit before pre-release / build metadata
    if (versionPart.empty()) return SemanticVersion{0, 0, 0, "", ""};

    std::istringstream ss(versionPart);
    std::string token;
    int part = 0;

    while (std::getline(ss, token, '.')) {
        if (part > 2) return SemanticVersion{0, 0, 0, "", ""}; // too many numeric segments
        if (token.empty()) return SemanticVersion{0, 0, 0, "", ""};
        // Check all chars are digits (no leading zeros beyond the digit zero itself is OK per semver)
        for (char c : token) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return SemanticVersion{0, 0, 0, "", ""};
        }
        char* end = nullptr;
        long val = std::strtol(token.c_str(), &end, 10);
        if (*end != '\0' || val < 0) return SemanticVersion{0, 0, 0, "", ""};
        if (part == 0) v.major = static_cast<uint32_t>(val);
        else if (part == 1) v.minor = static_cast<uint32_t>(val);
        else if (part == 2) v.patch = static_cast<uint32_t>(val);
        ++part;
    }

    if (part != 3) return SemanticVersion{0, 0, 0, "", ""}; // must have exactly 3 numeric parts

    v.preRelease = prePart;
    v.buildMetadata = buildPart;
    return v;
}

int SemanticVersion::compare(const SemanticVersion& other) const noexcept {
    if (major != other.major) return (major < other.major) ? -1 : 1;
    if (minor != other.minor) return (minor < other.minor) ? -1 : 1;
    if (patch != other.patch) return (patch < other.patch) ? -1 : 1;

    // No pre-release has higher precedence than with pre-release
    if (preRelease.empty() && !other.preRelease.empty()) return 1;
    if (!preRelease.empty() && other.preRelease.empty()) return -1;
    if (preRelease != other.preRelease) return (preRelease < other.preRelease) ? -1 : 1;

    return 0;
}

bool SemanticVersion::operator==(const SemanticVersion& other) const noexcept {
    return compare(other) == 0;
}
bool SemanticVersion::operator!=(const SemanticVersion& other) const noexcept {
    return compare(other) != 0;
}
bool SemanticVersion::operator<(const SemanticVersion& other) const noexcept {
    return compare(other) < 0;
}
bool SemanticVersion::operator>(const SemanticVersion& other) const noexcept {
    return compare(other) > 0;
}
bool SemanticVersion::operator<=(const SemanticVersion& other) const noexcept {
    return compare(other) <= 0;
}
bool SemanticVersion::operator>=(const SemanticVersion& other) const noexcept {
    return compare(other) >= 0;
}

// ---------------------------------------------------------------------------
// Global helpers for compiler / platform detection
// ---------------------------------------------------------------------------

static CompilerInfo detectCompiler() {
    CompilerInfo info;

#if defined(__clang__)
#if defined(__apple_build_version__)
    info.compilerName = "AppleClang";
    info.compilerId = "AppleClang";
#else
    info.compilerName = "Clang";
    info.compilerId = "Clang";
#endif
    info.compilerVersion =
        std::to_string(__clang_major__) + "." +
        std::to_string(__clang_minor__) + "." +
        std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__) || defined(__GNUG__)
    info.compilerName = "GCC";
    info.compilerId = "GNU";
    info.compilerVersion =
        std::to_string(__GNUC__) + "." +
        std::to_string(__GNUC_MINOR__) + "." +
        std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    info.compilerName = "MSVC";
    info.compilerId = "MSVC";
    info.compilerVersion = std::to_string(_MSC_VER);
#else
    info.compilerName = "Unknown";
    info.compilerId = "Unknown";
    info.compilerVersion = "0.0.0";
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    info.compilerArchitecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    info.compilerArchitecture = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.compilerArchitecture = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
    info.compilerArchitecture = "ARM";
#else
    info.compilerArchitecture = "Unknown";
#endif

#if __cplusplus >= 202302L
    info.cppStandard = "C++23";
#elif __cplusplus >= 202002L
    info.cppStandard = "C++20";
#elif __cplusplus >= 201703L
    info.cppStandard = "C++17";
#elif __cplusplus >= 201402L
    info.cppStandard = "C++14";
#elif __cplusplus >= 201103L
    info.cppStandard = "C++11";
#else
    info.cppStandard = "C++" + std::to_string(__cplusplus);
#endif

    return info;
}

static PlatformInfo detectPlatform() {
    PlatformInfo info;

#if defined(_WIN32)
    info.osName = "Windows";
#elif defined(__linux__)
    info.osName = "Linux";
#elif defined(__APPLE__)
    info.osName = "macOS";
#else
    info.osName = "Unknown";
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    info.osArchitecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    info.osArchitecture = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.osArchitecture = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
    info.osArchitecture = "ARM";
#else
    info.osArchitecture = "Unknown";
#endif

    info.pointerSize = sizeof(void*);

    // Endianness detection via constexpr at runtime
    union { uint32_t i; char c[4]; } endianTest{};
    endianTest.i = 0x01020304;
    info.isLittleEndian = (endianTest.c[0] == 0x04);

    info.numberOfCores = static_cast<uint32_t>(std::thread::hardware_concurrency());

    return info;
}

std::string VersionInfo::toJson() const {
    std::string json;
    json += "{\n";
    json += "  \"sdkVersion\": \"" + sdkVersion.toString() + "\",\n";
    json += "  \"coreVersion\": \"" + coreVersion.toString() + "\",\n";
    json += "  \"buildInfo\": {\n";
    json += "    \"buildDate\": \"" + buildInfo.buildDate + "\",\n";
    json += "    \"buildTime\": \"" + buildInfo.buildTime + "\",\n";
    json += "    \"buildType\": \"" + buildInfo.buildType + "\",\n";
    json += "    \"buildSystem\": \"" + buildInfo.buildSystem + "\",\n";
    json += "    \"buildCounter\": " + std::to_string(buildInfo.buildCounter) + "\n";
    json += "  },\n";
    json += "  \"gitInfo\": {\n";
    json += "    \"commitHash\": \"" + gitInfo.commitHash + "\",\n";
    json += "    \"commitShortHash\": \"" + gitInfo.commitShortHash + "\",\n";
    json += "    \"branch\": \"" + gitInfo.branch + "\",\n";
    json += "    \"tag\": \"" + gitInfo.tag + "\",\n";
    json += "    \"dirty\": " + std::string(gitInfo.dirty ? "true" : "false") + ",\n";
    json += "    \"commitCount\": " + std::to_string(gitInfo.commitCount) + ",\n";
    json += "    \"commitDate\": \"" + gitInfo.commitDate + "\",\n";
    json += "    \"commitMessage\": \"" + gitInfo.commitMessage + "\"\n";
    json += "  },\n";
    json += "  \"compilerInfo\": {\n";
    json += "    \"compilerName\": \"" + compilerInfo.compilerName + "\",\n";
    json += "    \"compilerVersion\": \"" + compilerInfo.compilerVersion + "\",\n";
    json += "    \"compilerId\": \"" + compilerInfo.compilerId + "\",\n";
    json += "    \"compilerArchitecture\": \"" + compilerInfo.compilerArchitecture + "\",\n";
    json += "    \"cppStandard\": \"" + compilerInfo.cppStandard + "\"\n";
    json += "  },\n";
    json += "  \"platformInfo\": {\n";
    json += "    \"osName\": \"" + platformInfo.osName + "\",\n";
    json += "    \"osVersion\": \"" + platformInfo.osVersion + "\",\n";
    json += "    \"osArchitecture\": \"" + platformInfo.osArchitecture + "\",\n";
    json += "    \"pointerSize\": " + std::to_string(platformInfo.pointerSize) + ",\n";
    json += "    \"isLittleEndian\": " + std::string(platformInfo.isLittleEndian ? "true" : "false") + ",\n";
    json += "    \"numberOfCores\": " + std::to_string(platformInfo.numberOfCores) + "\n";
    json += "  }\n";
    json += "}";
    return json;
}

std::string VersionInfo::toYaml() const {
    std::string yaml;
    yaml += "sdkVersion: " + sdkVersion.toString() + "\n";
    yaml += "coreVersion: " + coreVersion.toString() + "\n";
    yaml += "buildInfo:\n";
    yaml += "  buildDate: " + buildInfo.buildDate + "\n";
    yaml += "  buildTime: " + buildInfo.buildTime + "\n";
    yaml += "  buildType: " + buildInfo.buildType + "\n";
    yaml += "  buildSystem: " + buildInfo.buildSystem + "\n";
    yaml += "  buildCounter: " + std::to_string(buildInfo.buildCounter) + "\n";
    yaml += "gitInfo:\n";
    yaml += "  commitHash: " + gitInfo.commitHash + "\n";
    yaml += "  commitShortHash: " + gitInfo.commitShortHash + "\n";
    yaml += "  branch: " + gitInfo.branch + "\n";
    yaml += "  tag: " + gitInfo.tag + "\n";
    yaml += "  dirty: " + std::string(gitInfo.dirty ? "true" : "false") + "\n";
    yaml += "  commitCount: " + std::to_string(gitInfo.commitCount) + "\n";
    yaml += "  commitDate: " + gitInfo.commitDate + "\n";
    yaml += "  commitMessage: " + gitInfo.commitMessage + "\n";
    yaml += "compilerInfo:\n";
    yaml += "  compilerName: " + compilerInfo.compilerName + "\n";
    yaml += "  compilerVersion: " + compilerInfo.compilerVersion + "\n";
    yaml += "  compilerId: " + compilerInfo.compilerId + "\n";
    yaml += "  compilerArchitecture: " + compilerInfo.compilerArchitecture + "\n";
    yaml += "  cppStandard: " + compilerInfo.cppStandard + "\n";
    yaml += "platformInfo:\n";
    yaml += "  osName: " + platformInfo.osName + "\n";
    yaml += "  osVersion: " + platformInfo.osVersion + "\n";
    yaml += "  osArchitecture: " + platformInfo.osArchitecture + "\n";
    yaml += "  pointerSize: " + std::to_string(platformInfo.pointerSize) + "\n";
    yaml += "  isLittleEndian: " + std::string(platformInfo.isLittleEndian ? "true" : "false") + "\n";
    yaml += "  numberOfCores: " + std::to_string(platformInfo.numberOfCores) + "\n";
    return yaml;
}

std::string VersionInfo::toString() const {
    std::string s;
    s += "MBootCore Version Information\n";
    s += "============================\n\n";
    s += "SDK Version    : " + sdkVersion.toString() + "\n";
    s += "Core Version   : " + coreVersion.toString() + "\n\n";
    s += "Build\n";
    s += "  Date         : " + buildInfo.buildDate + "\n";
    s += "  Time         : " + buildInfo.buildTime + "\n";
    s += "  Type         : " + buildInfo.buildType + "\n";
    s += "  System       : " + buildInfo.buildSystem + "\n";
    s += "  Counter      : " + std::to_string(buildInfo.buildCounter) + "\n\n";
    s += "Git\n";
    s += "  Commit       : " + gitInfo.commitHash + "\n";
    s += "  Short        : " + gitInfo.commitShortHash + "\n";
    s += "  Branch       : " + gitInfo.branch + "\n";
    s += "  Tag          : " + gitInfo.tag + "\n";
    s += "  Dirty        : " + std::string(gitInfo.dirty ? "yes" : "no") + "\n";
    s += "  Count        : " + std::to_string(gitInfo.commitCount) + "\n";
    s += "  Date         : " + gitInfo.commitDate + "\n";
    s += "  Message      : " + gitInfo.commitMessage + "\n\n";
    s += "Compiler\n";
    s += "  Name         : " + compilerInfo.compilerName + "\n";
    s += "  Version      : " + compilerInfo.compilerVersion + "\n";
    s += "  ID           : " + compilerInfo.compilerId + "\n";
    s += "  Architecture : " + compilerInfo.compilerArchitecture + "\n";
    s += "  C++ Standard : " + compilerInfo.cppStandard + "\n\n";
    s += "Platform\n";
    s += "  OS           : " + platformInfo.osName + "\n";
    s += "  OS Version   : " + platformInfo.osVersion + "\n";
    s += "  Architecture : " + platformInfo.osArchitecture + "\n";
    s += "  Pointer Size : " + std::to_string(platformInfo.pointerSize) + " bytes\n";
    s += "  Endianness   : " + std::string(platformInfo.isLittleEndian ? "Little" : "Big") + "\n";
    s += "  CPU Cores    : " + std::to_string(platformInfo.numberOfCores);
    return s;
}

const VersionInfo& getVersionInfo() noexcept {
    static const VersionInfo info = []() -> VersionInfo {
        VersionInfo v;
        v.buildInfo.buildDate = __DATE__;
        v.buildInfo.buildTime = __TIME__;
#ifdef MBOOTCORE_BUILD_TYPE
        v.buildInfo.buildType = MBOOTCORE_BUILD_TYPE;
#else
        v.buildInfo.buildType = "Unknown";
#endif
#ifdef MBOOTCORE_BUILD_SYSTEM
        v.buildInfo.buildSystem = MBOOTCORE_BUILD_SYSTEM;
#else
        v.buildInfo.buildSystem = "CMake";
#endif
#ifdef MBOOTCORE_BUILD_COUNTER
        v.buildInfo.buildCounter = MBOOTCORE_BUILD_COUNTER;
#endif
#ifdef MBOOTCORE_GIT_COMMIT
        v.gitInfo.commitHash = MBOOTCORE_GIT_COMMIT;
#endif
#ifdef MBOOTCORE_GIT_SHORT_HASH
        v.gitInfo.commitShortHash = MBOOTCORE_GIT_SHORT_HASH;
#endif
#ifdef MBOOTCORE_GIT_BRANCH
        v.gitInfo.branch = MBOOTCORE_GIT_BRANCH;
#endif
#ifdef MBOOTCORE_GIT_TAG
        v.gitInfo.tag = MBOOTCORE_GIT_TAG;
#endif
#ifdef MBOOTCORE_GIT_DIRTY
        v.gitInfo.dirty = true;
#endif
#ifdef MBOOTCORE_GIT_COMMIT_COUNT
        v.gitInfo.commitCount = MBOOTCORE_GIT_COMMIT_COUNT;
#endif
#ifdef MBOOTCORE_GIT_COMMIT_DATE
        v.gitInfo.commitDate = MBOOTCORE_GIT_COMMIT_DATE;
#endif
#ifdef MBOOTCORE_GIT_COMMIT_MESSAGE
        v.gitInfo.commitMessage = MBOOTCORE_GIT_COMMIT_MESSAGE;
#endif
        v.compilerInfo = detectCompiler();
        v.platformInfo = detectPlatform();
        return v;
    }();
    return info;
}

const SemanticVersion& getSDKVersion() noexcept {
    return getVersionInfo().sdkVersion;
}

const SemanticVersion& getCoreVersion() noexcept {
    return getVersionInfo().coreVersion;
}

} // namespace sdk
} // namespace mbootcore
