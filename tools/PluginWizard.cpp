#include "PluginWizard.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace mbootcore {
namespace tools {

PluginWizard::PluginWizard()
{
    m_templatesPath = fs::current_path().string() + "/templates/plugins";
    if (!fs::exists(m_templatesPath)) {
        m_templatesPath = fs::current_path().string() + "/../templates/plugins";
    }
    if (!fs::exists(m_templatesPath)) {
        auto exePath = fs::current_path();
        for (int i = 0; i < 5; ++i) {
            exePath = exePath.parent_path();
            auto candidate = (exePath / "templates/plugins").string();
            if (fs::exists(candidate)) {
                m_templatesPath = candidate;
                break;
            }
        }
    }
}

bool PluginWizard::createPlugin(const std::string& type, const std::string& name,
                                 const std::string& outputDir,
                                 const std::unordered_map<std::string, std::string>& variables)
{
    (void)name;
    std::string templateDir = m_templatesPath + "/" + type;
    if (!fs::exists(templateDir)) {
        std::cerr << "Template not found: " << templateDir << "\n";
        return false;
    }

    fs::path out(outputDir);
    std::error_code ec;
    fs::create_directories(out, ec);
    if (ec) {
        std::cerr << "Failed to create output directory: " << outputDir << "\n";
        return false;
    }

    return copyTemplate(templateDir, outputDir, variables);
}

bool PluginWizard::createVendorPlugin(const std::string& name, const std::string& outputDir)
{
    std::unordered_map<std::string, std::string> vars;
    vars["PLUGIN_NAME"] = name;
    vars["PLUGIN_CLASS"] = name + "VendorPlugin";
    vars["PLUGIN_TYPE"] = "vendor";
    return createPlugin("vendor", name, outputDir, vars);
}

bool PluginWizard::createProtocolPlugin(const std::string& name, const std::string& protocolType,
                                         const std::string& outputDir)
{
    std::unordered_map<std::string, std::string> vars;
    vars["PLUGIN_NAME"] = name;
    vars["PLUGIN_CLASS"] = name + "ProtocolPlugin";
    vars["PLUGIN_TYPE"] = "protocol";
    vars["PROTOCOL_TYPE"] = protocolType;
    return createPlugin("protocol", name, outputDir, vars);
}

bool PluginWizard::createWorkflowPlugin(const std::string& name, const std::string& outputDir)
{
    std::unordered_map<std::string, std::string> vars;
    vars["PLUGIN_NAME"] = name;
    vars["PLUGIN_CLASS"] = name + "WorkflowPlugin";
    vars["PLUGIN_TYPE"] = "workflow";
    return createPlugin("workflow", name, outputDir, vars);
}

bool PluginWizard::createJobPlugin(const std::string& name, const std::string& outputDir)
{
    std::unordered_map<std::string, std::string> vars;
    vars["PLUGIN_NAME"] = name;
    vars["PLUGIN_CLASS"] = name + "JobPlugin";
    vars["PLUGIN_TYPE"] = "job";
    return createPlugin("job", name, outputDir, vars);
}

bool PluginWizard::createTransportPlugin(const std::string& name, const std::string& outputDir)
{
    std::unordered_map<std::string, std::string> vars;
    vars["PLUGIN_NAME"] = name;
    vars["PLUGIN_CLASS"] = name + "TransportPlugin";
    vars["PLUGIN_TYPE"] = "transport";
    return createPlugin("transport", name, outputDir, vars);
}

std::vector<PluginTemplateInfo> PluginWizard::availableTemplates() const
{
    std::vector<PluginTemplateInfo> templates;

    if (!fs::exists(m_templatesPath)) {
        PluginTemplateInfo fallback;
        fallback.name = "default";
        fallback.description = "Default templates (built-in)";
        fallback.files = {"CMakeLists.txt", "Plugin.cpp", "Plugin.hpp"};
        templates.push_back(fallback);
        return templates;
    }

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(m_templatesPath, ec)) {
        if (!entry.is_directory()) continue;
        PluginTemplateInfo info;
        info.name = entry.path().filename().string();
        info.description = info.name + " plugin template";

        for (const auto& f : fs::recursive_directory_iterator(entry.path(), ec)) {
            if (f.is_regular_file()) {
                info.files.push_back(f.path().filename().string());
            }
        }
        templates.push_back(info);
    }

    return templates;
}

std::string PluginWizard::generateProjectFile(const std::string& name,
                                               const std::unordered_map<std::string, std::string>& vars) const
{
    std::ostringstream ss;
    ss << "cmake_minimum_required(VERSION 3.16)\n";
    ss << "project(" << name << " VERSION 1.0.0 LANGUAGES CXX)\n\n";
    ss << "set(CMAKE_CXX_STANDARD 17)\n";
    ss << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
    if (vars.find("PLUGIN_TYPE") != vars.end()) {
        ss << "# " << vars.at("PLUGIN_TYPE") << " plugin\n\n";
    }

    ss << "find_package(mbootcore REQUIRED)\n\n";

    ss << "add_library(${PROJECT_NAME} SHARED\n";
    ss << "    src/" << name << "Plugin.cpp\n";
    ss << ")\n\n";

    ss << "target_include_directories(${PROJECT_NAME}\n";
    ss << "    PUBLIC\n";
    ss << "        ${CMAKE_CURRENT_SOURCE_DIR}/include\n";
    ss << "    PRIVATE\n";
    ss << "        ${mbootcore_INCLUDE_DIRS}\n";
    ss << ")\n\n";

    ss << "target_link_libraries(${PROJECT_NAME}\n";
    ss << "    PRIVATE\n";
    ss << "        mbootcore::mbootcore\n";
    ss << ")\n\n";

    ss << "install(TARGETS ${PROJECT_NAME}\n";
    ss << "    LIBRARY DESTINATION lib/mbootcore/plugins\n";
    ss << ")\n";

    return ss.str();
}

bool PluginWizard::copyTemplate(const std::string& templateDir, const std::string& outputDir,
                                 const std::unordered_map<std::string, std::string>& vars) const
{
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(templateDir, ec)) {
        if (!entry.is_regular_file()) continue;

        auto relPath = fs::relative(entry.path(), templateDir);
        auto outPath = fs::path(outputDir) / relPath;

        fs::create_directories(outPath.parent_path(), ec);
        if (ec) {
            std::cerr << "Failed to create directory: " << outPath.parent_path() << "\n";
            continue;
        }

        std::ifstream ifs(entry.path());
        if (!ifs.is_open()) {
            std::cerr << "Failed to read: " << entry.path() << "\n";
            continue;
        }

        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());

        std::string processed = substitute(content, vars);

        std::ofstream ofs(outPath);
        if (!ofs.is_open()) {
            std::cerr << "Failed to write: " << outPath << "\n";
            continue;
        }
        ofs << processed;
        std::cout << "Created: " << outPath << "\n";
    }

    return true;
}

std::string PluginWizard::substitute(const std::string& content,
                                     const std::unordered_map<std::string, std::string>& vars) const
{
    std::string result = content;
    for (const auto& [key, value] : vars) {
        std::string pattern = "%" + key + "%";
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.length(), value);
            pos += value.length();
        }
    }
    return result;
}

} // namespace tools
} // namespace mbootcore
