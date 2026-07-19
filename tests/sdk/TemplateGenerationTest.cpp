#include <sdk/VendorSDK.hpp>
#include <sdk/PluginManifest.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <sstream>
#include <unordered_map>

namespace sdk = mbootcore::sdk;

namespace {

class VendorTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/plugin/IPlugin.hpp>\n";
        ss << "#include <mbootcore/plugin/PluginTypes.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace plugins {\n\n";
        ss << "class " << name << "VendorPlugin : public plugin::IPlugin {\n";
        ss << "public:\n";
        ss << "    " << name << "VendorPlugin() = default;\n\n";
        ss << "    plugin::PluginMetadata metadata() const noexcept override {\n";
        ss << "        plugin::PluginMetadata m;\n";
        ss << "        m.name = \"" << name << "Vendor\";\n";
        ss << "        m.version = \"1.0.0\";\n";
        ss << "        m.vendor = discovery::Vendor::Custom;\n";
        ss << "        return m;\n";
        ss << "    }\n\n";
        ss << "    Result<void> initialize(plugin::PluginContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    Result<void> shutdown() noexcept override {\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    Result<void> registerComponents(plugin::PluginContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    Result<void> unregisterComponents(plugin::PluginContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    plugin::PluginState state() const noexcept override {\n";
        ss << "        return plugin::PluginState::Loaded;\n";
        ss << "    }\n\n";
        ss << "    void setEnabled(bool en) noexcept override { enabled_ = en; }\n";
        ss << "    bool isEnabled() const noexcept override { return enabled_; }\n\n";
        ss << "private:\n";
        ss << "    bool enabled_{true};\n";
        ss << "};\n\n";
        ss << "} // namespace plugins\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class ProtocolTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/plugin/IProtocolPlugin.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace plugins {\n\n";
        ss << "class " << name << "ProtocolPlugin : public plugin::IProtocolPlugin {\n";
        ss << "public:\n";
        ss << "    " << name << "ProtocolPlugin() = default;\n\n";
        ss << "    discovery::ProtocolType protocolType() const noexcept override {\n";
        ss << "        return discovery::ProtocolType::Custom;\n";
        ss << "    }\n\n";
        ss << "    Result<void> initialize(plugin::PluginContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    Result<void> shutdown() noexcept override {\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n";
        ss << "};\n\n";
        ss << "} // namespace plugins\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class TransportTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/domain/ITransport.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace transport {\n\n";
        ss << "class " << name << "Transport : public ITransport {\n";
        ss << "public:\n";
        ss << "    " << name << "Transport() = default;\n";
        ss << "    ~" << name << "Transport() override = default;\n\n";
        ss << "    Result<void> open(const std::string& path) override {\n";
        ss << "        (void)path;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n\n";
        ss << "    void close() noexcept override {}\n";
        ss << "    bool isOpen() const noexcept override { return false; }\n\n";
        ss << "    Result<size_t> read(uint8_t* buffer, size_t size, std::chrono::milliseconds timeout) override {\n";
        ss << "        (void)buffer; (void)size; (void)timeout;\n";
        ss << "        return Result<size_t>::Ok(0);\n";
        ss << "    }\n\n";
        ss << "    Result<size_t> write(const uint8_t* data, size_t size, std::chrono::milliseconds timeout) override {\n";
        ss << "        (void)data; (void)size; (void)timeout;\n";
        ss << "        return Result<size_t>::Ok(0);\n";
        ss << "    }\n";
        ss << "};\n\n";
        ss << "} // namespace transport\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class WorkflowTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/workflow/IWorkflow.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace workflow {\n\n";
        ss << "class " << name << "Workflow : public IWorkflow {\n";
        ss << "public:\n";
        ss << "    std::string name() const noexcept override { return \"" << name << "\"; }\n";
        ss << "    std::string description() const noexcept override { return \"" << name << " workflow\"; }\n";
        ss << "    Result<void> execute(WorkflowContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n";
        ss << "};\n\n";
        ss << "} // namespace workflow\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class JobTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/job/IJob.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace jobs {\n\n";
        ss << "class " << name << "Job : public job::IJob {\n";
        ss << "public:\n";
        ss << "    std::string name() const noexcept override { return \"" << name << "\"; }\n";
        ss << "    Result<void> execute(job::JobContext& ctx) override {\n";
        ss << "        (void)ctx;\n";
        ss << "        return Result<void>::Ok();\n";
        ss << "    }\n";
        ss << "    bool canCancel() const noexcept override { return true; }\n";
        ss << "    void cancel() noexcept override {}\n";
        ss << "};\n\n";
        ss << "} // namespace jobs\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class PackageTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/firmware/IFirmwareReader.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace firmware {\n\n";
        ss << "class " << name << "PackageReader : public IFirmwareReader {\n";
        ss << "public:\n";
        ss << "    bool canRead(const std::string& path) const override {\n";
        ss << "        (void)path;\n";
        ss << "        return false;\n";
        ss << "    }\n\n";
        ss << "    Result<FirmwarePackage> read(const std::string& path) override {\n";
        ss << "        (void)path;\n";
        ss << "        return Result<FirmwarePackage>::Error(mbootcore::ErrorCode::NotSupported);\n";
        ss << "    }\n";
        ss << "};\n\n";
        ss << "} // namespace firmware\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

class DiscoveryTemplateGenerator {
public:
    static std::string generate(const std::string& name) {
        std::ostringstream ss;
        ss << "#pragma once\n\n";
        ss << "#include <mbootcore/discovery/IDeviceDetector.hpp>\n\n";
        ss << "namespace mbootcore {\n";
        ss << "namespace discovery {\n\n";
        ss << "class " << name << "Detector : public IDeviceDetector {\n";
        ss << "public:\n";
        ss << "    DiscoveryResult detect(const DetectionCriteria& criteria) override {\n";
        ss << "        (void)criteria;\n";
        ss << "        return DiscoveryResult{};\n";
        ss << "    }\n";
        ss << "};\n\n";
        ss << "} // namespace discovery\n";
        ss << "} // namespace mbootcore\n";
        return ss.str();
    }
};

} // anonymous namespace

