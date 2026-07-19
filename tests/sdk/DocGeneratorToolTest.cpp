#include <catch2/catch_test_macros.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

#include "../../tools/DocGenerator.hpp"

namespace tools = mbootcore::tools;
namespace fs = std::filesystem;

static std::string findDomainPath() {
#ifdef MBOOTCORE_SOURCE_DIR
    auto candidate = std::string(MBOOTCORE_SOURCE_DIR) + "/lib/include/mbootcore/domain";
    if (fs::exists(candidate)) return candidate;
#endif
    auto p = fs::current_path();
    for (int i = 0; i < 10; ++i) {
        auto candidate = (p / "lib/include/mbootcore/domain").string();
        if (fs::exists(candidate)) return candidate;
        p = p.parent_path();
    }
    return "";
}

TEST_CASE("DocGeneratorToolTest", "[sdk]") {
    SECTION("testDefaultConfig") {
        tools::DocConfig cfg;
        REQUIRE(cfg.outputDir == "./docs/generated");
        REQUIRE(cfg.generateMarkdown);
        REQUIRE(!cfg.generateHtml);
        REQUIRE(!cfg.generateJson);
        REQUIRE(!cfg.generateDot);
        REQUIRE(cfg.generateModuleGraph);
        REQUIRE(cfg.generateIncludeGraph);
        REQUIRE(cfg.generateInheritanceGraph);
    }

    SECTION("testParseHeader") {
        std::string domainPath;
#ifdef MBOOTCORE_SOURCE_DIR
        domainPath = std::string(MBOOTCORE_SOURCE_DIR) + "/lib/include/mbootcore/domain";
        if (!fs::exists(domainPath)) domainPath.clear();
#endif
        if (domainPath.empty()) {
            auto srcDir = fs::current_path();
            while (!fs::exists(srcDir / "lib/include/mbootcore/domain")) {
                srcDir = srcDir.parent_path();
                if (srcDir.root_path() == srcDir) break;
            }
            domainPath = (srcDir / "lib/include/mbootcore/domain").string();
        }

        tools::DocConfig cfg;
        cfg.headerPaths.push_back(domainPath);
        tools::DocGenerator gen(cfg);

        auto headers = gen.scanHeaders(domainPath);
        REQUIRE(!headers.empty());

        for (const auto& h : headers) {
            auto dc = gen.parseHeader(h);
            if (!dc.name.empty()) {
                REQUIRE(!dc.name.empty());
                REQUIRE(!dc.file.empty());
            }
        }
    }

    SECTION("testCreateModule") {
        auto srcDir = fs::current_path();
        while (!fs::exists(srcDir / "lib/include/mbootcore/domain")) {
            srcDir = srcDir.parent_path();
            if (srcDir.root_path() == srcDir) break;
        }

        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-module";
        cfg.headerPaths.push_back((srcDir / "lib/include/mbootcore/domain").string());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateModules();
        REQUIRE(ok);

        std::string dot = gen.exportToDot();
        REQUIRE(!dot.empty());
    }

    SECTION("testGenerateMarkdown") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test";
        tools::DocGenerator gen(cfg);

        bool ok = gen.generateMarkdown("TestDoc", "# Test\n\nContent");
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "testdoc.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("# Test") != std::string::npos);
    }

    SECTION("testGenerateJson") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-json";
        tools::DocGenerator gen(cfg);

        bool ok = gen.generateJson("test.json", "{\"key\": \"value\"}");
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "test.json";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("key") != std::string::npos);
    }

    SECTION("testGenerateDot") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-dot";
        tools::DocGenerator gen(cfg);

        bool ok = gen.generateDotFile("test", "digraph { a -> b; }");
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "test.gv";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("digraph") != std::string::npos);
    }

    SECTION("testArchitectureGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-arch";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateArchitecture();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "architecture_overview.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("MBootCore Architecture") != std::string::npos);
    }

    SECTION("testModulesGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-modules";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateModules();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "module_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Module Reference") != std::string::npos);
    }

    SECTION("testClassesGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-classes";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateClasses();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "class_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Class Reference") != std::string::npos);
    }

    SECTION("testInterfacesGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-interfaces";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateInterfaces();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "interface_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Interface Reference") != std::string::npos);
    }

    SECTION("testPluginAPIGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-plugin";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generatePluginAPI();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "plugin_api_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Plugin API Reference") != std::string::npos);
        REQUIRE(content.find("IPlugin") != std::string::npos);
    }

    SECTION("testWorkflowAPIGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-workflow";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateWorkflowAPI();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "workflow_api_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Workflow API Reference") != std::string::npos);
    }

    SECTION("testRuntimeAPIGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-runtime";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateRuntimeAPI();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "runtime_api_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Runtime API Reference") != std::string::npos);
    }

    SECTION("testTransportAPIGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-transport";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateTransportAPI();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "transport_api_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Transport API Reference") != std::string::npos);
    }

    SECTION("testVendorAPIGeneration") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-vendor";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateVendorAPI();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "vendor_api_reference.md";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("Vendor API Reference") != std::string::npos);
    }

    SECTION("testDependencyGraph") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-depgraph";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateDependencyGraph();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "dependencies.gv";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("digraph") != std::string::npos);
    }

    SECTION("testIncludeGraph") {
        tools::DocConfig cfg;
        cfg.outputDir = fs::temp_directory_path().string() + "/mboot-doc-test-inclgraph";
        cfg.headerPaths.push_back(findDomainPath());
        tools::DocGenerator gen(cfg);

        gen.generate();
        bool ok = gen.generateIncludeGraph();
        REQUIRE(ok);

        fs::path expected = fs::path(cfg.outputDir) / "includes.gv";
        REQUIRE(fs::exists(expected));

        std::ifstream ifs(expected);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content.find("digraph") != std::string::npos);
    }
}
