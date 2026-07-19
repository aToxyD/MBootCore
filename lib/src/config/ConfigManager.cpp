#include <mbootcore/config/ConfigManager.hpp>

#include "SafeParser.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace mbootcore { namespace config {

namespace fs = std::filesystem;


struct ConfigManager::Impl {
    FullConfig config;
    std::string configPath;
    bool configPathSet{false};
};

ConfigManager::ConfigManager()
    : m_impl(std::make_unique<Impl>()) {}

ConfigManager::~ConfigManager() = default;
ConfigManager::ConfigManager(ConfigManager&&) noexcept = default;
ConfigManager& ConfigManager::operator=(ConfigManager&&) noexcept = default;

static Result<FullConfig> parseConfigFromJson(const std::string& jsonStr, FullConfig base);

Result<FullConfig> ConfigManager::load() const {
    if (!m_impl->configPathSet) {
        return m_impl->config;
    }
    std::ifstream file(m_impl->configPath);
    if (!file.is_open()) {
        return m_impl->config;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseConfigFromJson(buffer.str(), m_impl->config);
}

Result<void> ConfigManager::save(const FullConfig& config) {
    m_impl->config = config;
    if (!m_impl->configPathSet) {
        return {};
    }
    auto json = exportJson();
    if (json.isError()) {
        return json.error();
    }
    std::ofstream file(m_impl->configPath);
    if (!file.is_open()) {
        return ErrorCode::InvalidArgument;
    }
    file << json.value();
    return {};
}

Result<RuntimeConfig> ConfigManager::runtimeConfig() const {
    return m_impl->config.runtime;
}

Result<void> ConfigManager::setRuntimeConfig(const RuntimeConfig& config) {
    m_impl->config.runtime = config;
    return {};
}

Result<TransportConfig> ConfigManager::transportConfig() const {
    return m_impl->config.transport;
}

Result<void> ConfigManager::setTransportConfig(const TransportConfig& config) {
    m_impl->config.transport = config;
    return {};
}

Result<WorkflowConfig> ConfigManager::workflowConfig() const {
    return m_impl->config.workflow;
}

Result<void> ConfigManager::setWorkflowConfig(const WorkflowConfig& config) {
    m_impl->config.workflow = config;
    return {};
}

Result<JobConfig> ConfigManager::jobConfig() const {
    return m_impl->config.job;
}

Result<void> ConfigManager::setJobConfig(const JobConfig& config) {
    m_impl->config.job = config;
    return {};
}

Result<DSPConfig> ConfigManager::dspConfig() const {
    return m_impl->config.dsp;
}

Result<void> ConfigManager::setDSPConfig(const DSPConfig& config) {
    m_impl->config.dsp = config;
    return {};
}

Result<PluginConfig> ConfigManager::pluginConfig() const {
    return m_impl->config.plugin;
}

Result<void> ConfigManager::setPluginConfig(const PluginConfig& config) {
    m_impl->config.plugin = config;
    return {};
}

Result<VendorConfig> ConfigManager::vendorConfig() const {
    return m_impl->config.vendor;
}

Result<void> ConfigManager::setVendorConfig(const VendorConfig& config) {
    m_impl->config.vendor = config;
    return {};
}

Result<GUIConfig> ConfigManager::guiConfig() const {
    return m_impl->config.gui;
}

Result<void> ConfigManager::setGUIConfig(const GUIConfig& config) {
    m_impl->config.gui = config;
    return {};
}

Result<CLIConfig> ConfigManager::cliConfig() const {
    return m_impl->config.cli;
}

Result<void> ConfigManager::setCLIConfig(const CLIConfig& config) {
    m_impl->config.cli = config;
    return {};
}

static void setValueFromJson(RuntimeConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "threadPoolSize") { auto r = fromCharsUint32(value); if (r.ok) cfg.threadPoolSize = r.value; }
    else if (key == "operationTimeoutMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.operationTimeoutMs = r.value; }
    else if (key == "enableLogging") cfg.enableLogging = (value == "true");
    else if (key == "enableTelemetry") cfg.enableTelemetry = (value == "true");
    else if (key == "enableProfiling") cfg.enableProfiling = (value == "true");
    else if (key == "logLevel") cfg.logLevel = value;
    else if (key == "tempDirectory") cfg.tempDirectory = value;
}

static void setValueFromJson(TransportConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "usbTimeoutMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.usbTimeoutMs = r.value; }
    else if (key == "serialTimeoutMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.serialTimeoutMs = r.value; }
    else if (key == "tcpTimeoutMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.tcpTimeoutMs = r.value; }
    else if (key == "maxRetries") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxRetries = r.value; }
    else if (key == "enableKeepAlive") cfg.enableKeepAlive = (value == "true");
    else if (key == "keepAliveIntervalMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.keepAliveIntervalMs = r.value; }
    else if (key == "bufferSize") { auto r = fromCharsUint32(value); if (r.ok) cfg.bufferSize = r.value; }
}

static void setValueFromJson(WorkflowConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "maxRetries") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxRetries = r.value; }
    else if (key == "retryDelayMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.retryDelayMs = r.value; }
    else if (key == "enableRollback") cfg.enableRollback = (value == "true");
    else if (key == "enableCheckpoints") cfg.enableCheckpoints = (value == "true");
    else if (key == "enableValidation") cfg.enableValidation = (value == "true");
    else if (key == "maxParallelStages") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxParallelStages = r.value; }
}

static void setValueFromJson(JobConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "maxRetries") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxRetries = r.value; }
    else if (key == "retryDelayMs") { auto r = fromCharsUint32(value); if (r.ok) cfg.retryDelayMs = r.value; }
    else if (key == "maxParallel") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxParallel = r.value; }
    else if (key == "enableRecovery") cfg.enableRecovery = (value == "true");
    else if (key == "enableHistory") cfg.enableHistory = (value == "true");
    else if (key == "historyLimit") { auto r = fromCharsUint32(value); if (r.ok) cfg.historyLimit = r.value; }
}

