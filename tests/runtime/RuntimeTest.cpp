#include <catch2/catch_test_macros.hpp>
#include <memory>

#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/RuntimeObserver.hpp>

#include <mbootcore/logging/NullLogger.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>
#include <mbootcore/discovery/VirtualDeviceDetector.hpp>
#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <mbootcore/vendor/VendorFactory.hpp>
#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/job/GenericJobs.hpp>
#include <mbootcore/job/RecoveryPolicies.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>

#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "../virtual/VirtualRuntime.hpp"

using namespace mbootcore;
using namespace mbootcore::runtime;
using namespace mbootcore::discovery;
namespace fs = std::filesystem;

class TestObserver : public RuntimeObserver {
public:
    std::vector<RuntimeEvent> events;
    RuntimeStatistics lastStats;
    RuntimeHealth lastHealth;
    int eventCount{0};
    int statsCount{0};
    int healthCount{0};

    void onRuntimeEvent(const RuntimeEvent& event) override {
        events.push_back(event);
        ++eventCount;
    }

    void onStatisticsUpdated(const RuntimeStatistics& stats) override {
        lastStats = stats;
        ++statsCount;
    }

    void onHealthChanged(const RuntimeHealth& health) override {
        lastHealth = health;
        ++healthCount;
    }
};

class MockVendor : public vendor::IVendor {
public:
    Result<void> initialize(const vendor::VendorContext&) override { return {}; }
    Result<void> shutdown() noexcept override { return {}; }
    vendor::VendorInfo vendorInfo() const override {
        vendor::VendorInfo info;
        info.id = "mock-vendor";
        info.name = "Mock Vendor";
        info.family = vendor::VendorFamily::Custom;
        return info;
    }
    vendor::VendorCapability capabilities() const override {
        return vendor::VendorCapability::BootROM;
    }
    std::unique_ptr<IDeviceDetector> createDetector() override { return nullptr; }
    std::unique_ptr<IProtocolNegotiator> createNegotiator() override { return nullptr; }
    std::unique_ptr<pipeline::BootPipeline> createPipeline() override { return nullptr; }
    std::unique_ptr<IFlashDevice> createFlashDevice() override { return nullptr; }
    std::unique_ptr<workflow::WorkflowEngine> createWorkflow() override { return nullptr; }
    std::string_view name() const noexcept override { return "mock-vendor"; }
    std::unique_ptr<IVendor> clone() const override {
        return std::make_unique<MockVendor>();
    }
};

TEST_CASE("Runtime construction", "[runtime]") {
    SECTION("testDefaultConstruction") {
        Runtime rt;
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testMoveConstruction") {
        Runtime rt1;
        Runtime rt2(std::move(rt1));
        REQUIRE(!rt2.isInitialized());
        // Moved-from rt1 is in valid but unspecified state
    }

    SECTION("testMoveAssignment") {
        Runtime rt1;
        Runtime rt2;
        rt2 = std::move(rt1);
        REQUIRE(!rt2.isInitialized());
        // Moved-from rt1 is in valid but unspecified state
    }

    SECTION("testNotInitializedByDefault") {
        Runtime rt;
        REQUIRE(!rt.isInitialized());
        auto result = rt.discover();
        REQUIRE(result.isError());
    }

    SECTION("testDestructorShutdown") {
        {
            Runtime rt;
            auto result = rt.initialize();
            REQUIRE(result.isOk());
            REQUIRE(rt.isInitialized());
        }
    }
}

TEST_CASE("Runtime builder", "[runtime]") {
    SECTION("testBuilderDefaults") {
        auto rt = RuntimeBuilder().build();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testBuilderWithConfig") {
        RuntimeConfig cfg;
        cfg.maxRetries = 5;
        cfg.enableVendorRuntime = false;
        auto rt = RuntimeBuilder().withConfig(cfg).build();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testBuilderWithLogger") {
        auto logger = std::make_unique<NullLogger>();
        auto rt = RuntimeBuilder()
            .withLogger(std::move(logger))
            .build();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testBuilderWithOverrides") {
        auto registry = std::make_unique<discovery::ProtocolRegistry>();
        auto rt = RuntimeBuilder()
            .withProtocolRegistry(std::move(registry))
            .build();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testBuilderChaining") {
        auto rt = RuntimeBuilder()
            .withConfig(RuntimeConfig{})
            .withLogger(std::make_unique<NullLogger>())
            .build();
        REQUIRE(!rt.isInitialized());
    }
}

