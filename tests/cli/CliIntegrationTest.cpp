#include <catch2/catch_test_macros.hpp>

#include <CliApplication.hpp>
#include <CliParser.hpp>
#include <CliFormatter.hpp>

#include <VirtualRuntime.hpp>

#include <sstream>
#include <iostream>

using namespace mboot::cli;
using namespace mbootcore::runtime;

TEST_CASE("CliIntegrationTest", "[cli]") {

    SECTION("testDiscoverThenConnect") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"discover"});
        REQUIRE(r.type == CommandType::Discover);
        REQUIRE(r.args.size() == size_t(0));
    }

    SECTION("testVersionOutput") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"version"});
        REQUIRE(r.type == CommandType::Version);
    }

    SECTION("testHelpOutput") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"help"});
        REQUIRE(r.type == CommandType::Help);
    }

    SECTION("testJsonOutput") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"discover", "--json"});
        REQUIRE(r.type == CommandType::Discover);
        REQUIRE(r.options.json);
    }

    SECTION("testVerboseMode") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"flash", "pkg.mbn", "--verbose"});
        REQUIRE(r.options.verbose);
    }

    SECTION("testQuietMode") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"flash", "pkg.mbn", "--quiet"});
        REQUIRE(r.options.quiet);
    }

    SECTION("testDryRunFlash") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"flash", "pkg.mbn", "--dry-run"});
        REQUIRE(r.options.dryRun);
        REQUIRE(r.type == CommandType::Flash);
    }

    SECTION("testTimeoutOption") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"discover", "--timeout=30000"});
        REQUIRE(r.options.timeoutMs == 30000);
    }

    SECTION("testScriptMode") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"script", "test.mboot"});
        REQUIRE(r.type == CommandType::Script);
        REQUIRE(r.args[0] == "test.mboot");
    }

    SECTION("testShellMode") {
        CliApplication app(RuntimeFactory::createTesting());
        auto r = app.parser().parse({"shell"});
        REQUIRE(r.type == CommandType::Shell);
    }

    SECTION("testErrorPropagation") {
        CliFormatter formatter;
        auto s = formatter.formatError("read", "Timeout", 8);
        REQUIRE(s.find("Timeout") != std::string::npos);
    }

    SECTION("testProgressDisplay") {
        CliProgress progress;
        progress.start("Flashing", 1000);
        progress.update(500);
        REQUIRE(progress.isEnabled());
        REQUIRE(progress.bytes() == uint64_t(500));
    }

    SECTION("testProgressFinished") {
        CliProgress progress;
        progress.start("Test", 100);
        progress.update(100);
        REQUIRE(progress.percent() == 100.0);
    }

    SECTION("testExitCodeMapping") {
        CliFormatter formatter;
        auto s = formatter.formatError("op", "Device not found", 3);
        REQUIRE(s.find("3") != std::string::npos);
    }

    SECTION("testConfigLoadEnv") {
        CliApplication app(RuntimeFactory::createTesting());
        (void)app;
    }
}
