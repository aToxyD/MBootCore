#pragma once

#include <sdk/Version.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace sdk {

struct SDKComponent {
    std::string name;
    std::string version;
    std::string status;  // "available", "missing", "incompatible"
    std::string detail;
};

struct SDKCapability {
    std::string name;
    std::string description;
    bool available{false};
};

struct SDKInfo {
    VersionInfo versionInfo;
    std::vector<SDKComponent> components;
    std::vector<SDKCapability> capabilities;
    std::vector<std::string> supportedPlatforms;
    std::vector<std::string> availableTransports;
    std::vector<std::string> availableProtocols;
    std::vector<std::string> availableFeatures;

    std::string toJson() const;
    std::string toString() const;
    static SDKInfo collect();
};

class SDKDoctor {
public:
    SDKDoctor();

    struct DiagnosticResult {
        bool passed{true};
        std::string check;
        std::string message;
        std::vector<std::string> details;
        std::string severity;  // "info", "warning", "error"
    };

    struct DoctorReport {
        bool allPassed{false};
        std::vector<DiagnosticResult> results;
        int passedCount{0};
        int warningCount{0};
        int errorCount{0};
        std::string summary;
    };

    DoctorReport runAllChecks();
    DiagnosticResult checkInstallation();
    DiagnosticResult checkCompiler();
    DiagnosticResult checkDependencies();
    DiagnosticResult checkRuntime();
    DiagnosticResult checkPlugins();
    DiagnosticResult checkEnvironment();

    std::string generateReport(const DoctorReport& report) const;

private:
    bool isCompilerSupported() const;
};

class EnvironmentChecker {
public:
    EnvironmentChecker();

    struct EnvironmentReport {
        bool valid{false};
        std::string osName;
        std::string osVersion;
        std::string compilerName;
        std::string compilerVersion;
        std::string cmakeVersion;
        std::string architecture;
        std::vector<std::string> availablePaths;
        std::vector<std::string> missingDependencies;
        std::vector<std::string> warnings;
    };

    EnvironmentReport check() const;
    bool hasCMake() const;
    bool hasGit() const;
    bool hasPython3() const;
    bool hasCompiler() const;
    std::vector<std::string> findSDKPaths() const;
};

} // namespace sdk
} // namespace mbootcore
