#include <catch2/catch_test_macros.hpp>

#include <CliParser.hpp>

using namespace mboot::cli;

TEST_CASE("CliParserTest", "[cli]") {
    CliParser parser;

    SECTION("testEmptyInput") {
        auto cmd = parser.parse({});
        REQUIRE(cmd.type == CommandType::Help);
    }

    SECTION("testHelpCommand") {
        auto cmd = parser.parse({"help"});
        REQUIRE(cmd.type == CommandType::Help);
    }

    SECTION("testVersionCommand") {
        auto cmd = parser.parse({"version"});
        REQUIRE(cmd.type == CommandType::Version);
    }

    SECTION("testDiscoverCommand") {
        auto cmd = parser.parse({"discover"});
        REQUIRE(cmd.type == CommandType::Discover);
    }

    SECTION("testConnectCommand") {
        auto cmd = parser.parse({"connect", "0"});
        REQUIRE(cmd.type == CommandType::Connect);
        REQUIRE(cmd.args.size() == size_t(1));
        REQUIRE(cmd.args[0] == "0");
    }

    SECTION("testDisconnectCommand") {
        auto cmd = parser.parse({"disconnect"});
        REQUIRE(cmd.type == CommandType::Disconnect);
    }

    SECTION("testFlashCommand") {
        auto cmd = parser.parse({"flash", "package.mbn"});
        REQUIRE(cmd.type == CommandType::Flash);
        REQUIRE(cmd.args[0] == "package.mbn");
    }

    SECTION("testReadCommand") {
        auto cmd = parser.parse({"read", "0x1000", "1024"});
        REQUIRE(cmd.type == CommandType::Read);
        REQUIRE(cmd.args.size() == size_t(2));
    }

    SECTION("testWriteCommand") {
        auto cmd = parser.parse({"write", "0x2000", "AABBCCDD"});
        REQUIRE(cmd.type == CommandType::Write);
    }

    SECTION("testEraseCommand") {
        auto cmd = parser.parse({"erase", "0x3000", "4096"});
        REQUIRE(cmd.type == CommandType::Erase);
    }

    SECTION("testVerifyCommand") {
        auto cmd = parser.parse({"verify", "0x4000", "DEADBEEF"});
        REQUIRE(cmd.type == CommandType::Verify);
    }

    SECTION("testBackupCommand") {
        auto cmd = parser.parse({"backup", "boot"});
        REQUIRE(cmd.type == CommandType::Backup);
        REQUIRE(cmd.args[0] == "boot");
    }

    SECTION("testRestoreCommand") {
        auto cmd = parser.parse({"restore", "boot", "backup.img"});
        REQUIRE(cmd.type == CommandType::Restore);
        REQUIRE(cmd.args.size() == size_t(2));
    }

    SECTION("testWorkflowCommand") {
        auto cmd = parser.parse({"workflow", "qualcomm_flash"});
        REQUIRE(cmd.type == CommandType::Workflow);
    }

    SECTION("testStatsAlias") {
        auto cmd = parser.parse({"stats"});
        REQUIRE(cmd.type == CommandType::Statistics);
    }

    SECTION("testStatisticsCommand") {
        auto cmd = parser.parse({"statistics"});
        REQUIRE(cmd.type == CommandType::Statistics);
    }

    SECTION("testHealthCommand") {
        auto cmd = parser.parse({"health"});
        REQUIRE(cmd.type == CommandType::Health);
    }

    SECTION("testCapabilitiesCommand") {
        auto cmd = parser.parse({"capabilities"});
        REQUIRE(cmd.type == CommandType::Capabilities);
    }

    SECTION("testShellCommand") {
        auto cmd = parser.parse({"shell"});
        REQUIRE(cmd.type == CommandType::Shell);
    }

    SECTION("testScriptCommand") {
        auto cmd = parser.parse({"script", "flash.mboot"});
        REQUIRE(cmd.type == CommandType::Script);
        REQUIRE(cmd.args[0] == "flash.mboot");
    }

    SECTION("testPluginListCommand") {
        auto cmd = parser.parse({"plugin", "list"});
        REQUIRE(cmd.type == CommandType::Plugin);
    }

    SECTION("testCompletionCommand") {
        auto cmd = parser.parse({"completion", "bash"});
        REQUIRE(cmd.type == CommandType::Completion);
    }

    SECTION("testVerboseOption") {
        auto cmd = parser.parse({"flash", "pkg.mbn", "--verbose"});
        REQUIRE(cmd.type == CommandType::Flash);
        REQUIRE(cmd.options.verbose);
    }

    SECTION("testJsonOption") {
        auto cmd = parser.parse({"discover", "--json"});
        REQUIRE(cmd.options.json);
    }

    SECTION("testMultipleOptions") {
        auto cmd = parser.parse({"flash", "pkg.mbn", "--verbose", "--dry-run", "--timeout=30000"});
        REQUIRE(cmd.options.verbose);
        REQUIRE(cmd.options.dryRun);
        REQUIRE(cmd.options.timeoutMs == 30000);
    }

    SECTION("testUnknownCommand") {
        auto cmd = parser.parse({"nonexistent"});
        REQUIRE(cmd.type == CommandType::Invalid);
    }

    SECTION("testTokenizeQuotedStrings") {
        auto tokens = parser.tokenize("flash \"my package.mbn\" --verbose");
        REQUIRE(tokens.size() == size_t(3));
        REQUIRE(tokens[0] == "flash");
        REQUIRE(tokens[1] == "my package.mbn");
        REQUIRE(tokens[2] == "--verbose");
    }

    SECTION("testTokenizeEmptyLine") {
        auto tokens = parser.tokenize("");
        REQUIRE(tokens.empty());
    }

    SECTION("testPackageInfoCommand") {
        auto cmd = parser.parse({"package", "info", "firmware.mbn"});
        REQUIRE(cmd.type == CommandType::Package);
        REQUIRE(cmd.args.size() == size_t(2));
        REQUIRE(cmd.args[0] == "info");
    }

    SECTION("testResetCommand") {
        auto cmd = parser.parse({"reset"});
        REQUIRE(cmd.type == CommandType::Reset);
    }

    SECTION("testCancelCommand") {
        auto cmd = parser.parse({"cancel"});
        REQUIRE(cmd.type == CommandType::Cancel);
    }
}
