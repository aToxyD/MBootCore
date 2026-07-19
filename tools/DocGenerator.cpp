#include "DocGenerator.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <map>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace mbootcore {
namespace tools {

DocGenerator::DocGenerator(const DocConfig& config)
    : m_config(config)
{
}

bool DocGenerator::ensureOutputDir() const
{
    std::error_code ec;
    fs::create_directories(m_config.outputDir, ec);
    return !ec;
}

std::vector<std::string> DocGenerator::scanHeaders(const std::string& path) const
{
    std::vector<std::string> headers;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(path, ec)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        if (p.extension() != ".hpp" && p.extension() != ".h") continue;
        bool skip = false;
        for (const auto& sd : m_config.skipDirs) {
            if (p.string().find(sd) != std::string::npos) {
                skip = true;
                break;
            }
        }
        if (!skip) headers.push_back(p.string());
    }
    std::sort(headers.begin(), headers.end());
    return headers;
}

DocClass DocGenerator::parseHeader(const std::string& filePath) const
{
    DocClass dc;
    dc.file = filePath;

    std::ifstream ifs(filePath);
    if (!ifs.is_open()) return dc;

    std::string line;
    std::string content;
    while (std::getline(ifs, line)) {
        content += line + '\n';
    }

    std::istringstream stream(content);
    std::string currentNamespace;
    bool inClass = false;
    bool inEnum = false;
    std::string currentClass;
    std::string currentClassNamespace;

    std::regex classRegex(R"(\b(class|struct)\s+(\w+))");
    std::regex enumRegex(R"(\b(enum\s+class|enum)\s+(\w+))");
    std::regex namespaceRegex(R"(namespace\s+(\w+))");
    std::regex methodRegex(R"((virtual\s+)?\w[\w\s*&<>]+\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?(?:=\s*0\s*)?;)");
    std::regex memberRegex(R"((\w[\w\s*&<>]+\s+(\w+))\s*;)");
    std::regex inheritsRegex(R"(class\s+\w+\s*:\s*public\s+(\w+))");
    std::regex enumValueRegex(R"((\w+)\s*(?:=\s*[^,}]+)?)");

    std::string line2;
    std::istringstream lineStream(content);
    while (std::getline(lineStream, line2)) {
        std::smatch m;

        if (std::regex_search(line2, m, namespaceRegex)) {
            currentNamespace = m[1].str();
            continue;
        }

        if (std::regex_search(line2, m, enumRegex)) {
            dc.isEnum = true;
            dc.name = m[2].str();
            dc.m_namespaceName = currentNamespace;
            inEnum = true;
            continue;
        }

        if (inEnum && !dc.enumValues.empty() && line2.find('}') != std::string::npos) {
            inEnum = false;
            continue;
        }

        if (inEnum) {
            std::smatch vm;
            std::string trimmed = line2;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            if (!trimmed.empty() && trimmed.find("//") != 0 && trimmed.find("/*") != 0) {
                if (std::regex_search(trimmed, vm, enumValueRegex)) {
                    dc.enumValues.push_back(vm[1].str());
                }
            }
            continue;
        }

        if (std::regex_search(line2, m, classRegex)) {
            dc.name = m[2].str();
            dc.m_namespaceName = currentNamespace;
            currentClass = dc.name;
            currentClassNamespace = currentNamespace;
            dc.isInterface = (line2.find("= 0") != std::string::npos);

            std::smatch im;
            if (std::regex_search(line2, im, inheritsRegex)) {
                dc.baseClasses.push_back(im[1].str());
            }

            inClass = true;
            continue;
        }

        if (inClass) {
            if (line2.find('}') != std::string::npos &&
                line2.find(currentClass) != std::string::npos) {
                inClass = false;
                continue;
            }
            if (line2.find('{') != std::string::npos ||
                line2.find('}') != std::string::npos) {
                continue;
            }

            std::smatch mm;
            if (std::regex_search(line2, mm, methodRegex)) {
                dc.methods.push_back(mm[2].str());
                if (line2.find("= 0") != std::string::npos) {
                    dc.isInterface = true;
                }
            } else if (std::regex_search(line2, mm, memberRegex)) {
                std::string member = mm[2].str();
                if (member != "return" && member != "if" && member != "while" &&
                    member != "for" && member != "switch" && member != "class" &&
                    member != "struct" && member != "enum" && member != "namespace" &&
                    member != "private" && member != "public" && member != "protected") {
                    dc.members.push_back(member);
                }
            }
        }
    }

    if (!dc.name.empty() && dc.brief.empty()) {
        dc.brief = dc.name + " class documentation";
    }

    return dc;
}