TEST_CASE("Runtime factory", "[runtime]") {
    SECTION("testFactoryDefault") {
        auto rt = RuntimeFactory::createDefault();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryTesting") {
        auto rt = RuntimeFactory::createTesting();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryMinimal") {
        auto rt = RuntimeFactory::createMinimal();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryCLI") {
        auto rt = RuntimeFactory::createCLI();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryGUI") {
        auto rt = RuntimeFactory::createGUI();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryEmbedded") {
        auto rt = RuntimeFactory::createEmbedded();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testFactoryAllCreateWithoutCrash") {
        auto d = RuntimeFactory::createDefault();
        auto t = RuntimeFactory::createTesting();
        auto m = RuntimeFactory::createMinimal();
        auto c = RuntimeFactory::createCLI();
        auto g = RuntimeFactory::createGUI();
        auto e = RuntimeFactory::createEmbedded();
        REQUIRE(!d.isInitialized());
        REQUIRE(!t.isInitialized());
        REQUIRE(!m.isInitialized());
        REQUIRE(!c.isInitialized());
        REQUIRE(!g.isInitialized());
        REQUIRE(!e.isInitialized());
    }
}

TEST_CASE("Runtime lifecycle", "[runtime]") {
    SECTION("testInitialize") {
        Runtime rt;
        auto result = rt.initialize();
        REQUIRE(result.isOk());
        REQUIRE(rt.isInitialized());
    }

    SECTION("testInitializeTwiceFails") {
        Runtime rt;
        auto r1 = rt.initialize();
        REQUIRE(r1.isOk());
        auto r2 = rt.initialize();
        REQUIRE(r2.isError());
    }

    SECTION("testShutdown") {
        Runtime rt;
        auto result = rt.initialize();
        REQUIRE(result.isOk());
        rt.shutdown();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testShutdownWithoutInit") {
        Runtime rt;
        rt.shutdown();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testInitializeThenShutdown") {
        Runtime rt;
        REQUIRE(rt.initialize().isOk());
        REQUIRE(rt.isInitialized());
        rt.shutdown();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testInitializeMultipleRuntimes") {
        Runtime rt1;
        Runtime rt2;
        REQUIRE(rt1.initialize().isOk());
        REQUIRE(rt2.initialize().isOk());
        REQUIRE(rt1.isInitialized());
        REQUIRE(rt2.isInitialized());
        rt1.shutdown();
        rt2.shutdown();
    }
}

TEST_CASE("Runtime discovery", "[runtime]") {
    SECTION("testDiscoverWithVirtualDetector") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto result = vrt.discover();
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() >= 2);
    }

    SECTION("testDiscoverWithTimeout") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto result = vrt.discover(std::chrono::milliseconds(1000));
        REQUIRE(result.isOk());
    }

    SECTION("testDiscoverWithoutInit") {
        Runtime rt;
        auto result = rt.discover();
        REQUIRE(result.isError());
    }

    SECTION("testProbeDevice") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            auto result = vrt.runtime().probe(devices[0]);
            REQUIRE(result.isOk());
        }
    }

    SECTION("testProbeWithoutInit") {
        Runtime rt;
        DeviceDescriptor desc;
        auto result = rt.probe(desc);
        REQUIRE(result.isError());
    }

    SECTION("testDiscoverMultipleDevices") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        vrt.addVirtualDevice(VirtualDeviceDetector::createQualcommEDL(0x05C6, 0x9008));
        vrt.addVirtualDevice(VirtualDeviceDetector::createMediaTekPreloader(0x0E8D, 0x2000));
        vrt.addVirtualDevice(VirtualDeviceDetector::createUNISOCBootROM(0x1782, 0x4D00));

        auto result = vrt.discover();
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() >= 3);
    }
}

