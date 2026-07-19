#include <catch2/catch_test_macros.hpp>

#include <CliCommands.hpp>
#include <CliParser.hpp>
#include <CliFormatter.hpp>

#include <VirtualRuntime.hpp>

using namespace mboot::cli;
using namespace mbootcore::runtime;

TEST_CASE("CliCommandTest", "[cli]") {
    VirtualRuntime vruntime;
    CliParser parser;
    CliFormatter formatter;
    CliCommands commands(vruntime.runtime(), parser, formatter);

    auto r = vruntime.initialize();
    REQUIRE(r.isOk());

    SECTION("testDiscover") {
        ParsedCommand cmd;
        cmd.type = CommandType::Discover;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testVersion") {
        ParsedCommand cmd;
        cmd.type = CommandType::Version;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testVersionExactOutput") {
        ParsedCommand cmd;
        cmd.type = CommandType::Version;
        auto result = commands.execute(cmd);
        REQUIRE(result == std::string("MBootCore CLI v") + MBOOTCORE_VERSION);
    }

    SECTION("testVersionFullPath") {
        auto parsed = parser.parse({"version"});
        REQUIRE(parsed.type == CommandType::Version);
        auto result = commands.execute(parsed);
        REQUIRE(result.find("MBootCore CLI v") == 0);
        REQUIRE(result.find(MBOOTCORE_VERSION) != std::string::npos);
    }

    SECTION("testHelpTextContainsVersion") {
        ParsedCommand cmd;
        cmd.type = CommandType::Help;
        auto result = commands.execute(cmd);
        REQUIRE(result.find(MBOOTCORE_VERSION) != std::string::npos);
    }

    SECTION("testHelp") {
        ParsedCommand cmd;
        cmd.type = CommandType::Help;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testList") {
        ParsedCommand cmd;
        cmd.type = CommandType::List;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testConnect") {
        auto devices = vruntime.discover();
        REQUIRE(devices.isOk());
        REQUIRE(!devices.value().empty());

        ParsedCommand cmd;
        cmd.type = CommandType::Connect;
        cmd.args = {"0"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testDisconnect") {
        ParsedCommand cmd;
        cmd.type = CommandType::Disconnect;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testReconnect") {
        ParsedCommand cmd;
        cmd.type = CommandType::Reconnect;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testFlashWithDryRun") {
        ParsedCommand cmd;
        cmd.type = CommandType::Flash;
        cmd.args = {"test_package.mbn"};
        cmd.options.dryRun = true;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
        REQUIRE((result.find("dry-run") != std::string::npos || result.find("OK") != std::string::npos));
    }

    SECTION("testRead") {
        ParsedCommand cmd;
        cmd.type = CommandType::Read;
        cmd.args = {"0x1000", "64"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testErase") {
        ParsedCommand cmd;
        cmd.type = CommandType::Erase;
        cmd.args = {"0x2000", "4096"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testBackup") {
        ParsedCommand cmd;
        cmd.type = CommandType::Backup;
        cmd.args = {"boot"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testRestore") {
        ParsedCommand cmd;
        cmd.type = CommandType::Restore;
        cmd.args = {"boot", "backup.bin"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testWorkflow") {
        ParsedCommand cmd;
        cmd.type = CommandType::Workflow;
        cmd.args = {"qualcomm_flash"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testPluginList") {
        ParsedCommand cmd;
        cmd.type = CommandType::Plugin;
        cmd.args = {"list"};
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testStatistics") {
        ParsedCommand cmd;
        cmd.type = CommandType::Statistics;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testHealth") {
        ParsedCommand cmd;
        cmd.type = CommandType::Health;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testCapabilities") {
        ParsedCommand cmd;
        cmd.type = CommandType::Capabilities;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testReset") {
        ParsedCommand cmd;
        cmd.type = CommandType::Reset;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    SECTION("testInvalidCommand") {
        ParsedCommand cmd;
        cmd.type = CommandType::Invalid;
        auto result = commands.execute(cmd);
        REQUIRE(!result.empty());
    }

    vruntime.shutdown();
}