DocModule DocGenerator::createModule(const std::string& name,
                                      const std::vector<std::string>& headers)
{
    DocModule mod;
    mod.name = name;
    mod.headers = headers;

    for (const auto& h : headers) {
        auto dc = parseHeader(h);
        mod.classes.push_back(dc);
    }

    return mod;
}

bool DocGenerator::generate()
{
    if (!ensureOutputDir()) {
        std::cerr << "Failed to create output directory: " << m_config.outputDir << "\n";
        return false;
    }

    m_allClasses.clear();
    m_modules.clear();

    for (const auto& path : m_config.headerPaths) {
        auto headers = scanHeaders(path);
        for (const auto& h : headers) {
            m_allClasses.push_back(parseHeader(h));
        }

        std::string modName = fs::path(path).filename().string();
        if (modName.empty()) modName = "root";
        m_modules.push_back(createModule(modName, headers));
    }

    bool ok = true;
    if (m_config.generateMarkdown) {
        ok = generateArchitecture() && ok;
        ok = generateModules() && ok;
        ok = generateClasses() && ok;
        ok = generateInterfaces() && ok;
        ok = generatePluginAPI() && ok;
        ok = generateWorkflowAPI() && ok;
        ok = generateRuntimeAPI() && ok;
        ok = generateTransportAPI() && ok;
        ok = generateVendorAPI() && ok;
    }

    if (m_config.generateModuleGraph) {
        ok = generateDependencyGraph() && ok;
    }
    if (m_config.generateIncludeGraph) {
        ok = generateIncludeGraph() && ok;
    }
    if (m_config.generateInheritanceGraph) {
        ok = generateInheritanceGraph() && ok;
    }

    return ok;
}

bool DocGenerator::generateArchitecture()
{
    std::ostringstream ss;
    ss << "# MBootCore Architecture Overview\n\n";
    ss << "## Layered Architecture\n\n";
    ss << "MBootCore follows Clean Architecture with the following layers:\n\n";
    ss << "1. **Domain** — Interfaces + Types (no platform dependencies)\n";
    ss << "2. **Core** — State machine utilities\n";
    ss << "3. **Protocols** — Self-contained per protocol (Sahara, Firehose)\n";
    ss << "4. **Transport** — USB/Serial/TCP implementations\n";
    ss << "5. **Device** — DefaultDevice (implements IDevice)\n";
    ss << "6. **Loader** — LoaderManager + ProgrammerLoader + LoaderFramework\n";
    ss << "7. **ELF** — ElfParser, ElfValidator, MemoryImageBuilder\n";
    ss << "8. **Pipeline** — BootPipeline orchestrator\n";
    ss << "9. **GPT** — GPT partition management\n";
    ss << "10. **Application** — Session (public API facade)\n";
    ss << "11. **Discovery** — Device detection and protocol negotiation\n";
    ss << "12. **Logging** — Console/File/Null loggers\n";
    ss << "13. **Plugin** — Plugin system infrastructure\n";
    ss << "14. **Firmware** — Firmware package management\n";
    ss << "15. **Job** — Job engine for async operations\n";
    ss << "16. **Workflow** — Workflow engine for complex operations\n";
    ss << "17. **Runtime** — Runtime environment\n";
    ss << "18. **Vendor** — Vendor-specific extensions\n\n";

    ss << "## Key Design Decisions\n\n";
    ss << "- Each protocol owns its serializer/parser\n";
    ss << "- Core has NO protocol knowledge (no layer violations)\n";
    ss << "- IProtocol does NOT depend on ILogger\n";
    ss << "- IDevice does NOT expose transport()\n";
    ss << "- Session accepts dependencies via setter methods\n";
    ss << "- Result<T> over exceptions for predictable control flow\n";
    ss << "- Explicit state machines with transition tables\n";
    ss << "- Adapters are the only bridge between generic layer and protocols\n";
    ss << "- Registry-based design for discovery\n";
    ss << "- Confidence scoring for protocol negotiation\n\n";

    ss << "## Number of Classes by Layer\n\n";
    ss << "| Layer | Headers | Classes |\n";
    ss << "|-------|---------|--------|\n";
    for (const auto& mod : m_modules) {
        ss << "| " << mod.name << " | " << mod.headers.size() << " | "
           << mod.classes.size() << " |\n";
    }
    ss << "\n";

    return generateMarkdown("Architecture Overview", ss.str());
}