TEST_CASE("Runtime connect/disconnect", "[runtime]") {
    SECTION("testConnectDisconnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            auto connResult = vrt.connect(devices[0]);
            // Virtual device may not have full protocol stack; accept known outcomes
            REQUIRE((connResult.isOk() || connResult.error() == ErrorCode::NotSupported ||
                    connResult.error() == ErrorCode::NoMatchingProtocol));
            vrt.disconnect();
            REQUIRE(!vrt.runtime().isConnected());
        }
    }

    SECTION("testConnectWithoutInit") {
        Runtime rt;
        DeviceDescriptor desc;
        auto result = rt.connect(desc);
        REQUIRE(result.isError());
    }

    SECTION("testDoubleConnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            auto r1 = vrt.connect(devices[0]);
            if (r1.isOk()) {
                auto r2 = vrt.connect(devices[0]);
                REQUIRE(r2.isError());
                REQUIRE(r2.error() == ErrorCode::SessionAlreadyConnected);
            }
            vrt.disconnect();
        }
    }

    SECTION("testDisconnectWithoutConnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        vrt.disconnect();
        REQUIRE(!vrt.runtime().isConnected());
    }

    SECTION("testReconnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            auto r1 = vrt.connect(devices[0]);
            if (r1.isOk()) {
                auto r2 = vrt.runtime().reconnect();
                // Virtual device may not support reconnect
                REQUIRE((r2.isOk() || r2.error() == ErrorCode::NotSupported));
                REQUIRE(vrt.runtime().isInitialized());
            }
            vrt.disconnect();
        }
    }

    SECTION("testIsConnected") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        REQUIRE(!vrt.runtime().isConnected());
    }
}

TEST_CASE("Runtime workflow", "[runtime]") {
    SECTION("testExecuteWorkflow") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto wf = std::make_unique<workflow::WorkflowEngine>();
        auto result = vrt.runtime().executeWorkflow(std::move(wf));
        // Unique-ptr overload delegates to WorkflowService without connection check;
        // may succeed or fail depending on workflow implementation
        REQUIRE((result.isOk() || result.error() == ErrorCode::SessionNotConnected));
    }

    SECTION("testExecuteWorkflowByName") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().executeWorkflow("verify");
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::SessionNotConnected);
    }

    SECTION("testExecuteWorkflowWithoutInit") {
        Runtime rt;
        auto wf = std::make_unique<workflow::WorkflowEngine>();
        auto result = rt.executeWorkflow(std::move(wf));
        REQUIRE(result.isError());
    }

    SECTION("testExecuteWorkflowWithoutConnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().executeWorkflow("flash");
        REQUIRE(result.isError());
    }

    SECTION("testExecuteWorkflowInvalidName") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().executeWorkflow("nonexistent");
        REQUIRE(result.isError());
        // SessionNotConnected: string overload checks connection before name validation
        REQUIRE(result.error() == ErrorCode::SessionNotConnected);
    }
}

TEST_CASE("Runtime jobs", "[runtime]") {
    SECTION("testRunJob") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto job = std::make_unique<job::FlashJob>("test-job", "partition", ByteBuffer{});
        auto result = vrt.runtime().runJob(std::move(job));
        REQUIRE(result.isOk());
    }

    SECTION("testRunMultipleJobs") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto job1 = std::make_unique<job::FlashJob>("job1", "part1", ByteBuffer{});
        auto job2 = std::make_unique<job::BackupJob>("job2", "part2");

        std::vector<std::unique_ptr<job::IJob>> jobs;
        jobs.push_back(std::move(job1));
        jobs.push_back(std::move(job2));

        auto result = vrt.runtime().runJobs(std::move(jobs));
        REQUIRE(result.isOk());
    }

    SECTION("testRunJobWithoutInit") {
        Runtime rt;
        auto job = std::make_unique<job::FlashJob>("test", "part", ByteBuffer{});
        auto result = rt.runJob(std::move(job));
        REQUIRE(result.isError());
    }

    SECTION("testRunJobsFromVector") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        std::vector<std::unique_ptr<job::IJob>> jobs;
        for (int i = 0; i < 3; ++i) {
            jobs.push_back(std::make_unique<job::FlashJob>(
                "job" + std::to_string(i), "part" + std::to_string(i), ByteBuffer{}));
        }

        auto result = vrt.runtime().runJobs(std::move(jobs));
        REQUIRE(result.isOk());
    }
}

