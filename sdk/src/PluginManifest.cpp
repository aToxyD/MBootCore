#include <sdk/PluginManifest.hpp>
#include <sstream>
#include <algorithm>

namespace mbootcore {
namespace sdk {

PluginManifest::PluginManifest()
    : m_pluginName("unnamed")
    , m_pluginVersion("1.0.0")
    , m_sdkVersion("1.0.0")
    , m_minRuntimeVersion("1.0.0")
    , m_maxRuntimeVersion("1.1.0")
{
}

std::string PluginManifest::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"pluginName\": \"" << m_pluginName << "\",\n";
    ss << "  \"pluginVersion\": \"" << m_pluginVersion << "\",\n";
    ss << "  \"sdkVersion\": \"" << m_sdkVersion << "\",\n";
    ss << "  \"minRuntimeVersion\": \"" << m_minRuntimeVersion << "\",\n";
    ss << "  \"maxRuntimeVersion\": \"" << m_maxRuntimeVersion << "\",\n";
    ss << "  \"vendor\": \"" << m_vendor << "\",\n";
    ss << "  \"author\": \"" << m_author << "\",\n";
    ss << "  \"license\": \"" << m_license << "\",\n";
    ss << "  \"description\": \"" << m_description << "\",\n";

    ss << "  \"dependencies\": [\n";
    for (size_t i = 0; i < m_dependencies.size(); ++i) {
        const auto& d = m_dependencies[i];
        ss << "    {\"name\": \"" << d.name << "\", \"minVersion\": \"" << d.minVersion
           << "\", \"maxVersion\": \"" << d.maxVersion << "\", \"optional\": "
           << (d.optional ? "true" : "false") << "}";
        if (i < m_dependencies.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"entries\": [\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        ss << "    {\"type\": \"" << e.type << "\", \"name\": \"" << e.name
           << "\", \"version\": \"" << e.version << "\", \"vendor\": \"" << e.vendor
           << "\", \"description\": \"" << e.description << "\"}";
        if (i < m_entries.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"capabilities\": [";
    for (size_t i = 0; i < m_capabilities.size(); ++i) {
        ss << "\"" << m_capabilities[i] << "\"";
        if (i < m_capabilities.size() - 1) ss << ", ";
    }
    ss << "]\n";

    ss << "}\n";
    return ss.str();
}

PluginManifest PluginManifest::fromJson(const std::string& json) {
    PluginManifest manifest;
    auto findValue = [&](const std::string& key) -> std::string {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return {};
        pos = json.find(":", pos);
        if (pos == std::string::npos) return {};
        pos = json.find("\"", pos);
        if (pos == std::string::npos) return {};
        ++pos;
        auto end = json.find("\"", pos);
        if (end == std::string::npos) return {};
        return json.substr(pos, end - pos);
    };

    manifest.m_pluginName = findValue("pluginName");
    manifest.m_pluginVersion = findValue("pluginVersion");
    manifest.m_sdkVersion = findValue("sdkVersion");
    manifest.m_minRuntimeVersion = findValue("minRuntimeVersion");
    manifest.m_maxRuntimeVersion = findValue("maxRuntimeVersion");
    manifest.m_vendor = findValue("vendor");
    manifest.m_author = findValue("author");
    manifest.m_license = findValue("license");
    manifest.m_description = findValue("description");

    return manifest;
}

bool PluginManifest::isValid() const {
    return !m_pluginName.empty() && !m_pluginVersion.empty();
}

std::vector<std::string> PluginManifest::validate() const {
    std::vector<std::string> errors;
    if (m_pluginName.empty()) errors.push_back("Plugin name is required");
    if (m_pluginVersion.empty()) errors.push_back("Plugin version is required");
    if (m_sdkVersion.empty()) errors.push_back("SDK version is required");
    return errors;
}

} // namespace sdk
} // namespace mbootcore