bool DocGenerator::generateModules()
{
    std::ostringstream ss;
    ss << "# Module Reference\n\n";

    for (const auto& mod : m_modules) {
        ss << "## " << mod.name << "\n\n";
        ss << "Headers: " << mod.headers.size() << "\n\n";
        ss << "### Classes\n\n";
        for (const auto& cls : mod.classes) {
            ss << "- " << cls.name;
            if (cls.isInterface) ss << " (interface)";
            if (cls.isEnum) ss << " (enum)";
            ss << "\n";
        }
        ss << "\n### Dependencies\n\n";
        if (!mod.dependencies.empty()) {
            for (const auto& dep : mod.dependencies) {
                ss << "- " << dep << "\n";
            }
        } else {
            ss << "None specified.\n";
        }
        ss << "\n";
    }

    return generateMarkdown("Module Reference", ss.str());
}

bool DocGenerator::generateNamespaces()
{
    std::ostringstream ss;
    ss << "# Namespace Reference\n\n";

    std::map<std::string, std::vector<DocClass>> nsMap;
    for (const auto& cls : m_allClasses) {
        nsMap[cls.m_namespaceName].push_back(cls);
    }

    for (const auto& [ns, classes] : nsMap) {
        ss << "## " << (ns.empty() ? "(global)" : ns) << "\n\n";
        for (const auto& cls : classes) {
            ss << "- " << cls.name;
            if (cls.isInterface) ss << " (interface)";
            if (cls.isEnum) ss << " (enum)";
            ss << "\n";
        }
        ss << "\n";
    }

    return generateMarkdown("Namespace Reference", ss.str());
}

bool DocGenerator::generateClasses()
{
    std::ostringstream ss;
    ss << "# Class Reference\n\n";

    for (const auto& cls : m_allClasses) {
        if (cls.name.empty()) continue;
        if (cls.isEnum) continue;

        ss << "## " << cls.name << "\n\n";
        ss << "**File:** " << cls.file << "\n\n";
        ss << "**Namespace:** " << (cls.m_namespaceName.empty() ? "(global)" : cls.m_namespaceName) << "\n\n";
        ss << "**Brief:** " << cls.brief << "\n\n";

        if (!cls.baseClasses.empty()) {
            ss << "**Base Classes:** ";
            for (size_t i = 0; i < cls.baseClasses.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << cls.baseClasses[i];
            }
            ss << "\n\n";
        }

        if (cls.isInterface) {
            ss << "**Type:** Interface (abstract class)\n\n";
        }

        if (!cls.methods.empty()) {
            ss << "### Methods\n\n";
            for (const auto& m : cls.methods) {
                ss << "- " << m << "\n";
            }
            ss << "\n";
        }

        if (!cls.members.empty()) {
            ss << "### Members\n\n";
            for (const auto& m : cls.members) {
                ss << "- " << m << "\n";
            }
            ss << "\n";
        }

        ss << "---\n\n";
    }

    return generateMarkdown("Class Reference", ss.str());
}

bool DocGenerator::generateInterfaces()
{
    std::ostringstream ss;
    ss << "# Interface Reference\n\n";

    bool hasInterfaces = false;
    for (const auto& cls : m_allClasses) {
        if (!cls.isInterface || cls.name.empty()) continue;
        hasInterfaces = true;

        ss << "## " << cls.name << "\n\n";
        ss << "**File:** " << cls.file << "\n\n";
        ss << "**Namespace:** " << (cls.m_namespaceName.empty() ? "(global)" : cls.m_namespaceName) << "\n\n";

        if (!cls.baseClasses.empty()) {
            ss << "**Extends:** ";
            for (size_t i = 0; i < cls.baseClasses.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << cls.baseClasses[i];
            }
            ss << "\n\n";
        }

        if (!cls.methods.empty()) {
            ss << "### Pure Virtual Methods\n\n";
            for (const auto& m : cls.methods) {
                ss << "- `" << m << "()`\n";
            }
            ss << "\n";
        }

        ss << "---\n\n";
    }

    if (!hasInterfaces) {
        ss << "No interfaces detected.\n";
    }

    return generateMarkdown("Interface Reference", ss.str());
}