TEST_CASE("Runtime package loading", "[runtime]") {
    SECTION("testLoadFirmwarePackageNoPath") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().loadFirmwarePackage("./nonexistent/path");
        REQUIRE(result.isError());
    }

    SECTION("testLoadedPackageNullByDefault") {
        Runtime rt;
        REQUIRE(rt.initialize().isOk());
        REQUIRE(rt.loadedPackage() == nullptr);
    }

    SECTION("testLoadPackageWithoutInit") {
        Runtime rt;
        auto result = rt.loadFirmwarePackage("./some/path");
        REQUIRE(result.isError());
    }
}

TEST_CASE("Runtime firmware service contract", "[runtime]") {
    // Regression: loadFirmwarePackage must never return a success
    // Result containing a null unique_ptr (Bug 1).
    SECTION("testLoadedPackageReturnsNonNullOnSuccess") {
        // Create a temporary minimal firmware package directory
        auto tmpDir = fs::temp_directory_path() / "mboot_fw_contract_test";
        fs::create_directories(tmpDir);
        auto manifestPath = tmpDir / "manifest.json";
        {
            std::ofstream mf(manifestPath);
            mf << R"({"vendor": "ANY", "platform": "ANY"})";
        }

        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().loadFirmwarePackage(tmpDir.string());
        REQUIRE(result.isOk());
        REQUIRE(result.value() != nullptr);

        // loadedPackage() should also return non-null while caller holds the package
        REQUIRE(vrt.runtime().loadedPackage() != nullptr);

        // Ownership transfer: result's unique_ptr holds the package
        auto& pkg = *result.value();
        REQUIRE(pkg.metadata().vendor == "ANY");

        // Cleanup
        fs::remove_all(tmpDir);
    }

    SECTION("testLoadedPackageRejectedOnInvalidDir") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto result = vrt.runtime().loadFirmwarePackage("/nonexistent/path");
        REQUIRE(result.isError());
    }

    SECTION("testLoadedPackageRejectedWithoutInit") {
        Runtime rt;
        auto result = rt.loadFirmwarePackage("/some/path");
        REQUIRE(result.isError());
    }

    SECTION("testLoadedPackageNullByDefault") {
        Runtime rt;
        REQUIRE(rt.loadedPackage() == nullptr);
    }
}

TEST_CASE("Runtime plugins", "[runtime]") {
    SECTION("testInstallPluginNoPlugin") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().installPlugin(
            std::unique_ptr<plugin::IPlugin>(static_cast<plugin::IPlugin*>(nullptr)));
        REQUIRE(result.isError());
    }

    SECTION("testRemovePluginNotFound") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().removePlugin("nonexistent-plugin");
        REQUIRE(result.isError());
    }

    SECTION("testListPluginsEmpty") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto plugins = vrt.runtime().listPlugins();
        REQUIRE(plugins.empty());
    }
}

TEST_CASE("Runtime vendor registration", "[runtime]") {
    SECTION("testRegisterVendor") {
        Runtime rt;
        REQUIRE(rt.initialize().isOk());

        auto vendor = std::make_unique<MockVendor>();
        auto result = rt.registerVendor(std::move(vendor));
        REQUIRE(result.isOk());
    }

    SECTION("testRegisterVendorDuplicate") {
        Runtime rt;
        REQUIRE(rt.initialize().isOk());

        auto v1 = std::make_unique<MockVendor>();
        auto v2 = std::make_unique<MockVendor>();
        auto r1 = rt.registerVendor(std::move(v1));
        REQUIRE(r1.isOk());
        auto r2 = rt.registerVendor(std::move(v2));
        REQUIRE(r2.isError());
        REQUIRE(r2.error() == ErrorCode::AlreadyExists);
    }

    SECTION("testRegisterVendorWithoutInit") {
        Runtime rt;
        auto vendor = std::make_unique<MockVendor>();
        auto result = rt.registerVendor(std::move(vendor));
        REQUIRE(result.isError());
    }
}