static void setValueFromJson(DSPConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "enableVerification") cfg.enableVerification = (value == "true");
    else if (key == "enableCaching") cfg.enableCaching = (value == "true");
    else if (key == "cacheSizeMb") { auto r = fromCharsUint32(value); if (r.ok) cfg.cacheSizeMb = r.value; }
    else if (key == "repositoryPath") cfg.repositoryPath = value;
}

static void setValueFromJson(PluginConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "enableVerification") cfg.enableVerification = (value == "true");
    else if (key == "enableHotReload") cfg.enableHotReload = (value == "true");
    else if (key == "maxPlugins") { auto r = fromCharsUint32(value); if (r.ok) cfg.maxPlugins = r.value; }
}

static void setValueFromJson(VendorConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "enableAutoDetect") cfg.enableAutoDetect = (value == "true");
}

static void setValueFromJson(GUIConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "enableAnimations") cfg.enableAnimations = (value == "true");
    else if (key == "enableTrayIcon") cfg.enableTrayIcon = (value == "true");
    else if (key == "theme") cfg.theme = value;
    else if (key == "language") cfg.language = value;
    else if (key == "windowWidth") { auto r = fromCharsUint32(value); if (r.ok) cfg.windowWidth = r.value; }
    else if (key == "windowHeight") { auto r = fromCharsUint32(value); if (r.ok) cfg.windowHeight = r.value; }
    else if (key == "maximizeOnStart") cfg.maximizeOnStart = (value == "true");
}

static void setValueFromJson(CLIConfig& cfg, const std::string& key, const std::string& value) {
    if (key == "enableColor") cfg.enableColor = (value == "true");
    else if (key == "enableProgress") cfg.enableProgress = (value == "true");
    else if (key == "outputFormat") cfg.outputFormat = value;
    else if (key == "verbosity") { auto r = fromCharsUint32(value); if (r.ok) cfg.verbosity = r.value; }
}

