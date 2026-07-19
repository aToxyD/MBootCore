#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include <mbootcore/logging/LoggingTypes.hpp>
#include <mbootcore/logging/StructuredLogger.hpp>

namespace l = mbootcore::logging;

void logAllLevels(l::StructuredLogger& logger) {
    logger.trace("trace msg");
    logger.debug("debug msg");
    logger.info("info msg");
    logger.warning("warning msg");
    logger.error("error msg");
    logger.fatal("fatal msg");
}

TEST_CASE("LoggingTypesTest", "[logging]") {
    SECTION("testLogLevelEnumValues") {
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Trace) == 0u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Debug) == 1u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Info) == 2u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Warning) == 3u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Error) == 4u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::Fatal) == 5u);
        REQUIRE(static_cast<uint32_t>(l::LogLevel::None) == 6u);
    }

    SECTION("testLogFormatEnumValues") {
        REQUIRE(static_cast<uint32_t>(l::LogFormat::Text) == 0u);
        REQUIRE(static_cast<uint32_t>(l::LogFormat::JSON) == 1u);
    }

    SECTION("testLogCategoryEnumValues") {
        REQUIRE(static_cast<uint32_t>(l::LogCategory::General) == 0u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Transport) == 1u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Protocol) == 2u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Pipeline) == 3u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Workflow) == 4u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Job) == 5u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Plugin) == 6u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::DSP) == 7u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Security) == 8u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Config) == 9u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Diagnostics) == 10u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Profiler) == 11u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Memory) == 12u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Session) == 13u);
        REQUIRE(static_cast<uint32_t>(l::LogCategory::Device) == 14u);
    }

    SECTION("testLogEntryDefaults") {
        l::LogEntry e;
        REQUIRE(e.level == l::LogLevel::Info);
        REQUIRE(e.category == l::LogCategory::General);
        REQUIRE(e.message.empty());
        REQUIRE(e.scope.empty());
        REQUIRE(e.threadId.empty());
        REQUIRE(e.sessionId.empty());
        REQUIRE(e.lineNumber == 0u);
    }

    SECTION("testLogRotationConfigDefaults") {
        l::LogRotationConfig cfg;
        REQUIRE(cfg.enabled);
        REQUIRE(cfg.maxSizeBytes == uint64_t{10 * 1024 * 1024});
        REQUIRE(cfg.maxFiles == uint32_t{5});
        REQUIRE(!cfg.compression);
    }
}

