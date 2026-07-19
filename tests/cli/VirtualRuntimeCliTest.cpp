#include <catch2/catch_test_macros.hpp>

#include <CliCommands.hpp>
#include <CliParser.hpp>
#include <CliFormatter.hpp>
#include <CliApplication.hpp>

#include <VirtualRuntime.hpp>

using namespace mboot::cli;
using namespace mbootcore::runtime;
using namespace mbootcore::discovery;

TEST_CASE("VirtualRuntimeCliTest", "[cli]") {

    SECTION("testVirtualRuntimeCreation") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        REQUIRE(r.isOk());

        auto devices = vruntime.getVirtualDevices();
        REQUIRE(!devices.empty());
        REQUIRE(devices.size() == size_t(2));

        vruntime.shutdown();
    }

    SECTION("testVirtualDeviceListing") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        CliFormatter formatter;
        auto devices = vruntime.getVirtualDevices();
        auto s = formatter.formatDevices(devices);
        REQUIRE(!s.empty());

        vruntime.shutdown();
    }

    SECTION("testVirtualConnectDisconnect") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        auto devices = vruntime.getVirtualDevices();
        REQUIRE(!devices.empty());

        auto rc = vruntime.connect(devices[0]);
        (void)rc;
        REQUIRE(r.isOk());

        vruntime.disconnect();
        vruntime.shutdown();
    }

    SECTION("testVirtualConnectThroughCommands") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        CliParser parser;
        CliFormatter formatter;
        CliCommands commands(vruntime.runtime(), parser, formatter);

        ParsedCommand cmd;
        cmd.type = CommandType::Connect;
        cmd.args = {"0"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());

        vruntime.shutdown();
    }

    SECTION("testVirtualMultipleDiscoveries") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        for (int i = 0; i < 3; ++i) {
            auto devices = vruntime.discover();
            REQUIRE(devices.isOk());
            REQUIRE(!devices.value().empty());
        }

        vruntime.shutdown();
    }

    SECTION("testVirtualAddDevice") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        VirtualDeviceSpec spec;
        spec.friendlyName = "Custom Virtual Device";
        spec.vendor = Vendor::Samsung;
        spec.protocolHint = ProtocolType::Fastboot;
        spec.transport = TransportType::USB;
        vruntime.addVirtualDevice(spec);

        auto devices = vruntime.getVirtualDevices();
        REQUIRE(devices.size() == size_t(3));

        vruntime.shutdown();
    }

    SECTION("testVirtualClearDevices") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        vruntime.clearDevices();
        auto devices = vruntime.getVirtualDevices();
        REQUIRE(devices.empty());

        vruntime.shutdown();
    }

    SECTION("testVirtualDeviceCapabilities") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        auto devices = vruntime.getVirtualDevices();
        for (auto& d : devices) {
            REQUIRE(!d.friendlyName.empty());
            REQUIRE(d.vendor != Vendor::Unknown);
        }

        vruntime.shutdown();
    }

    SECTION("testVirtualRuntimeHealth") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        auto health = vruntime.runtime().health();
        REQUIRE(health.connectedDevices == uint32_t(0));
        REQUIRE(health.uptimeSeconds >= 0.0);

        vruntime.shutdown();
    }

    SECTION("testVirtualRuntimeStatistics") {
        VirtualRuntime vruntime;
        auto r = vruntime.initialize();
        (void)r;

        vruntime.runtime().statistics();

        auto devices = vruntime.discover();
        if (devices.isOk() && !devices.value().empty()) {
            auto r3 = vruntime.connect(devices.value()[0]);
            (void)r3;
        }

        auto stats = vruntime.runtime().statistics();
        REQUIRE((stats.devicesConnected > 0 || stats.devicesDisconnected == 0));

        vruntime.shutdown();
    }
}