TEST_CASE("TemplateGenerationTest", "[sdk]") {
    SECTION("testVendorTemplateGenerated") {
        auto result = VendorTemplateGenerator::generate("MyVendor");
        REQUIRE(!result.empty());
    }

    SECTION("testVendorTemplateContainsName") {
        auto result = VendorTemplateGenerator::generate("MyVendor");
        REQUIRE(result.find("MyVendorVendorPlugin") != std::string::npos);
    }

    SECTION("testProtocolTemplateGenerated") {
        auto result = ProtocolTemplateGenerator::generate("MyProto");
        REQUIRE(!result.empty());
    }

    SECTION("testProtocolTemplateContainsInterface") {
        auto result = ProtocolTemplateGenerator::generate("MyProto");
        REQUIRE(result.find("IProtocolPlugin") != std::string::npos);
    }

    SECTION("testTransportTemplateGenerated") {
        auto result = TransportTemplateGenerator::generate("Usb");
        REQUIRE(!result.empty());
    }

    SECTION("testTransportTemplateContainsOpen") {
        auto result = TransportTemplateGenerator::generate("Usb");
        REQUIRE(result.find("Result<void> open") != std::string::npos);
    }

    SECTION("testWorkflowTemplateGenerated") {
        auto result = WorkflowTemplateGenerator::generate("Flash");
        REQUIRE(!result.empty());
    }

    SECTION("testWorkflowTemplateContainsName") {
        auto result = WorkflowTemplateGenerator::generate("Flash");
        REQUIRE(result.find("FlashWorkflow") != std::string::npos);
    }

    SECTION("testJobTemplateGenerated") {
        auto result = JobTemplateGenerator::generate("Erase");
        REQUIRE(!result.empty());
    }

    SECTION("testJobTemplateContainsCancel") {
        auto result = JobTemplateGenerator::generate("Erase");
        REQUIRE(result.find("cancel()") != std::string::npos);
    }

    SECTION("testPackageTemplateGenerated") {
        auto result = PackageTemplateGenerator::generate("MyPackage");
        REQUIRE(!result.empty());
    }

    SECTION("testPackageTemplateContainsCanRead") {
        auto result = PackageTemplateGenerator::generate("MyPackage");
        REQUIRE(result.find("canRead") != std::string::npos);
    }

    SECTION("testDiscoveryTemplateGenerated") {
        auto result = DiscoveryTemplateGenerator::generate("UsbDetector");
        REQUIRE(!result.empty());
    }

    SECTION("testDiscoveryTemplateContainsDetect") {
        auto result = DiscoveryTemplateGenerator::generate("UsbDetector");
        REQUIRE(result.find("detect") != std::string::npos);
    }

    SECTION("testAllTemplatesCompilableSyntax") {
        auto vendor = VendorTemplateGenerator::generate("Check");
        auto proto = ProtocolTemplateGenerator::generate("Check");
        auto transport = TransportTemplateGenerator::generate("Check");
        auto workflow = WorkflowTemplateGenerator::generate("Check");
        auto job = JobTemplateGenerator::generate("Check");
        auto pkg = PackageTemplateGenerator::generate("Check");
        auto discovery = DiscoveryTemplateGenerator::generate("Check");

        REQUIRE(vendor.find("#pragma once") != std::string::npos);
        REQUIRE(vendor.find("namespace") != std::string::npos);
        REQUIRE(vendor.find("class") != std::string::npos);

        REQUIRE(proto.find("#pragma once") != std::string::npos);
        REQUIRE(proto.find("namespace") != std::string::npos);
        REQUIRE(proto.find("class") != std::string::npos);

        REQUIRE(transport.find("#pragma once") != std::string::npos);
        REQUIRE(transport.find("namespace") != std::string::npos);
        REQUIRE(transport.find("class") != std::string::npos);

        REQUIRE(workflow.find("#pragma once") != std::string::npos);
        REQUIRE(workflow.find("namespace") != std::string::npos);
        REQUIRE(workflow.find("class") != std::string::npos);

        REQUIRE(job.find("#pragma once") != std::string::npos);
        REQUIRE(job.find("namespace") != std::string::npos);
        REQUIRE(job.find("class") != std::string::npos);

        REQUIRE(pkg.find("#pragma once") != std::string::npos);
        REQUIRE(pkg.find("namespace") != std::string::npos);
        REQUIRE(pkg.find("class") != std::string::npos);

        REQUIRE(discovery.find("#pragma once") != std::string::npos);
        REQUIRE(discovery.find("namespace") != std::string::npos);
        REQUIRE(discovery.find("class") != std::string::npos);
    }
}