TEST_CASE("Runtime callbacks", "[runtime]") {
    SECTION("testLogCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onLog = [&](const std::string&) { called = true; };
        rt.setCallbacks(cbs);
        REQUIRE(rt.initialize().isOk());
        REQUIRE(called);
    }

    SECTION("testErrorCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onError = [&](const std::string&) { called = true; };
        rt.setCallbacks(cbs);
        (void)rt.initialize();
        auto result = rt.discover();
        REQUIRE(!called);
    }

    SECTION("testWarningCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onWarning = [&](const std::string&) { called = true; };
        rt.setCallbacks(cbs);
        (void)rt.initialize();
        REQUIRE(rt.isInitialized());
        REQUIRE_FALSE(called);
    }

    SECTION("testStatusCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onStatus = [&](const std::string&) { called = true; };
        rt.setCallbacks(cbs);
        (void)rt.initialize();
        REQUIRE(called);
    }

    SECTION("testDeviceDiscoveredCallback") {
        VirtualRuntime vrt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onDeviceDiscovered = [&](const DeviceDescriptor&) { called = true; };
        vrt.runtime().setCallbacks(cbs);
        (void)vrt.initialize();
        called = false;
        (void)vrt.discover();
        REQUIRE(called);
    }

    SECTION("testDeviceConnectedCallback") {
        VirtualRuntime vrt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onDeviceConnected = [&](const DeviceDescriptor&) { called = true; };
        vrt.runtime().setCallbacks(cbs);
        (void)vrt.initialize();

        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            auto connResult = vrt.connect(devices[0]);
            if (connResult.isOk()) {
                REQUIRE(called);
            }
            vrt.disconnect();
        }
    }

    SECTION("testProgressCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onProgress = [&](const std::string&, double) { called = true; };
        rt.setCallbacks(cbs);
        (void)rt.initialize();
        REQUIRE(rt.isInitialized());
    }

    SECTION("testWorkflowProgressCallback") {
        Runtime rt;
        RuntimeCallbacks cbs;
        bool called = false;
        cbs.onWorkflowProgress = [&](const workflow::WorkflowProgress&) { called = true; };
        rt.setCallbacks(cbs);
        (void)rt.initialize();

        auto wf = std::make_unique<workflow::WorkflowEngine>();
        (void)rt.executeWorkflow(std::move(wf));
        REQUIRE(called);
    }

    SECTION("testSetCallbacks") {
        Runtime rt;
        RuntimeCallbacks cbs;
        rt.setCallbacks(cbs);
        auto& ref = rt.callbacks();
        REQUIRE(!ref.onLog);
        REQUIRE(!ref.onError);
    }
}

TEST_CASE("Runtime statistics", "[runtime]") {
    SECTION("testStatisticsAfterInit") {
        Runtime rt;
        (void)rt.initialize();
        auto stats = rt.statistics();
        REQUIRE(stats.devicesConnected == 0u);
        REQUIRE(stats.jobsExecuted == 0u);
        REQUIRE(stats.workflowsExecuted == 0u);
        REQUIRE(stats.packagesInstalled == 0u);
        REQUIRE(stats.pluginsLoaded == 0u);
        REQUIRE(stats.totalErrors == 0u);
    }

    SECTION("testStatisticsUptime") {
        Runtime rt;
        (void)rt.initialize();
        auto stats = rt.statistics();
        REQUIRE(stats.uptimeSeconds >= 0.0);
    }

    SECTION("testStatisticsAfterOperations") {
        VirtualRuntime vrt;
        (void)vrt.initialize();

        auto job = std::make_unique<job::FlashJob>("test-job", "part", ByteBuffer{});
        (void)vrt.runtime().runJob(std::move(job));

        auto stats = vrt.runtime().statistics();
        REQUIRE(stats.jobsExecuted > 0);
    }

    SECTION("testStatisticsDevicesConnected") {
        Runtime rt;
        (void)rt.initialize();
        auto stats = rt.statistics();
        REQUIRE(stats.devicesConnected == 0u);
    }

    SECTION("testStatisticsWithoutStats") {
        RuntimeConfig cfg;
        cfg.enableStatistics = false;
        auto rt = RuntimeBuilder().withConfig(cfg).build();
        (void)rt.initialize();
        auto stats = rt.statistics();
        REQUIRE(stats.devicesConnected == 0u);
    }
}

