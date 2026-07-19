#include <catch2/catch_test_macros.hpp>

#include <CliFormatter.hpp>

using namespace mboot::cli;
using namespace mbootcore::discovery;
using namespace mbootcore::runtime;

TEST_CASE("CliFormatterTest", "[cli]") {

    SECTION("testHumanDevice") {
        CliFormatter fmt(OutputFormat::Human);
        DeviceDescriptor d;
        d.friendlyName = "Qualcomm EDL";
        d.vendor = Vendor::Qualcomm;
        d.protocolHint = ProtocolType::Sahara;
        auto s = fmt.formatDevice(d);
        REQUIRE(s.find("Qualcomm EDL") != std::string::npos);
    }

    SECTION("testJsonDevice") {
        CliFormatter fmt(OutputFormat::Json);
        DeviceDescriptor d;
        d.friendlyName = "Test Device";
        d.vendor = Vendor::Qualcomm;
        d.usbVid = 0x05c6;
        d.usbPid = 0x9008;
        auto s = fmt.formatDevice(d);
        REQUIRE(s.find("Test Device") != std::string::npos);
        REQUIRE(s.find("friendlyName") != std::string::npos);
        REQUIRE(s.find("{") != std::string::npos);
        REQUIRE(s.find("}") != std::string::npos);
    }

    SECTION("testXmlDevice") {
        CliFormatter fmt(OutputFormat::Xml);
        DeviceDescriptor d;
        d.friendlyName = "Test Device";
        d.vendor = Vendor::Qualcomm;
        auto s = fmt.formatDevice(d);
        REQUIRE(s.find("<device>") != std::string::npos);
        REQUIRE(s.find("Test Device") != std::string::npos);
    }

    SECTION("testHumanStatistics") {
        CliFormatter fmt(OutputFormat::Human);
        RuntimeStatistics stats;
        stats.devicesConnected = 5;
        stats.workflowsExecuted = 10;
        stats.uptimeSeconds = 3600.0;
        auto s = fmt.formatStatistics(stats);
        REQUIRE(s.find("5") != std::string::npos);
        REQUIRE(s.find("10") != std::string::npos);
    }

    SECTION("testJsonStatistics") {
        CliFormatter fmt(OutputFormat::Json);
        RuntimeStatistics stats;
        stats.devicesConnected = 3;
        stats.workflowsExecuted = 7;
        auto s = fmt.formatStatistics(stats);
        REQUIRE(s.find("devicesConnected") != std::string::npos);
        REQUIRE(s.find("{") != std::string::npos);
        REQUIRE(s.find("}") != std::string::npos);
    }

    SECTION("testHumanHealth") {
        CliFormatter fmt(OutputFormat::Human);
        RuntimeHealth health;
        health.activeSessions = 2;
        health.connectedDevices = 2;
        health.transportState = "USB";
        auto s = fmt.formatHealth(health);
        REQUIRE(s.find("USB") != std::string::npos);
    }

    SECTION("testJsonHealth") {
        CliFormatter fmt(OutputFormat::Json);
        RuntimeHealth health;
        health.activeSessions = 1;
        health.uptimeSeconds = 100.0;
        auto s = fmt.formatHealth(health);
        REQUIRE(s.find("uptimeSeconds") != std::string::npos);
    }

    SECTION("testHumanResult") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatResult("flash", true, "Package flashed");
        REQUIRE(s.find("[OK]") != std::string::npos);
        REQUIRE(s.find("flash") != std::string::npos);

        s = fmt.formatResult("write", false, "Write failed");
        REQUIRE(s.find("[FAIL]") != std::string::npos);
    }

    SECTION("testJsonResult") {
        CliFormatter fmt(OutputFormat::Json);
        auto s = fmt.formatResult("test", true, "ok");
        REQUIRE(s.find("\"success\":true") != std::string::npos);
        REQUIRE(s.find("{") != std::string::npos);
        REQUIRE(s.find("}") != std::string::npos);
    }

    SECTION("testHumanError") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatError("connect", "Device not found", 3);
        REQUIRE(s.find("Error [3]") != std::string::npos);
    }

    SECTION("testJsonError") {
        CliFormatter fmt(OutputFormat::Json);
        auto s = fmt.formatError("read", "Timeout", 8);
        REQUIRE(s.find("\"exitCode\":8") != std::string::npos);
        REQUIRE(s.find("{") != std::string::npos);
        REQUIRE(s.find("}") != std::string::npos);
    }

    SECTION("testVersion") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatVersion("1.0.0");
        REQUIRE(s.find("1.0.0") != std::string::npos);
    }

    SECTION("testVersionExactOutput") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatVersion(MBOOTCORE_VERSION);
        REQUIRE(s == std::string("MBootCore CLI v") + MBOOTCORE_VERSION);
    }

    SECTION("testJsonVersion") {
        CliFormatter fmt(OutputFormat::Json);
        auto s = fmt.formatVersion("2.0");
        REQUIRE(s.find("\"version\":\"2.0\"") != std::string::npos);
        REQUIRE(s.find("{") != std::string::npos);
        REQUIRE(s.find("}") != std::string::npos);
    }

    SECTION("testCapabilities") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatCapabilities({"sahara", "firehose", "gpt"});
        REQUIRE(s.find("sahara") != std::string::npos);
    }

    SECTION("testList") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatList({"item1", "item2"});
        REQUIRE(s.find("item1") != std::string::npos);
    }

    SECTION("testFormatString") {
        CliFormatter fmt(OutputFormat::Human);
        auto s = fmt.formatString("key", "value");
        REQUIRE(s == "key: value");
    }

    SECTION("testJsonFormatString") {
        CliFormatter fmt(OutputFormat::Json);
        auto s = fmt.formatString("key", "value");
        REQUIRE(s.find("key") != std::string::npos);
        REQUIRE(s.find("value") != std::string::npos);
    }

    SECTION("testHelpTextNonEmpty") {
        CliFormatter fmt;
        auto s = fmt.helpText();
        REQUIRE(!s.empty());
        REQUIRE(s.find("MBoot CLI") != std::string::npos);
    }

    SECTION("testWelcomeText") {
        CliFormatter fmt;
        auto s = fmt.welcomeText();
        REQUIRE(s.find("MBoot CLI") != std::string::npos);
    }
}
