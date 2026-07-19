#include <catch2/catch_test_macros.hpp>

#include <CliParser.hpp>
#include <CliApplication.hpp>
#include <CliFormatter.hpp>

#include <VirtualRuntime.hpp>

#include <fstream>
#include <sstream>

using namespace mboot::cli;
using namespace mbootcore::runtime;

TEST_CASE("CliScriptTest", "[cli]") {
    CliParser parser;

    SECTION("testParseScriptCommand") {
        auto cmd = parser.parseLine("script flash_script.mboot");
        REQUIRE(cmd.type == CommandType::Script);
        REQUIRE(cmd.args.size() == size_t(1));
        REQUIRE(cmd.args[0] == "flash_script.mboot");
    }

    SECTION("testParseScriptWithOptions") {
        auto cmd = parser.parseLine("script flash.mboot --verbose");
        REQUIRE(cmd.type == CommandType::Script);
        REQUIRE(cmd.options.verbose);
    }

    SECTION("testParseCommentLine") {
        auto tokens = parser.tokenize("# This is a comment");
        REQUIRE(tokens.size() == size_t(5));
        REQUIRE(tokens[0] == "#");
    }

    SECTION("testParseEmptyLineLine") {
        auto cmd = parser.parseLine("");
        REQUIRE(cmd.type == CommandType::Help);
    }

    SECTION("testParseWhitespaceOnly") {
        auto cmd = parser.parseLine("   ");
        REQUIRE(cmd.type == CommandType::Help);
    }

    SECTION("testMultipleCommands") {
        auto lines = {
            std::string("version"),
            std::string("discover"),
            std::string("connect 0"),
            std::string("flash package.mbn")
        };
        int count = 0;
        for (auto& line : lines) {
            auto cmd = parser.parseLine(line);
            REQUIRE(cmd.type != CommandType::Invalid);
            ++count;
        }
        REQUIRE(count == 4);
    }

    SECTION("testScriptWithMixedOptions") {
        auto cmd = parser.parseLine("flash firmware.mbn --verbose --dry-run --json");
        REQUIRE(cmd.type == CommandType::Flash);
        REQUIRE(cmd.options.verbose);
        REQUIRE(cmd.options.dryRun);
        REQUIRE(cmd.options.json);
    }

    SECTION("testScriptErrorHandling") {
        auto cmd = parser.parseLine("nonexistent_command");
        REQUIRE(cmd.type == CommandType::Invalid);
    }

    SECTION("testScriptExitCommand") {
        auto cmd = parser.parseLine("exit");
        REQUIRE(cmd.type == CommandType::Invalid);
    }

    SECTION("testScriptPauseAndResume") {
        auto pause = parser.parseLine("pause");
        REQUIRE(pause.type == CommandType::Pause);

        auto resume = parser.parseLine("resume");
        REQUIRE(resume.type == CommandType::Resume);
    }

    SECTION("testScriptCancel") {
        auto cmd = parser.parseLine("cancel");
        REQUIRE(cmd.type == CommandType::Cancel);
    }

    SECTION("testScriptReboot") {
        auto cmd = parser.parseLine("reboot");
        REQUIRE(cmd.type == CommandType::Reboot);
    }
}