static Result<FullConfig> parseConfigFromJson(const std::string& jsonStr, FullConfig base) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonStr);
    } catch (...) {
        return ErrorCode::InvalidArgument;
    }
    if (!json.is_object()) return ErrorCode::InvalidArgument;

    {
        auto obj = json.value("runtime", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.runtime, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("transport", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.transport, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("workflow", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.workflow, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("job", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.job, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("dsp", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.dsp, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("plugin", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.plugin, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("vendor", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.vendor, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("gui", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.gui, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    {
        auto obj = json.value("cli", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                setValueFromJson(base.cli, it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
            }
        }
    }

    if (json.contains("source")) {
        auto src = json.value("source", std::string{});
        if (src == "file") base.source = ConfigSource::File;
        else if (src == "environment") base.source = ConfigSource::Environment;
        else if (src == "api") base.source = ConfigSource::API;
        else if (src == "cli") base.source = ConfigSource::CLI;
        else base.source = ConfigSource::Default;
    }

    {
        auto obj = json.value("customOptions", nlohmann::json::object());
        if (obj.is_object()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                base.customOptions[it.key()] = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
            }
        }
    }

    {
        auto arr = json.value("trustedVendors", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) {
                if (item.is_string()) base.dsp.trustedVendors.push_back(item.get<std::string>());
            }
        }
    }

    {
        auto arr = json.value("pluginPaths", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) {
                if (item.is_string()) base.plugin.pluginPaths.push_back(item.get<std::string>());
            }
        }
    }

    {
        auto arr = json.value("blacklist", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) {
                if (item.is_string()) base.plugin.blacklist.push_back(item.get<std::string>());
            }
        }
    }

    {
        auto arr = json.value("preferredVendors", nlohmann::json::array());
        if (arr.is_array()) {
            for (const auto& item : arr) {
                if (item.is_string()) base.vendor.preferredVendors.push_back(item.get<std::string>());
            }
        }
    }

    return base;
}

Result<void> ConfigManager::importJson(const std::string& json) {
    auto parsed = parseConfigFromJson(json, m_impl->config);
    if (parsed.isError()) {
        return parsed.error();
    }
    m_impl->config = std::move(parsed.value());
    return {};
}

Result<std::string> ConfigManager::exportJson() const {
    try {
        nlohmann::json j;

        nlohmann::json runtime;
        runtime["threadPoolSize"] = m_impl->config.runtime.threadPoolSize;
        runtime["operationTimeoutMs"] = m_impl->config.runtime.operationTimeoutMs;
        runtime["enableLogging"] = m_impl->config.runtime.enableLogging;
        runtime["enableTelemetry"] = m_impl->config.runtime.enableTelemetry;
        runtime["enableProfiling"] = m_impl->config.runtime.enableProfiling;
        runtime["logLevel"] = m_impl->config.runtime.logLevel;
        runtime["tempDirectory"] = m_impl->config.runtime.tempDirectory;
        j["runtime"] = std::move(runtime);

        nlohmann::json transport;
        transport["usbTimeoutMs"] = m_impl->config.transport.usbTimeoutMs;
        transport["serialTimeoutMs"] = m_impl->config.transport.serialTimeoutMs;
        transport["tcpTimeoutMs"] = m_impl->config.transport.tcpTimeoutMs;
        transport["maxRetries"] = m_impl->config.transport.maxRetries;
        transport["enableKeepAlive"] = m_impl->config.transport.enableKeepAlive;
        transport["keepAliveIntervalMs"] = m_impl->config.transport.keepAliveIntervalMs;
        transport["bufferSize"] = m_impl->config.transport.bufferSize;
        j["transport"] = std::move(transport);

        nlohmann::json workflow;
        workflow["maxRetries"] = m_impl->config.workflow.maxRetries;
        workflow["retryDelayMs"] = m_impl->config.workflow.retryDelayMs;
        workflow["enableRollback"] = m_impl->config.workflow.enableRollback;
        workflow["enableCheckpoints"] = m_impl->config.workflow.enableCheckpoints;
        workflow["enableValidation"] = m_impl->config.workflow.enableValidation;
        workflow["maxParallelStages"] = m_impl->config.workflow.maxParallelStages;
        j["workflow"] = std::move(workflow);

        nlohmann::json job;
        job["maxRetries"] = m_impl->config.job.maxRetries;
        job["retryDelayMs"] = m_impl->config.job.retryDelayMs;
        job["maxParallel"] = m_impl->config.job.maxParallel;
        job["enableRecovery"] = m_impl->config.job.enableRecovery;
        job["enableHistory"] = m_impl->config.job.enableHistory;
        job["historyLimit"] = m_impl->config.job.historyLimit;
        j["job"] = std::move(job);

        nlohmann::json dsp;
        dsp["enableVerification"] = m_impl->config.dsp.enableVerification;
        dsp["enableCaching"] = m_impl->config.dsp.enableCaching;
        dsp["cacheSizeMb"] = m_impl->config.dsp.cacheSizeMb;
        dsp["repositoryPath"] = m_impl->config.dsp.repositoryPath;
        if (!m_impl->config.dsp.trustedVendors.empty()) {
            dsp["trustedVendors"] = m_impl->config.dsp.trustedVendors;
        }
        j["dsp"] = std::move(dsp);

        nlohmann::json plugin;
        plugin["enableVerification"] = m_impl->config.plugin.enableVerification;
        plugin["enableHotReload"] = m_impl->config.plugin.enableHotReload;
        plugin["maxPlugins"] = m_impl->config.plugin.maxPlugins;
        if (!m_impl->config.plugin.pluginPaths.empty()) {
            plugin["pluginPaths"] = m_impl->config.plugin.pluginPaths;
        }
        if (!m_impl->config.plugin.blacklist.empty()) {
            plugin["blacklist"] = m_impl->config.plugin.blacklist;
        }
        j["plugin"] = std::move(plugin);

        nlohmann::json vendor;
        vendor["enableAutoDetect"] = m_impl->config.vendor.enableAutoDetect;
        if (!m_impl->config.vendor.preferredVendors.empty()) {
            vendor["preferredVendors"] = m_impl->config.vendor.preferredVendors;
        }
        if (!m_impl->config.vendor.vendorOptions.empty()) {
            nlohmann::json opts = nlohmann::json::object();
            for (const auto& [k, v] : m_impl->config.vendor.vendorOptions) {
                opts[k] = v;
            }
            vendor["vendorOptions"] = std::move(opts);
        }
        j["vendor"] = std::move(vendor);

        nlohmann::json gui;
        gui["enableAnimations"] = m_impl->config.gui.enableAnimations;
        gui["enableTrayIcon"] = m_impl->config.gui.enableTrayIcon;
        gui["theme"] = m_impl->config.gui.theme;
        gui["language"] = m_impl->config.gui.language;
        gui["windowWidth"] = m_impl->config.gui.windowWidth;
        gui["windowHeight"] = m_impl->config.gui.windowHeight;
        gui["maximizeOnStart"] = m_impl->config.gui.maximizeOnStart;
        j["gui"] = std::move(gui);

        nlohmann::json cli;
        cli["enableColor"] = m_impl->config.cli.enableColor;
        cli["enableProgress"] = m_impl->config.cli.enableProgress;
        cli["outputFormat"] = m_impl->config.cli.outputFormat;
        cli["verbosity"] = m_impl->config.cli.verbosity;
        j["cli"] = std::move(cli);

        switch (m_impl->config.source) {
            case ConfigSource::File: j["source"] = "file"; break;
            case ConfigSource::Environment: j["source"] = "environment"; break;
            case ConfigSource::API: j["source"] = "api"; break;
            case ConfigSource::CLI: j["source"] = "cli"; break;
            default: j["source"] = "default"; break;
        }

        if (!m_impl->config.customOptions.empty()) {
            nlohmann::json opts = nlohmann::json::object();
            for (const auto& [k, v] : m_impl->config.customOptions) {
                opts[k] = v;
            }
            j["customOptions"] = std::move(opts);
        }

        return j.dump(2);
    } catch (...) {
        return ErrorCode::Unknown;
    }
}

static bool validateRuntime(const RuntimeConfig& cfg) {
    if (cfg.threadPoolSize == 0) return false;
    if (cfg.operationTimeoutMs == 0) return false;
    return true;
}

static bool validateTransport(const TransportConfig& cfg) {
    if (cfg.usbTimeoutMs == 0) return false;
    if (cfg.serialTimeoutMs == 0) return false;
    if (cfg.tcpTimeoutMs == 0) return false;
    if (cfg.maxRetries == 0) return false;
    if (cfg.bufferSize == 0) return false;
    return true;
}

static bool validateWorkflow(const WorkflowConfig& cfg) {
    if (cfg.maxRetries == 0) return false;
    if (cfg.retryDelayMs == 0) return false;
    if (cfg.maxParallelStages == 0) return false;
    return true;
}

static bool validateJob(const JobConfig& cfg) {
    if (cfg.maxRetries == 0) return false;
    if (cfg.retryDelayMs == 0) return false;
    if (cfg.maxParallel == 0) return false;
    if (cfg.historyLimit == 0) return false;
    return true;
}

static bool validateDSP(const DSPConfig& cfg) {
    if (cfg.cacheSizeMb == 0) return false;
    if (cfg.repositoryPath.empty()) return false;
    return true;
}

static bool validatePlugin(const PluginConfig& cfg) {
    if (cfg.maxPlugins == 0) return false;
    return true;
}

static bool validateGUI(const GUIConfig& cfg) {
    if (cfg.windowWidth == 0) return false;
    if (cfg.windowHeight == 0) return false;
    return true;
}

static bool validateCLI(const CLIConfig& cfg) {
    if (cfg.outputFormat.empty()) return false;
    return true;
}

Result<ConfigStatus> ConfigManager::validate(const FullConfig& config) const {
    if (!validateRuntime(config.runtime)) return ConfigStatus::Invalid;
    if (!validateTransport(config.transport)) return ConfigStatus::Invalid;
    if (!validateWorkflow(config.workflow)) return ConfigStatus::Invalid;
    if (!validateJob(config.job)) return ConfigStatus::Invalid;
    if (!validateDSP(config.dsp)) return ConfigStatus::Invalid;
    if (!validatePlugin(config.plugin)) return ConfigStatus::Invalid;
    if (!validateGUI(config.gui)) return ConfigStatus::Invalid;
    if (!validateCLI(config.cli)) return ConfigStatus::Invalid;
    return ConfigStatus::Valid;
}

Result<FullConfig> ConfigManager::defaults() const {
    return FullConfig{};
}

Result<FullConfig> ConfigManager::migrate(const FullConfig& old, uint32_t fromVersion, uint32_t toVersion) {
    FullConfig result = old;
    if (fromVersion < 1 && toVersion >= 1) {
        if (result.runtime.threadPoolSize < 2) result.runtime.threadPoolSize = 4;
        if (result.transport.maxRetries < 1) result.transport.maxRetries = 3;
    }
    if (fromVersion < 2 && toVersion >= 2) {
        if (result.job.historyLimit == 0) result.job.historyLimit = 1000;
        if (result.workflow.maxParallelStages == 0) result.workflow.maxParallelStages = 4;
    }
    return result;
}

Result<void> ConfigManager::setCustomOption(const std::string& key, const std::string& value) {
    m_impl->config.customOptions[key] = value;
    return {};
}

Result<std::string> ConfigManager::customOption(const std::string& key) const {
    auto it = m_impl->config.customOptions.find(key);
    if (it == m_impl->config.customOptions.end()) {
        return ErrorCode::InvalidArgument;
    }
    return it->second;
}

Result<void> ConfigManager::setConfigPath(const std::string& path) {
    m_impl->configPath = path;
    m_impl->configPathSet = true;
    return {};
}

std::string ConfigManager::configPath() const {
    return m_impl->configPath;
}

} }