bool DocGenerator::generatePluginAPI()
{
    std::ostringstream ss;
    ss << "# Plugin API Reference\n\n";
    ss << "## Overview\n\n";
    ss << "The Plugin API enables third-party extensions to MBootCore.\n\n";
    ss << "## Interfaces\n\n";
    ss << "- **IPlugin** — Base plugin interface\n";
    ss << "- **IProtocolPlugin** — Protocol plugin interface\n";
    ss << "- **IVendorPlugin** — Vendor plugin interface\n\n";
    ss << "## Key Types\n\n";
    ss << "- PluginMetadata — Plugin identity and versioning\n";
    ss << "- PluginContext — Plugin runtime context\n";
    ss << "- PluginState — Plugin lifecycle states\n";
    ss << "- PluginDependency — Plugin dependency descriptor\n\n";

    ss << "## Plugin Manager\n\n";
    ss << "The PluginManager handles loading, unloading, and dependency resolution.\n\n";

    ss << "### Related Headers\n\n";
    ss << "- lib/include/mbootcore/plugin/IPlugin.hpp\n";
    ss << "- lib/include/mbootcore/plugin/IProtocolPlugin.hpp\n";
    ss << "- lib/include/mbootcore/plugin/PluginTypes.hpp\n";
    ss << "- lib/include/mbootcore/plugin/PluginContext.hpp\n";
    ss << "- lib/include/mbootcore/plugin/PluginManager.hpp\n";

    return generateMarkdown("Plugin API Reference", ss.str());
}

bool DocGenerator::generateWorkflowAPI()
{
    std::ostringstream ss;
    ss << "# Workflow API Reference\n\n";
    ss << "## Overview\n\n";
    ss << "The Workflow API provides a framework for defining and executing multi-step operations.\n\n";
    ss << "## Key Components\n\n";
    ss << "- **IWorkflow** — Workflow interface\n";
    ss << "- **IWorkflowStep** — Individual step interface\n";
    ss << "- **WorkflowEngine** — Orchestrates workflow execution\n";
    ss << "- **WorkflowBuilder** — Builds workflows programmatically\n";
    ss << "- **WorkflowExecutor** — Executes workflow steps\n";
    ss << "- **WorkflowFactory** — Creates workflow instances\n";
    ss << "- **WorkflowRecovery** — Handles workflow failures\n";
    ss << "- **WorkflowHistory** — Records workflow execution history\n";
    ss << "- **WorkflowProgressEngine** — Tracks workflow progress\n\n";

    ss << "### Related Headers\n\n";
    ss << "- lib/include/mbootcore/workflow/IWorkflow.hpp\n";
    ss << "- lib/include/mbootcore/workflow/IWorkflowStep.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowEngine.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowBuilder.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowExecutor.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowFactory.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowRecovery.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowHistory.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowProgressEngine.hpp\n";
    ss << "- lib/include/mbootcore/workflow/WorkflowTypes.hpp\n";
    ss << "- lib/include/mbootcore/workflow/VirtualWorkflowRuntime.hpp\n";

    return generateMarkdown("Workflow API Reference", ss.str());
}

bool DocGenerator::generateRuntimeAPI()
{
    std::ostringstream ss;
    ss << "# Runtime API Reference\n\n";
    ss << "## Overview\n\n";
    ss << "The Runtime API provides runtime environment management for MBootCore.\n\n";
    ss << "## Key Components\n\n";
    ss << "- **Runtime** — Main runtime class\n";
    ss << "- **RuntimeBuilder** — Builder pattern for runtime construction\n";
    ss << "- **RuntimeConfig** — Runtime configuration\n";
    ss << "- **RuntimeFactory** — Creates runtime instances\n";
    ss << "- **RuntimeCallbacks** — Runtime event callbacks\n";
    ss << "- **RuntimeEvents** — Runtime event types\n";
    ss << "- **RuntimeObserver** — Runtime observation interface\n";
    ss << "- **RuntimeStatistics** — Runtime statistics tracking\n";
    ss << "- **RuntimeHardware** — Hardware abstraction\n\n";

    ss << "### Related Headers\n\n";
    ss << "- lib/include/mbootcore/runtime/Runtime.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeBuilder.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeConfig.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeFactory.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeCallbacks.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeEvents.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeObserver.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeStatistics.hpp\n";
    ss << "- lib/include/mbootcore/runtime/RuntimeHardware.hpp\n";

    return generateMarkdown("Runtime API Reference", ss.str());
}

