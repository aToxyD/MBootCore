#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <chrono>

namespace mbootcore {
namespace sdk {

struct ManifestDependency {
    std::string name;
    std::string minVersion;
    std::string maxVersion;
    bool optional{false};
};

struct ManifestEntry {
    std::string type;
    std::string name;
    std::string version;
    std::string vendor;
    std::string description;
    std::unordered_map<std::string, std::string> attributes;
};

class PluginManifest {
public:
    PluginManifest();

    void setPluginName(const std::string& name) { m_pluginName = name; }
    void setPluginVersion(const std::string& version) { m_pluginVersion = version; }
    void setSDKVersion(const std::string& version) { m_sdkVersion = version; }
    void setMinRuntimeVersion(const std::string& version) { m_minRuntimeVersion = version; }
    void setMaxRuntimeVersion(const std::string& version) { m_maxRuntimeVersion = version; }
    void setVendor(const std::string& vendor) { m_vendor = vendor; }
    void setAuthor(const std::string& author) { m_author = author; }
    void setLicense(const std::string& license) { m_license = license; }
    void setDescription(const std::string& desc) { m_description = desc; }

    const std::string& pluginName() const noexcept { return m_pluginName; }
    const std::string& pluginVersion() const noexcept { return m_pluginVersion; }
    const std::string& sdkVersion() const noexcept { return m_sdkVersion; }
    const std::string& minRuntimeVersion() const noexcept { return m_minRuntimeVersion; }
    const std::string& maxRuntimeVersion() const noexcept { return m_maxRuntimeVersion; }
    const std::string& vendor() const noexcept { return m_vendor; }
    const std::string& author() const noexcept { return m_author; }
    const std::string& license() const noexcept { return m_license; }
    const std::string& description() const noexcept { return m_description; }

    void addDependency(const ManifestDependency& dep) { m_dependencies.push_back(dep); }
    const std::vector<ManifestDependency>& dependencies() const noexcept { return m_dependencies; }

    void addEntry(const ManifestEntry& entry) { m_entries.push_back(entry); }
    const std::vector<ManifestEntry>& entries() const noexcept { return m_entries; }

    void setCapabilities(const std::vector<std::string>& caps) { m_capabilities = caps; }
    const std::vector<std::string>& capabilities() const noexcept { return m_capabilities; }

    std::string toJson() const;
    static PluginManifest fromJson(const std::string& json);

    bool isValid() const;
    std::vector<std::string> validate() const;

private:
    std::string m_pluginName;
    std::string m_pluginVersion;
    std::string m_sdkVersion;
    std::string m_minRuntimeVersion;
    std::string m_maxRuntimeVersion;
    std::string m_vendor;
    std::string m_author;
    std::string m_license;
    std::string m_description;
    std::vector<ManifestDependency> m_dependencies;
    std::vector<ManifestEntry> m_entries;
    std::vector<std::string> m_capabilities;
};

} // namespace sdk
} // namespace mbootcore
