#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

namespace mbootcore {
namespace tools {

struct DocClass {
    std::string name;
    std::string m_namespaceName;
    std::string file;
    std::string brief;
    std::vector<std::string> methods;
    std::vector<std::string> members;
    std::vector<std::string> baseClasses;
    bool isInterface{false};
    bool isEnum{false};
    std::vector<std::string> enumValues;
};

struct DocModule {
    std::string name;
    std::string description;
    std::vector<std::string> headers;
    std::vector<DocClass> classes;
    std::vector<std::string> dependencies;
};

struct DocConfig {
    std::string outputDir{"./docs/generated"};
    std::vector<std::string> headerPaths;
    std::vector<std::string> skipDirs;
    bool generateMarkdown{true};
    bool generateHtml{false};
    bool generateJson{false};
    bool generateDot{false};
    bool generateModuleGraph{true};
    bool generateIncludeGraph{true};
    bool generateInheritanceGraph{true};
};

class DocGenerator {
public:
    explicit DocGenerator(const DocConfig& config);

    bool generate();
    bool generateArchitecture();
    bool generateModules();
    bool generateNamespaces();
    bool generateClasses();
    bool generateInterfaces();
    bool generatePluginAPI();
    bool generateWorkflowAPI();
    bool generateRuntimeAPI();
    bool generateTransportAPI();
    bool generateVendorAPI();

    bool generateDependencyGraph();
    bool generateIncludeGraph();
    bool generateInheritanceGraph();

    bool generateMarkdown(const std::string& title, const std::string& content);
    bool generateJson(const std::string& filename, const std::string& content);
    bool generateDotFile(const std::string& filename, const std::string& content);

    std::string exportToMarkdown();
    std::string exportToJson();
    std::string exportToDot();

    // Public for testing
    std::vector<std::string> scanHeaders(const std::string& path) const;
    DocClass parseHeader(const std::string& filePath) const;
    DocModule createModule(const std::string& name, const std::vector<std::string>& headers);

private:
    DocConfig m_config;
    std::vector<DocModule> m_modules;
    std::vector<DocClass> m_allClasses;

    bool ensureOutputDir() const;
};

} // namespace tools
} // namespace mbootcore