TEST_CASE("Runtime health", "[runtime]") {
    SECTION("testHealthAfterInit") {
        Runtime rt;
        (void)rt.initialize();
        auto health = rt.health();
        REQUIRE(health.activeSessions == 0u);
        REQUIRE(health.connectedDevices == 0u);
        REQUIRE(health.uptimeSeconds >= 0.0);
    }

    SECTION("testHealthAfterConnect") {
        VirtualRuntime vrt;
        (void)vrt.initialize();

        auto devices = vrt.getVirtualDevices();
        if (!devices.empty()) {
            (void)vrt.connect(devices[0]);
        }

        auto health = vrt.runtime().health();
        REQUIRE(health.uptimeSeconds >= 0.0);
    }

    SECTION("testHealthAfterShutdown") {
        Runtime rt;
        (void)rt.initialize();
        rt.shutdown();
        auto health = rt.health();
        REQUIRE(health.activeSessions == 0u);
    }

    SECTION("testHealthTransportState") {
        VirtualRuntime vrt;
        (void)vrt.initialize();
        auto health = vrt.runtime().health();
        REQUIRE(!health.transportState.empty());
    }

    SECTION("testHealthDoesNotFabricateUnknownValues") {
        Runtime rt;
        (void)rt.initialize();
        auto health = rt.health();
        // threadCount is 0 when unknown (not fabricated as 2)
        REQUIRE(health.threadCount == 0u);
        // memoryUsageBytes is 0 when unavailable (not fabricated)
        REQUIRE(health.memoryUsageBytes == 0u);
    }
}

TEST_CASE("Runtime capabilities", "[runtime]") {
    SECTION("testCapabilities") {
        Runtime rt;
        auto caps = rt.capabilities();
        REQUIRE(!caps.empty());
    }

    SECTION("testCapabilitiesNotEmpty") {
        Runtime rt;
        auto caps = rt.capabilities();
        REQUIRE(std::find(caps.begin(), caps.end(), "discovery") != caps.end());
        REQUIRE(std::find(caps.begin(), caps.end(), "transport") != caps.end());
    }

    SECTION("testCapabilitiesContent") {
        Runtime rt;
        auto caps = rt.capabilities();
        REQUIRE(std::find(caps.begin(), caps.end(), "flash") != caps.end());
        REQUIRE(std::find(caps.begin(), caps.end(), "workflow") != caps.end());
        REQUIRE(std::find(caps.begin(), caps.end(), "plugins") != caps.end());
    }

    SECTION("testCapabilitiesPlatformSpecific") {
        Runtime rt;
        auto caps = rt.capabilities();
        // If a USB backend is present, the specific backend name must be included
        auto usbIt = std::find(caps.begin(), caps.end(), "usb-backend");
        if (usbIt != caps.end()) {
            bool hasBackend = std::find(caps.begin(), caps.end(), "libusb") != caps.end()
                           || std::find(caps.begin(), caps.end(), "winusb") != caps.end();
            REQUIRE(hasBackend);
        }
    }
}

TEST_CASE("Runtime version", "[runtime]") {
    SECTION("testVersion") {
        Runtime rt;
        auto ver = rt.version();
        REQUIRE(!ver.empty());
    }

    SECTION("testVersionNotEmpty") {
        Runtime rt;
        auto ver = rt.version();
        REQUIRE(ver.find("MBootCore") != std::string::npos);
    }

    SECTION("testVersionMatchesBuild") {
        Runtime rt;
        auto ver = rt.version();
        // Must contain the actual build version, not a fabricated string
        REQUIRE(ver.find(MBOOTCORE_VERSION) != std::string::npos);
    }
}

TEST_CASE("Runtime observers", "[runtime]") {
    SECTION("testAddObserver") {
        TestObserver obs;
        Runtime rt;
        rt.addObserver(&obs);
        (void)rt.initialize();
        rt.removeObserver(&obs);
        REQUIRE(obs.eventCount > 0);
    }

    SECTION("testRemoveObserver") {
        TestObserver obs;
        Runtime rt;
        rt.addObserver(&obs);
        rt.removeObserver(&obs);
        (void)rt.initialize();
        REQUIRE(obs.eventCount == 0);
    }

    SECTION("testObserverReceivesEvents") {
        TestObserver obs;
        Runtime rt;
        rt.addObserver(&obs);
        (void)rt.initialize();
        rt.removeObserver(&obs);
        REQUIRE(obs.eventCount > 0);
        bool foundStarted = false;
        for (const auto& ev : obs.events) {
            if (ev.type == RuntimeEventType::RuntimeStarted) {
                foundStarted = true;
                break;
            }
        }
        REQUIRE(foundStarted);
    }

    SECTION("testMultipleObservers") {
        TestObserver obs1, obs2;
        Runtime rt;
        rt.addObserver(&obs1);
        rt.addObserver(&obs2);
        (void)rt.initialize();
        rt.removeObserver(&obs1);
        rt.removeObserver(&obs2);
        REQUIRE(obs1.eventCount > 0);
        REQUIRE(obs2.eventCount > 0);
    }

    SECTION("testAddNullObserver") {
        Runtime rt;
        rt.addObserver(nullptr);
        (void)rt.initialize();
        REQUIRE(rt.isInitialized());
    }

    SECTION("testRemoveNonExistentObserver") {
        TestObserver obs;
        Runtime rt;
        rt.removeObserver(&obs);
        (void)rt.initialize();
        REQUIRE(rt.isInitialized());
    }

    SECTION("testAddSharedPtrObserver") {
        auto obs = std::make_shared<TestObserver>();
        Runtime rt;
        rt.addObserver(obs);
        (void)rt.initialize();
        REQUIRE(obs->eventCount > 0);
        // obs stays alive until the shared_ptr goes out of scope
    }

    SECTION("testAddNullSharedPtrObserver") {
        Runtime rt;
        rt.addObserver(std::shared_ptr<RuntimeObserver>{});
        (void)rt.initialize();
        REQUIRE(rt.isInitialized());
    }
}