bool DocGenerator::generateTransportAPI()
{
    std::ostringstream ss;
    ss << "# Transport API Reference\n\n";
    ss << "## Overview\n\n";
    ss << "The Transport API provides abstracted communication channels.\n\n";
    ss << "## Transport Types\n\n";
    ss << "- **USBTransport** — USB communication\n";
    ss << "- **SerialTransport** — Serial port communication\n";
    ss << "- **TCPTransport** — TCP network communication\n";
    ss << "- **UdpTransport** — UDP network communication\n\n";
    ss << "## Supporting Components\n\n";
    ss << "- **TransportFactory** — Creates transport instances\n";
    ss << "- **TransportManager** — Manages transport lifecycle\n";
    ss << "- **TransportMonitor** — Monitors transport health\n";
    ss << "- **TransportAsync** — Async transport wrapper\n";
    ss << "- **HotplugManager** — Hotplug device detection\n";
    ss << "- **ReconnectPolicy** — Auto-reconnect logic\n";
    ss << "- **RetryPolicy** — Retry logic for operations\n";
    ss << "- **TimeoutPolicy** — Timeout configurations\n";
    ss << "- **BufferPool** — Reusable buffer management\n";
    ss << "- **ByteStream** — Stream abstraction\n";
    ss << "- **PacketBuffer** — Packet buffering\n";
    ss << "- **RingBuffer** — Lock-free ring buffer\n";
    ss << "- **UsbEnumerator** — USB device enumeration\n";
    ss << "- **UsbHotplugMonitor** — USB hotplug events\n";
    ss << "- **SerialEnumerator** — Serial port enumeration\n\n";

    ss << "### Related Headers\n\n";
    ss << "- lib/include/mbootcore/transport/TransportTypes.hpp\n";
    ss << "- lib/include/mbootcore/transport/TransportFactory.hpp\n";
    ss << "- lib/include/mbootcore/transport/TransportManager.hpp\n";
    ss << "- lib/include/mbootcore/transport/TransportMonitor.hpp\n";
    ss << "- lib/include/mbootcore/transport/TransportAsync.hpp\n";
    ss << "- lib/include/mbootcore/transport/HotplugManager.hpp\n";
    ss << "- lib/include/mbootcore/transport/ReconnectPolicy.hpp\n";
    ss << "- lib/include/mbootcore/transport/RetryPolicy.hpp\n";
    ss << "- lib/include/mbootcore/transport/TimeoutPolicy.hpp\n";
    ss << "- lib/include/mbootcore/transport/BufferPool.hpp\n";
    ss << "- lib/include/mbootcore/transport/ByteStream.hpp\n";
    ss << "- lib/include/mbootcore/transport/PacketBuffer.hpp\n";
    ss << "- lib/include/mbootcore/transport/RingBuffer.hpp\n";
    ss << "- lib/include/mbootcore/transport/USBTransport.hpp\n";
    ss << "- lib/include/mbootcore/transport/SerialTransport.hpp\n";
    ss << "- lib/include/mbootcore/transport/TCPTransport.hpp\n";
    ss << "- lib/include/mbootcore/transport/UdpTransport.hpp\n";
    ss << "- lib/include/mbootcore/transport/UsbEnumerator.hpp\n";
    ss << "- lib/include/mbootcore/transport/UsbHotplugMonitor.hpp\n";
    ss << "- lib/include/mbootcore/transport/SerialEnumerator.hpp\n";

    return generateMarkdown("Transport API Reference", ss.str());
}

