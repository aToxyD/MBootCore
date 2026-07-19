#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace mbootcore {
namespace tools {

struct PluginTemplateInfo {
    std::string name;
    std::string description;
    std::vector<std::string> files;
};

class PluginWizard {
public:
    PluginWizard();

    bool createPlugin(const std::string& type, const std::string& name,
                      const std::string& outputDir,
                      const std::unordered_map<std::string, std::string>& variables);

    bool createVendorPlugin(const std::string& name, const std::string& outputDir);
    bool createProtocolPlugin(const std::string& name, const std::string& protocolType,
                              const std::string& outputDir);
    bool createWorkflowPlugin(const std::string& name, const std::string& outputDir);
    bool createJobPlugin(const std::string& name, const std::string& outputDir);
    bool createTransportPlugin(const std::string& name, const std::string& outputDir);

    std::vector<PluginTemplateInfo> availableTemplates() const;
    std::string generateProjectFile(const std::string& name,
                                     const std::unordered_map<std::string, std::string>& vars) const;

    void setTemplatesPath(const std::string& path) { m_templatesPath = path; }
    const std::string& templatesPath() const noexcept { return m_templatesPath; }

private:
    std::string m_templatesPath;
    bool copyTemplate(const std::string& templateDir, const std::string& outputDir,
                       const std::unordered_map<std::string, std::string>& vars) const;
    std::string substitute(const std::string& content,
                           const std::unordered_map<std::string, std::string>& vars) const;
};

} // namespace tools
} // namespace mbootcore