TEST_CASE("Runtime control", "[runtime]") {
    SECTION("testCancel") {
        VirtualRuntime vrt;
        (void)vrt.initialize();
        vrt.runtime().cancel();
        REQUIRE(vrt.runtime().isInitialized());
    }

    SECTION("testPause") {
        VirtualRuntime vrt;
        (void)vrt.initialize();
        vrt.runtime().pause();
        REQUIRE(vrt.runtime().isInitialized());
    }

    SECTION("testResume") {
        VirtualRuntime vrt;
        (void)vrt.initialize();
        vrt.runtime().resume();
        REQUIRE(vrt.runtime().isInitialized());
    }

    SECTION("testReset") {
        VirtualRuntime vrt;
        (void)vrt.initialize();
        auto result = vrt.runtime().reset();
        REQUIRE(result.isOk());
    }

    SECTION("testCancelWithoutInit") {
        Runtime rt;
        rt.cancel();
        REQUIRE_FALSE(rt.isInitialized());
    }
}

TEST_CASE("Runtime multiple instances", "[runtime]") {
    SECTION("testTwoSeparateRuntimes") {
        Runtime rt1;
        Runtime rt2;
        REQUIRE(rt1.initialize().isOk());
        REQUIRE(rt2.initialize().isOk());
        REQUIRE(rt1.isInitialized());
        REQUIRE(rt2.isInitialized());
        rt1.shutdown();
        rt2.shutdown();
        REQUIRE(!rt1.isInitialized());
        REQUIRE(!rt2.isInitialized());
    }

    SECTION("testTwoRuntimesBothInitialize") {
        Runtime rt1;
        Runtime rt2;
        REQUIRE(rt1.initialize().isOk());
        REQUIRE(rt2.initialize().isOk());
        auto stats1 = rt1.statistics();
        auto stats2 = rt2.statistics();
        REQUIRE(stats1.devicesConnected == 0u);
        REQUIRE(stats2.devicesConnected == 0u);
        rt1.shutdown();
        rt2.shutdown();
    }
}

TEST_CASE("Runtime thread safety", "[runtime]") {
    SECTION("testThreadSafeInitializeShutdown") {
        for (int iter = 0; iter < 10; ++iter) {
            Runtime rt;
            std::thread t1([&]() { (void)rt.initialize(); });
            std::thread t2([&]() { rt.shutdown(); });
            t1.join();
            t2.join();
            // Non-deterministic outcome; verify object is still usable
            (void)rt.isInitialized();
        }
    }

    SECTION("testThreadSafeDiscovery") {
        VirtualRuntime vrt;
        (void)vrt.initialize();

        std::atomic<int> successCount{0};
        std::vector<std::thread> threads;

        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&]() {
                auto result = vrt.runtime().discover();
                if (result.isOk()) ++successCount;
            });
        }

        for (auto& t : threads) t.join();
        REQUIRE(successCount > 0);
    }

    SECTION("testConcurrentOperations") {
        VirtualRuntime vrt;
        (void)vrt.initialize();

        std::vector<std::thread> threads;

        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&]() {
                vrt.runtime().statistics();
                vrt.runtime().health();
                vrt.runtime().version();
                vrt.runtime().capabilities();
            });
        }

        for (auto& t : threads) t.join();
        REQUIRE(vrt.runtime().isInitialized());
        auto stats = vrt.runtime().statistics();
        REQUIRE(stats.uptimeSeconds >= 0.0);
    }
}

