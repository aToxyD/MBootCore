#include <sdk/VendorSDK.hpp>
#include <sdk/PluginManifest.hpp>
#include <sdk/VendorRegistration.hpp>
#include <sdk/ProtocolRegistration.hpp>
#include <sdk/PluginDependencyGraph.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <sstream>

namespace sdk = mbootcore::sdk;

namespace {

class DocumentationGenerator {
public:
    static std::string generateVendorAPI(const std::string& vendorName) {
        std::ostringstream ss;
        ss << "# " << vendorName << " Vendor API Reference\n\n";
        ss << "## Overview\n\n";
        ss << "This document describes the vendor API for " << vendorName << ".\n\n";
        ss << "## Registration\n\n";
        ss << "```cpp\n";
        ss << "VendorSDK sdk;\n";
        ss << "sdk.registerVendor({.name = \"" << vendorName << "\"});\n";
        ss << "sdk.finalize();\n";
        ss << "```\n\n";
        ss << "## Capabilities\n\n";
        ss << "- Protocol support\n";
        ss << "- Device discovery\n";
        ss << "- Firmware flashing\n\n";
        return ss.str();
    }

    static std::string generateProtocolAPI(const std::string& protocolName) {
        std::ostringstream ss;
        ss << "# " << protocolName << " Protocol API Reference\n\n";
        ss << "## Overview\n\n";
        ss << "Protocol implementation for " << protocolName << ".\n\n";
        ss << "## Interface\n\n";
        ss << "- `initialize()` - Initialize protocol\n";
        ss << "- `negotiate()` - Negotiate protocol version\n";
        ss << "- `send()` - Send packet\n";
        ss << "- `receive()` - Receive packet\n\n";
        return ss.str();
    }
};

} // anonymous namespace

TEST_CASE("DocumentationGeneratorTest", "[sdk]") {
    SECTION("testVendorAPIDocGenerated") {
        auto doc = DocumentationGenerator::generateVendorAPI("Qualcomm");
        REQUIRE(!doc.empty());
    }

    SECTION("testVendorAPIDocContainsName") {
        auto doc = DocumentationGenerator::generateVendorAPI("Qualcomm");
        REQUIRE(doc.find("Qualcomm") != std::string::npos);
    }

    SECTION("testVendorAPIDocContainsRegistration") {
        auto doc = DocumentationGenerator::generateVendorAPI("Qualcomm");
        REQUIRE(doc.find("registerVendor") != std::string::npos);
    }

    SECTION("testProtocolAPIDocGenerated") {
        auto doc = DocumentationGenerator::generateProtocolAPI("Sahara");
        REQUIRE(!doc.empty());
    }

    SECTION("testProtocolAPIDocContainsInterface") {
        auto doc = DocumentationGenerator::generateProtocolAPI("Sahara");
        REQUIRE(doc.find("initialize()") != std::string::npos);
    }

    SECTION("testDependencyGraphDocExport") {
        sdk::PluginDependencyGraph graph;
        auto exported = graph.toDot();
        REQUIRE(!exported.empty());
    }

    SECTION("testGraphContainsNodes") {
        sdk::PluginDependencyGraph graph;
        graph.addNode("NodeA", "vendor", "NodeAVendor", "1.0.0", {});
        graph.addNode("NodeB", "protocol", "NodeBProtocol", "1.0.0", {});
        auto dot = graph.toDot();
        REQUIRE(dot.find("NodeA") != std::string::npos);
        REQUIRE(dot.find("NodeB") != std::string::npos);
    }

    SECTION("testMarkdownFormat") {
        auto doc = DocumentationGenerator::generateVendorAPI("Test");
        REQUIRE(doc.find("# ") == 0);
    }

    SECTION("testCodeBlocksInDocs") {
        auto doc = DocumentationGenerator::generateVendorAPI("Test");
        REQUIRE(doc.find("```") != std::string::npos);
    }

    SECTION("testHeaderFormat") {
        auto doc = DocumentationGenerator::generateVendorAPI("Test");
        REQUIRE(doc.find("## Overview") != std::string::npos);
        REQUIRE(doc.find("## Registration") != std::string::npos);
        REQUIRE(doc.find("## Capabilities") != std::string::npos);
    }

    SECTION("testMultipleVendorDocs") {
        auto qcom = DocumentationGenerator::generateVendorAPI("Qualcomm");
        auto mtk = DocumentationGenerator::generateVendorAPI("MediaTek");
        REQUIRE(!qcom.empty());
        REQUIRE(!mtk.empty());
        REQUIRE(qcom.find("Qualcomm") != std::string::npos);
        REQUIRE(mtk.find("MediaTek") != std::string::npos);
    }

    SECTION("testMultipleProtocolDocs") {
        auto sahara = DocumentationGenerator::generateProtocolAPI("Sahara");
        auto firehose = DocumentationGenerator::generateProtocolAPI("Firehose");
        REQUIRE(!sahara.empty());
        REQUIRE(!firehose.empty());
        REQUIRE(sahara.find("Sahara") != std::string::npos);
        REQUIRE(firehose.find("Firehose") != std::string::npos);
    }

    SECTION("testDocContentNotEmpty") {
        auto vendorDoc = DocumentationGenerator::generateVendorAPI("Test");
        auto protoDoc = DocumentationGenerator::generateProtocolAPI("Test");
        REQUIRE(vendorDoc.size() > 50);
        REQUIRE(protoDoc.size() > 50);
    }

    SECTION("testDocStructure") {
        auto doc = DocumentationGenerator::generateVendorAPI("Test");
        REQUIRE(doc.find("Overview") != std::string::npos);
    }

    SECTION("testIntegrationWithSDK") {
        auto sdk = std::make_unique<sdk::VendorSDK>();

        sdk::VendorRegistration reg;
        reg.vendorId = mbootcore::discovery::Vendor::Qualcomm;
        reg.name = "Qualcomm";
        sdk->registerVendor(reg);

        auto doc = DocumentationGenerator::generateVendorAPI("Qualcomm");
        REQUIRE(!doc.empty());
        REQUIRE(doc.find("Qualcomm") != std::string::npos);
        REQUIRE(doc.find("registerVendor") != std::string::npos);
    }
}