bool DocGenerator::generateVendorAPI()
{
    std::ostringstream ss;
    ss << "# Vendor API Reference\n\n";
    ss << "## Overview\n\n";
    ss << "The Vendor API enables vendor-specific extensions and customizations.\n\n";
    ss << "## Key Components\n\n";
    ss << "- **IVendor** — Vendor interface\n";
    ss << "- **IVendorPlugin** — Vendor plugin interface\n";
    ss << "- **VendorRegistry** — Vendor registration\n";
    ss << "- **VendorFactory** — Vendor instance factory\n";
    ss << "- **VendorContext** — Vendor runtime context\n";
    ss << "- **VendorSession** — Vendor session management\n";
    ss << "- **VendorPipeline** — Vendor pipeline customization\n";
    ss << "- **VendorRuntime** — Vendor runtime environment\n";
    ss << "- **VendorMonitor** — Vendor device monitoring\n";
    ss << "- **VendorEvents** — Vendor event handling\n";
    ss << "- **CapabilityResolver** — Capability resolution\n";
    ss << "- **VendorPackageResolver** — Package resolution\n";
    ss << "- **VendorTypes** — Vendor type definitions\n\n";

    ss << "### Related Headers\n\n";
    ss << "- lib/include/mbootcore/vendor/IVendor.hpp\n";
    ss << "- lib/include/mbootcore/vendor/IVendorPlugin.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorRegistry.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorFactory.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorContext.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorSession.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorPipeline.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorRuntime.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorMonitor.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorEvents.hpp\n";
    ss << "- lib/include/mbootcore/vendor/CapabilityResolver.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorPackageResolver.hpp\n";
    ss << "- lib/include/mbootcore/vendor/VendorTypes.hpp\n";

    return generateMarkdown("Vendor API Reference", ss.str());
}

bool DocGenerator::generateDependencyGraph()
{
    std::ostringstream ss;
    ss << "digraph Dependencies {\n";
    ss << "    rankdir=LR;\n";
    ss << "    node [shape=box, style=filled, fillcolor=lightyellow];\n";
    ss << "    edge [color=gray50];\n\n";

    ss << "    subgraph cluster_legend {\n";
    ss << "        label=\"Legend\";\n";
    ss << "        style=dashed;\n";
    ss << "        domain [label=\"Domain\", fillcolor=lightblue];\n";
    ss << "        core [label=\"Core\", fillcolor=lightgreen];\n";
    ss << "        protocol [label=\"Protocol\", fillcolor=lightsalmon];\n";
    ss << "        transport [label=\"Transport\", fillcolor=lightcyan];\n";
    ss << "        application [label=\"Application\", fillcolor=plum];\n";
    ss << "    }\n\n";

    for (const auto& mod : m_modules) {
        std::string safe = mod.name;
        std::replace(safe.begin(), safe.end(), '-', '_');
        std::replace(safe.begin(), safe.end(), '/', '_');
        ss << "    \"" << safe << "\" [label=\"" << mod.name
           << "\\n(" << mod.classes.size() << " classes)\"];\n";
    }

    ss << "\n    // Domain layer dependencies\n";
    for (size_t i = 1; i < m_modules.size(); ++i) {
        ss << "    \"" << m_modules[i].name << "\" -> \"domain\";\n";
    }

    ss << "}\n";

    return generateDotFile("dependencies", ss.str());
}

bool DocGenerator::generateIncludeGraph()
{
    std::ostringstream ss;
    ss << "digraph Includes {\n";
    ss << "    rankdir=LR;\n";
    ss << "    node [shape=folder, style=filled, fillcolor=lightyellow];\n";
    ss << "    edge [color=blue, arrowhead=vee];\n\n";

    std::set<std::string> seen;
    for (const auto& cls : m_allClasses) {
        if (cls.file.empty() || cls.name.empty()) continue;
        if (seen.count(cls.name)) continue;
        seen.insert(cls.name);

        std::string safeFile = cls.file;
        std::replace(safeFile.begin(), safeFile.end(), '\\', '/');
        size_t pos = safeFile.find("mbootcore/");
        std::string label = (pos != std::string::npos) ? safeFile.substr(pos) : safeFile;

        ss << "    \"" << cls.name << "\" [label=\"" << label << "\"];\n";
    }

    ss << "}\n";

    return generateDotFile("includes", ss.str());
}

bool DocGenerator::generateInheritanceGraph()
{
    std::ostringstream ss;
    ss << "digraph Inheritance {\n";
    ss << "    rankdir=BT;\n";
    ss << "    node [shape=record, style=filled, fillcolor=lightyellow];\n";
    ss << "    edge [color=red, arrowhead=empty, style=dashed];\n\n";

    for (const auto& cls : m_allClasses) {
        if (cls.name.empty()) continue;
        std::string color = cls.isInterface ? "lightblue" : "lightyellow";
        ss << "    \"" << cls.name << "\" [fillcolor=" << color << "];\n";
    }

    ss << "\n";
    for (const auto& cls : m_allClasses) {
        for (const auto& base : cls.baseClasses) {
            ss << "    \"" << cls.name << "\" -> \"" << base << "\";\n";
        }
    }

    ss << "}\n";

    return generateDotFile("inheritance", ss.str());
}

