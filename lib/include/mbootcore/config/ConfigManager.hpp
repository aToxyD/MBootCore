#pragma once

#include <memory>
#include <string>

#include <mbootcore/config/ConfigTypes.hpp>
#include <mbootcore/domain/Error.hpp>

namespace mbootcore { namespace config {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) noexcept;
    ConfigManager& operator=(ConfigManager&&) noexcept;

    Result<FullConfig> load() const;
    Result<void> save(const FullConfig& config);

    Result<RuntimeConfig> runtimeConfig() const;
    Result<void> setRuntimeConfig(const RuntimeConfig& config);
    Result<TransportConfig> transportConfig() const;
    Result<void> setTransportConfig(const TransportConfig& config);
    Result<WorkflowConfig> workflowConfig() const;
    Result<void> setWorkflowConfig(const WorkflowConfig& config);
    Result<JobConfig> jobConfig() const;
    Result<void> setJobConfig(const JobConfig& config);
    Result<DSPConfig> dspConfig() const;
    Result<void> setDSPConfig(const DSPConfig& config);
    Result<PluginConfig> pluginConfig() const;
    Result<void> setPluginConfig(const PluginConfig& config);
    Result<VendorConfig> vendorConfig() const;
    Result<void> setVendorConfig(const VendorConfig& config);
    Result<GUIConfig> guiConfig() const;
    Result<void> setGUIConfig(const GUIConfig& config);
    Result<CLIConfig> cliConfig() const;
    Result<void> setCLIConfig(const CLIConfig& config);

    Result<void> importJson(const std::string& json);
    Result<std::string> exportJson() const;

    Result<ConfigStatus> validate(const FullConfig& config) const;
    Result<FullConfig> defaults() const;

    Result<FullConfig> migrate(const FullConfig& old, uint32_t fromVersion, uint32_t toVersion);

    Result<void> setCustomOption(const std::string& key, const std::string& value);
    Result<std::string> customOption(const std::string& key) const;

    Result<void> setConfigPath(const std::string& path);
    std::string configPath() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} }