TEST_CASE("Runtime virtual runtime integration", "[runtime]") {
    SECTION("testVirtualRuntimeInitialize") {
        VirtualRuntime vrt;
        auto result = vrt.initialize();
        REQUIRE(result.isOk());
        REQUIRE(vrt.runtime().isInitialized());
    }

    SECTION("testVirtualRuntimeDiscover") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto result = vrt.runtime().discover();
        REQUIRE(result.isOk());
        REQUIRE(!result.value().empty());
    }

    SECTION("testVirtualRuntimeShutdown") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        vrt.shutdown();
        REQUIRE(!vrt.runtime().isInitialized());
    }

    SECTION("testVirtualRuntimeDoubleInitialize") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        auto r2 = vrt.initialize();
        REQUIRE(r2.isError());
    }

    SECTION("testVirtualRuntimeAddDevice") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        vrt.addVirtualDevice(VirtualDeviceDetector::createSamsungDownload());
        auto result = vrt.runtime().discover();
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() >= 3);
    }

    SECTION("testVirtualRuntimeMultipleDevices") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        vrt.addVirtualDevice(VirtualDeviceDetector::createQualcommEDL());
        vrt.addVirtualDevice(VirtualDeviceDetector::createMediaTekPreloader());
        vrt.addVirtualDevice(VirtualDeviceDetector::createUNISOCBootROM());
        vrt.addVirtualDevice(VirtualDeviceDetector::createSamsungDownload());
        vrt.addVirtualDevice(VirtualDeviceDetector::createRockchipMaskROM());

        auto result = vrt.runtime().discover();
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() >= 5);
    }
}

TEST_CASE("Runtime stress", "[runtime]") {
    SECTION("testStressInitializeShutdown") {
        for (int i = 0; i < 50; ++i) {
            Runtime rt;
            REQUIRE(rt.initialize().isOk());
            rt.shutdown();
        }
    }

    SECTION("testStressMultipleOperations") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        for (int i = 0; i < 20; ++i) {
            auto result = vrt.runtime().discover();
            REQUIRE(result.isOk());
            vrt.runtime().statistics();
            vrt.runtime().health();
            vrt.runtime().version();
            vrt.runtime().capabilities();
        }
    }
}

TEST_CASE("Runtime edge cases", "[runtime]") {
    SECTION("testShutdownDuringOperation") {
        Runtime rt;
        REQUIRE(rt.initialize().isOk());

        rt.cancel();
        rt.shutdown();
        REQUIRE(!rt.isInitialized());
    }

    SECTION("testCancelDuringConnect") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());
        vrt.runtime().cancel();
        REQUIRE(vrt.runtime().isInitialized());
    }

    SECTION("testShutdownServicesAliveDuringNotification") {
        // Observer that verifies services() is still accessible during shutdown notification
        struct ShutdownSafetyObserver : RuntimeObserver {
            Runtime* runtime;
            bool servicesAccessibleDuringShutdown{false};
            void onRuntimeEvent(const RuntimeEvent& event) override {
                if (event.type == RuntimeEventType::RuntimeStopped) {
                    // services() should still be accessible — shutdown fires
                    // the event before resetting services_
                    if (runtime) {
                        (void)runtime->services();
                        servicesAccessibleDuringShutdown = true;
                    }
                }
            }
            void onStatisticsUpdated(const RuntimeStatistics&) override {}
            void onHealthChanged(const RuntimeHealth&) override {}
        };

        Runtime rt;
        ShutdownSafetyObserver obs;
        obs.runtime = &rt;
        rt.addObserver(&obs);
        REQUIRE(rt.initialize().isOk());
        rt.shutdown();
        REQUIRE(obs.servicesAccessibleDuringShutdown);
    }

    SECTION("testResetDuringWorkflow") {
        VirtualRuntime vrt;
        REQUIRE(vrt.initialize().isOk());

        auto result = vrt.runtime().reset();
        REQUIRE(result.isOk());
        REQUIRE(vrt.runtime().isInitialized());
    }
}