TEST_CASE("StructuredLoggerTest", "[logging]") {
    SECTION("testDefaultLevel") {
        l::StructuredLogger logger;
        REQUIRE(logger.level() == l::LogLevel::Trace);
    }

    SECTION("testSetLevel") {
        l::StructuredLogger logger;
        REQUIRE(logger.setLevel(l::LogLevel::Warning).isOk());
        REQUIRE(logger.level() == l::LogLevel::Warning);
    }

    SECTION("testDefaultFormat") {
        l::StructuredLogger logger;
        REQUIRE(logger.format() == l::LogFormat::Text);
    }

    SECTION("testSetFormat") {
        l::StructuredLogger logger;
        REQUIRE(logger.setFormat(l::LogFormat::JSON).isOk());
        REQUIRE(logger.format() == l::LogFormat::JSON);
    }

    SECTION("testLogMessageAtAllLevels") {
        l::StructuredLogger logger;
        logAllLevels(logger);

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{6});
    }

    SECTION("testLevelFiltering") {
        l::StructuredLogger logger;
        logger.setLevel(l::LogLevel::Warning);
        logAllLevels(logger);

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{3});

        for (const auto& e : all.value()) {
            REQUIRE(e.level >= l::LogLevel::Warning);
        }
    }

    SECTION("testLogNoneFiltersEverything") {
        l::StructuredLogger logger;
        logger.setLevel(l::LogLevel::None);
        logAllLevels(logger);

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{0});
    }

    SECTION("testConvenienceMethods") {
        l::StructuredLogger logger;

        logger.trace("t");
        logger.debug("d");
        logger.info("i");
        logger.warning("w");
        logger.error("e");
        logger.fatal("f");

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{6});
    }

    SECTION("testCategoryParameter") {
        l::StructuredLogger logger;
        logger.info("transport msg", l::LogCategory::Transport);
        logger.error("protocol error", l::LogCategory::Protocol);

        auto transport = logger.entriesByCategory(l::LogCategory::Transport);
        REQUIRE(transport.isOk());
        REQUIRE(transport.value().size() == size_t{1});
        REQUIRE(transport.value()[0].message == "transport msg");

        auto protocol = logger.entriesByCategory(l::LogCategory::Protocol);
        REQUIRE(protocol.isOk());
        REQUIRE(protocol.value().size() == size_t{1});
        REQUIRE(protocol.value()[0].message == "protocol error");
    }

    SECTION("testEntriesByLevel") {
        l::StructuredLogger logger;
        logAllLevels(logger);

        auto warnings = logger.entries(l::LogLevel::Warning);
        REQUIRE(warnings.isOk());
        REQUIRE(warnings.value().size() == size_t{3});

        auto errors = logger.entries(l::LogLevel::Error);
        REQUIRE(errors.isOk());
        REQUIRE(errors.value().size() == size_t{2});
    }

    SECTION("testSessionId") {
        l::StructuredLogger logger;
        logger.setSessionId("sess_001");
        logger.info("test");

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value()[0].sessionId == "sess_001");
    }

    SECTION("testDeviceId") {
        l::StructuredLogger logger;
        logger.setDeviceId("dev_abc");
        logger.info("test");

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value()[0].deviceId == "dev_abc");
    }

    SECTION("testWorkflowId") {
        l::StructuredLogger logger;
        logger.setWorkflowId("wf_xyz");
        logger.info("test");

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value()[0].workflowId == "wf_xyz");
    }

    SECTION("testEntriesBySession") {
        l::StructuredLogger logger;
        logger.setSessionId("s1");
        logger.info("msg1");
        logger.setSessionId("s2");
        logger.info("msg2");
        logger.setSessionId("s1");
        logger.info("msg3");

        auto s1 = logger.entriesBySession("s1");
        REQUIRE(s1.isOk());
        REQUIRE(s1.value().size() == size_t{2});

        auto s2 = logger.entriesBySession("s2");
        REQUIRE(s2.isOk());
        REQUIRE(s2.value().size() == size_t{1});
    }

    SECTION("testFilters") {
        l::StructuredLogger logger;
        logger.setLevel(l::LogLevel::None);
        logger.addFilter(l::LogLevel::Info, l::LogCategory::Transport);

        logger.info("should not appear", l::LogCategory::General);
        logger.info("should appear", l::LogCategory::Transport);
        logger.warning("also appears", l::LogCategory::Transport);

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{2});
        REQUIRE(all.value()[0].message == "should appear");
    }

    SECTION("testClearFilters") {
        l::StructuredLogger logger;
        logger.setLevel(l::LogLevel::None);
        logger.addFilter(l::LogLevel::Info, l::LogCategory::Transport);
        logger.clearFilters();

        logger.info("should not appear", l::LogCategory::Transport);
        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{0});
    }

    SECTION("testClear") {
        l::StructuredLogger logger;
        logger.info("msg");
        REQUIRE(logger.clear().isOk());
        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{0});
    }

    SECTION("testExportText") {
        l::StructuredLogger logger;
        logger.info("hello world");

        auto text = logger.exportText();
        REQUIRE(text.isOk());
        REQUIRE(text.value().find("INFO") != std::string::npos);
        REQUIRE(text.value().find("hello world") != std::string::npos);
    }

    SECTION("testExportJson") {
        l::StructuredLogger logger;
        logger.setSessionId("sess_json");
        logger.info("json test");

        auto json = logger.exportJson();
        REQUIRE(json.isOk());
        REQUIRE(json.value().find("\"level\": \"INFO\"") != std::string::npos);
        REQUIRE(json.value().find("json test") != std::string::npos);
        REQUIRE(json.value().find("sess_json") != std::string::npos);
    }

    SECTION("testRotationConfig") {
        l::StructuredLogger logger;
        l::LogRotationConfig cfg;
        cfg.enabled = true;
        cfg.maxSizeBytes = 1024;
        cfg.maxFiles = 3;
        cfg.compression = true;

        REQUIRE(logger.setRotation(cfg).isOk());
    }

    SECTION("testFileOutput") {
        auto tmpDir = std::filesystem::temp_directory_path();
        auto logPath = tmpDir / "mboot_test_file_log.txt";
        std::string filePath = logPath.string();

        {
            l::StructuredLogger logger;
            auto r = logger.setOutput(filePath);
            REQUIRE(r.isOk());

            logger.info("file log message");
            logger.flush();
        }

        REQUIRE(std::filesystem::exists(logPath));
        std::ifstream file(filePath);
        REQUIRE(file.is_open());
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();

        REQUIRE(content.find("file log message") != std::string::npos);

        std::filesystem::remove(logPath);
    }

    SECTION("testFileOutputJsonFormat") {
        auto tmpDir = std::filesystem::temp_directory_path();
        auto logPath = tmpDir / "mboot_test_json_log.json";
        std::string filePath = logPath.string();

        {
            l::StructuredLogger logger;
            logger.setFormat(l::LogFormat::JSON);
            auto r = logger.setOutput(filePath);
            REQUIRE(r.isOk());

            logger.info("json file test");
            logger.flush();
        }

        REQUIRE(std::filesystem::exists(logPath));
        std::ifstream file(filePath);
        REQUIRE(file.is_open());
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();

        REQUIRE(content.find("json file test") != std::string::npos);
        REQUIRE(content.find("\"level\":\"INFO\"") != std::string::npos);

        std::filesystem::remove(logPath);
    }

    SECTION("testMetadataInEntry") {
        l::StructuredLogger logger;
        logger.setSessionId("sid");
        logger.setDeviceId("did");
        logger.setWorkflowId("wid");
        logger.info("metadata test", l::LogCategory::Pipeline);

        auto all = logger.entries();
        REQUIRE(all.isOk());
        REQUIRE(all.value().size() == size_t{1});

        const auto& e = all.value()[0];
        REQUIRE(e.sessionId == "sid");
        REQUIRE(e.deviceId == "did");
        REQUIRE(e.workflowId == "wid");
        REQUIRE(e.category == l::LogCategory::Pipeline);
        REQUIRE(e.level == l::LogLevel::Info);
    }
}
