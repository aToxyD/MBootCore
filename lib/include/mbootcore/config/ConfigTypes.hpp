#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace mbootcore { namespace config {

enum class ConfigSource : uint32_t { Default=0, File=1, Environment=2, API=3, CLI=4 };
enum class ConfigStatus : uint32_t { Valid=0, Invalid=1, Missing=2, Deprecated=3, Migrated=4 };

struct RuntimeConfig {
    uint32_t threadPoolSize{4};
    uint32_t operationTimeoutMs{30000};
    bool enableLogging{true};
    bool enableTelemetry{false};
    bool enableProfiling{false};
    std::string logLevel{"info"};
    std::string tempDirectory{"/tmp/mbootcore"};
};

struct TransportConfig {
    uint32_t usbTimeoutMs{5000};
    uint32_t serialTimeoutMs{5000};
    uint32_t tcpTimeoutMs{10000};
    uint32_t maxRetries{3};
    bool enableKeepAlive{true};
    uint32_t keepAliveIntervalMs{1000};
    uint32_t bufferSize{65536};
};

struct WorkflowConfig {
    uint32_t maxRetries{3};
    uint32_t retryDelayMs{1000};
    bool enableRollback{true};
    bool enableCheckpoints{true};
    bool enableValidation{true};
    uint32_t maxParallelStages{4};
};

struct JobConfig {
    uint32_t maxRetries{3};
    uint32_t retryDelayMs{1000};
    uint32_t maxParallel{4};
    bool enableRecovery{true};
    bool enableHistory{true};
    uint32_t historyLimit{1000};
};

struct DSPConfig {
    bool enableVerification{true};
    bool enableCaching{true};
    uint32_t cacheSizeMb{256};
    std::string repositoryPath{"/etc/mbootcore/dsp"};
    std::vector<std::string> trustedVendors;
};

struct PluginConfig {
    bool enableVerification{true};
    bool enableHotReload{false};
    uint32_t maxPlugins{100};
    std::vector<std::string> pluginPaths;
    std::vector<std::string> blacklist;
};

struct VendorConfig {
    bool enableAutoDetect{true};
    std::vector<std::string> preferredVendors;
    std::map<std::string,std::string> vendorOptions;
};

struct GUIConfig {
    bool enableAnimations{true};
    bool enableTrayIcon{true};
    std::string theme{"dark"};
    std::string language{"auto"};
    uint32_t windowWidth{1280};
    uint32_t windowHeight{720};
    bool maximizeOnStart{false};
};

struct CLIConfig {
    bool enableColor{true};
    bool enableProgress{true};
    std::string outputFormat{"text"};
    uint32_t verbosity{1};
};

struct FullConfig {
    RuntimeConfig runtime;
    TransportConfig transport;
    WorkflowConfig workflow;
    JobConfig job;
    DSPConfig dsp;
    PluginConfig plugin;
    VendorConfig vendor;
    GUIConfig gui;
    CLIConfig cli;
    ConfigSource source{ConfigSource::Default};
    std::map<std::string,std::string> customOptions;
};

} }
