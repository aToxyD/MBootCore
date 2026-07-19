#include <catch2/catch_test_macros.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>

#include "../../tools/PluginWizard.hpp"

namespace tools = mbootcore::tools;
namespace fs = std::filesystem;

static std::string findTemplatesDir() {
#ifdef MBOOTCORE_SOURCE_DIR
    auto candidate = std::string(MBOOTCORE_SOURCE_DIR) + "/templates/plugins";
    if (fs::exists(candidate)) return candidate;
#endif
    auto p = fs::current_path();
    for (int i = 0; i < 5; ++i) {
        auto candidate = (p / "templates/plugins").string();
        if (fs::exists(candidate)) return candidate;
        candidate = (p / "../templates/plugins").string();
        if (fs::exists(candidate)) return candidate;
        p = p.parent_path();
    }
    return "";
}

static tools::PluginWizard createWizard() {
    tools::PluginWizard wizard;
    auto tplDir = findTemplatesDir();
    if (!tplDir.empty()) {
        wizard.setTemplatesPath(tplDir);
    }
    return wizard;
}

TEST_CASE("PluginWizardTest", "[sdk]") {
    SECTION("testAvailableTemplates") {
        auto wizard = createWizard();
        auto templates = wizard.availableTemplates();
        REQUIRE(!templates.empty());

        for (const auto& t : templates) {
            REQUIRE(!t.name.empty());
            REQUIRE(!t.description.empty());
        }
    }

    SECTION("testCreateVendorPlugin") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-vendor";

        bool ok = wizard.createVendorPlugin("TestVendor", outputDir);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }

    SECTION("testCreateProtocolPlugin") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-proto";

        bool ok = wizard.createProtocolPlugin("TestProto", "Sahara", outputDir);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }

    SECTION("testCreateWorkflowPlugin") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-workflow";

        bool ok = wizard.createWorkflowPlugin("TestWorkflow", outputDir);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }

    SECTION("testCreateJobPlugin") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-job";

        bool ok = wizard.createJobPlugin("TestJob", outputDir);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }

    SECTION("testCreateTransportPlugin") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-transport";

        bool ok = wizard.createTransportPlugin("TestTransport", outputDir);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }

    SECTION("testSubstituteVariables") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-subst";

        std::unordered_map<std::string, std::string> vars;
        vars["VENDOR_NAME"] = "SubstVendor";
        vars["VENDOR_DISPLAY"] = "Subst Vendor Inc";
        vars["VENDOR_VERSION"] = "2.0.0";

        bool ok = wizard.createPlugin("vendor", "SubstVendor", outputDir, vars);
        REQUIRE(ok);

        REQUIRE(fs::exists(outputDir));
        REQUIRE((fs::exists(outputDir + "/main.cpp.in") || fs::exists(outputDir + "/CMakeLists.txt.in")));

        if (fs::exists(outputDir + "/main.cpp.in")) {
            std::ifstream ifs(outputDir + "/main.cpp.in");
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            REQUIRE(content.find("SubstVendor") != std::string::npos);
        }

        fs::remove_all(outputDir);
    }

    SECTION("testGenerateProjectFile") {
        auto wizard = createWizard();
        std::unordered_map<std::string, std::string> vars;
        vars["PLUGIN_TYPE"] = "vendor";
        vars["PLUGIN_NAME"] = "TestVendor";

        auto cmake = wizard.generateProjectFile("TestVendor", vars);
        REQUIRE(!cmake.empty());
        REQUIRE(cmake.find("TestVendor") != std::string::npos);
        REQUIRE(cmake.find("find_package") != std::string::npos);
        REQUIRE(cmake.find("add_library") != std::string::npos);
    }

    SECTION("testOutputDirectoryCreated") {
        auto wizard = createWizard();
        std::string outputDir = fs::temp_directory_path().string() + "/mboot-wiz-test-newdir";

        REQUIRE(!fs::exists(outputDir));

        bool ok = wizard.createVendorPlugin("NewDirTest", outputDir);
        REQUIRE(ok);
        REQUIRE(fs::exists(outputDir));

        fs::remove_all(outputDir);
    }
}