bool DocGenerator::generateMarkdown(const std::string& title, const std::string& content)
{
    if (!ensureOutputDir()) return false;
    std::string filename = title;
    std::replace(filename.begin(), filename.end(), ' ', '_');
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    filename += ".md";

    fs::path outPath = fs::path(m_config.outputDir) / filename;
    std::ofstream ofs(outPath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to write: " << outPath << "\n";
        return false;
    }
    ofs << content;
    std::cout << "Generated: " << outPath << "\n";
    return true;
}

bool DocGenerator::generateJson(const std::string& filename, const std::string& content)
{
    if (!ensureOutputDir()) return false;
    fs::path outPath = fs::path(m_config.outputDir) / filename;
    std::ofstream ofs(outPath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to write: " << outPath << "\n";
        return false;
    }
    ofs << content;
    std::cout << "Generated: " << outPath << "\n";
    return true;
}

bool DocGenerator::generateDotFile(const std::string& filename, const std::string& content)
{
    if (!ensureOutputDir()) return false;
    std::string fname = filename;
    if (fname.find(".gv") == std::string::npos && fname.find(".dot") == std::string::npos) {
        fname += ".gv";
    }
    fs::path outPath = fs::path(m_config.outputDir) / fname;
    std::ofstream ofs(outPath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to write: " << outPath << "\n";
        return false;
    }
    ofs << content;
    std::cout << "Generated: " << outPath << "\n";
    return true;
}

std::string DocGenerator::exportToMarkdown()
{
    std::ostringstream ss;
    ss << "# MBootCore API Reference\n\n";
    ss << "Auto-generated documentation.\n\n";
    ss << "## Contents\n\n";

    for (const auto& cls : m_allClasses) {
        if (cls.name.empty()) continue;
        ss << "- " << cls.name;
        if (cls.isInterface) ss << " (interface)";
        ss << "\n";
    }

    ss << "\n## Details\n\n";
    for (const auto& cls : m_allClasses) {
        if (cls.name.empty()) continue;
        ss << "### " << cls.name << "\n\n";
        ss << "- File: " << cls.file << "\n";
        ss << "- Namespace: " << (cls.m_namespaceName.empty() ? "(global)" : cls.m_namespaceName) << "\n";
        ss << "- Brief: " << cls.brief << "\n";
        if (!cls.baseClasses.empty()) {
            ss << "- Base Classes: ";
            for (size_t i = 0; i < cls.baseClasses.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << cls.baseClasses[i];
            }
            ss << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}

std::string DocGenerator::exportToJson()
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"generator\": \"MBootCore DocGenerator\",\n";
    ss << "  \"classes\": [\n";

    for (size_t i = 0; i < m_allClasses.size(); ++i) {
        const auto& cls = m_allClasses[i];
        if (cls.name.empty()) continue;
        ss << "    {\n";
        ss << "      \"name\": \"" << cls.name << "\",\n";
        ss << "      \"namespace\": \"" << cls.m_namespaceName << "\",\n";
        ss << "      \"file\": \"" << cls.file << "\",\n";
        ss << "      \"brief\": \"" << cls.brief << "\",\n";
        ss << "      \"isInterface\": " << (cls.isInterface ? "true" : "false") << ",\n";
        ss << "      \"isEnum\": " << (cls.isEnum ? "true" : "false") << ",\n";
        ss << "      \"methodCount\": " << cls.methods.size() << ",\n";
        ss << "      \"memberCount\": " << cls.members.size() << "\n";
        ss << "    }";
        if (i + 1 < m_allClasses.size()) ss << ",";
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

std::string DocGenerator::exportToDot()
{
    std::ostringstream ss;
    ss << "digraph MBootCore {\n";
    ss << "    rankdir=LR;\n";
    ss << "    node [shape=box];\n\n";

    for (size_t i = 0; i < m_modules.size(); ++i) {
        std::string safe = m_modules[i].name;
        std::replace(safe.begin(), safe.end(), '-', '_');
        ss << "    \"" << safe << "\" [label=\"" << m_modules[i].name
           << "\", style=filled, fillcolor=lightyellow];\n";
    }

    ss << "}\n";
    return ss.str();
}

} // namespace tools
} // namespace mbootcore
