#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <mbootcore/config/ConfigManager.hpp>
#include <mbootcore/telemetry/TelemetryCollector.hpp>
#include <mbootcore/benchmark/BenchmarkTypes.hpp>
#include <mbootcore/benchmark/BenchmarkRunner.hpp>
#include <mbootcore/profiler/Profiler.hpp>
#include <mbootcore/logging/StructuredLogger.hpp>
#include <mbootcore/logging/LoggingTypes.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace mbootcore;
using namespace mbootcore::config;
using namespace mbootcore::telemetry;
using namespace mbootcore::benchmark;
using namespace mbootcore::logging;
using namespace mbootcore::profiler;

// ---------------------------------------------------------------------------
// Benchmark helper (must be at file scope)
// ---------------------------------------------------------------------------

static benchmark::BenchmarkResult dummyBenchReal() {
    benchmark::BenchmarkResult r;
    r.name = "dummy_bench";
    r.durationNs = 1000;
    r.operations = 100;
    r.opsPerSecond = 1e8;
    return r;
}

// ---------------------------------------------------------------------------
// Direct nlohmann/json edge-case tests
// ---------------------------------------------------------------------------

TEST_CASE("JsonDirectTest", "[json][regression]") {

SECTION("unicode parsing") {
    auto j = nlohmann::json::parse(R"({"text": "caf\u00e9 \u4f60\u597d"})");
    REQUIRE(j.is_object());
    REQUIRE(j["text"].is_string());
    std::string val = j["text"];
    REQUIRE(val == "café 你好");
}

SECTION("escaped characters") {
    auto j = nlohmann::json::parse(R"({"s": "line1\nline2\ttab\\backslash\"quote"})");
    REQUIRE(j["s"] == "line1\nline2\ttab\\backslash\"quote");
}

SECTION("nested objects") {
    auto j = nlohmann::json::parse(R"({"a":{"b":{"c":{"d":"deep"}}}})");
    REQUIRE(j["a"]["b"]["c"]["d"] == "deep");
}

SECTION("nested arrays") {
    auto j = nlohmann::json::parse(R"({"arr":[[[1,2],[3,4]],[[5,6]]]})");
    REQUIRE(j["arr"][0][0][0] == 1);
    REQUIRE(j["arr"][1][0][0] == 5);
}

SECTION("array of objects") {
    auto j = nlohmann::json::parse(R"([
        {"id": 1, "name": "first"},
        {"id": 2, "name": "second"}
    ])");
    REQUIRE(j.is_array());
    REQUIRE(j.size() == 2);
    REQUIRE(j[0]["name"] == "first");
    REQUIRE(j[1]["id"] == 2);
}

SECTION("malformed json throws parse_error") {
    REQUIRE_THROWS_AS(nlohmann::json::parse("{invalid}"), nlohmann::json::parse_error);
    REQUIRE_THROWS_AS(nlohmann::json::parse("[1,2,3"), nlohmann::json::parse_error);
    REQUIRE_THROWS_AS(nlohmann::json::parse(""), nlohmann::json::parse_error);
}

SECTION("type mismatch: value() returns default") {
    nlohmann::json j = {{"str", "hello"}, {"num", 42}, {"flag", true}};
    REQUIRE(j.value("str", "default") == "hello");
    REQUIRE(j.value("missing", "default") == "default");
    REQUIRE(j.value("num", uint64_t{0}) == 42);
    REQUIRE(j.value("flag", false) == true);
}

SECTION("null values") {
    auto j = nlohmann::json::parse(R"({"a": null, "b": "text"})");
    REQUIRE(j["a"].is_null());
    REQUIRE(j.value("b", "fallback") == "text");
    // .value() with string default throws on null — must use find()
    auto it = j.find("a");
    REQUIRE(it != j.end());
    REQUIRE(it->is_null());
}

SECTION("large numbers") {
    auto j = nlohmann::json::parse(R"({"big": 18446744073709551615})");
    REQUIRE(j["big"].is_number_unsigned());
    REQUIRE(j["big"].get<uint64_t>() == 18446744073709551615ULL);
}

SECTION("negative numbers") {
    auto j = nlohmann::json::parse(R"({"neg": -42})");
    REQUIRE(j["neg"].is_number_integer());
    REQUIRE(j["neg"].get<int64_t>() == -42);
}

SECTION("floating point") {
    auto j = nlohmann::json::parse(R"({"pi": 3.14159, "sci": 1.5e10})");
    REQUIRE(j["pi"].get<double>() > 3.14);
    REQUIRE(j["sci"].get<double>() > 1.4e10);
}

SECTION("deep nesting") {
    nlohmann::json j = {{"level", 0}};
    auto* p = &j;
    for (int i = 0; i < 100; ++i) {
        (*p)["next"] = {{"level", i + 1}};
        p = &(*p)["next"];
    }
    auto dumped = j.dump();
    auto parsed = nlohmann::json::parse(dumped);
    int lvl = 0;
    auto* cur = &parsed;
    while (cur->contains("next")) {
        REQUIRE((*cur)["level"] == lvl);
        cur = &(*cur)["next"];
        ++lvl;
    }
    REQUIRE(lvl == 100);
    REQUIRE((*cur)["level"] == 100);
}

SECTION("empty object and array") {
    auto obj = nlohmann::json::parse("{}");
    REQUIRE(obj.empty());
    REQUIRE(obj.size() == 0);

    auto arr = nlohmann::json::parse("[]");
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
}

SECTION("round trip") {
    nlohmann::json original = {
        {"name", "test"},
        {"count", 42},
        {"rate", 3.14},
        {"active", true},
        {"tags", {"a", "b", "c"}},
        {"meta", {{"version", 2}, {"author", "me"}}}
    };
    auto serialized = original.dump();
    auto parsed = nlohmann::json::parse(serialized);
    REQUIRE(parsed == original);
}

SECTION("serialization pretty printing") {
    nlohmann::json j = {{"key", "value"}, {"num", 42}};
    auto pretty = j.dump(2);
    nlohmann::json parsed = nlohmann::json::parse(pretty);
    REQUIRE(parsed == j);
}

}

// ---------------------------------------------------------------------------
// ConfigManager JSON round-trip regression tests
// ---------------------------------------------------------------------------

TEST_CASE("ConfigManagerJsonTest", "[json][regression][config]") {

SECTION("export and import round-trip preserves values") {
    ConfigManager mgr;
    RuntimeConfig rc;
    rc.threadPoolSize = 16;
    rc.operationTimeoutMs = 60000;
    rc.enableProfiling = true;
    rc.logLevel = "debug";
    REQUIRE(mgr.setRuntimeConfig(rc).isOk());

    TransportConfig tc;
    tc.usbTimeoutMs = 9999;
    tc.bufferSize = 131072;
    REQUIRE(mgr.setTransportConfig(tc).isOk());

    REQUIRE(mgr.setCustomOption("custom_key", "custom_val").isOk());

    auto exported = mgr.exportJson();
    REQUIRE(exported.isOk());

    ConfigManager mgr2;
    auto importResult = mgr2.importJson(exported.value());
    REQUIRE(importResult.isOk());

    auto rt = mgr2.runtimeConfig();
    REQUIRE(rt.isOk());
    REQUIRE(rt.value().threadPoolSize == 16u);
    REQUIRE(rt.value().operationTimeoutMs == 60000u);
    REQUIRE(rt.value().enableProfiling);
    REQUIRE(rt.value().logLevel == "debug");

    auto tr = mgr2.transportConfig();
    REQUIRE(tr.isOk());
    REQUIRE(tr.value().usbTimeoutMs == 9999u);

    auto co = mgr2.customOption("custom_key");
    REQUIRE(co.isOk());
    REQUIRE(co.value() == "custom_val");
}

SECTION("export with unicode values") {
    ConfigManager mgr;
    REQUIRE(mgr.setCustomOption("label", "café 日本語").isOk());
    REQUIRE(mgr.setCustomOption("emoji", "🔥 🚀").isOk());

    auto exported = mgr.exportJson();
    REQUIRE(exported.isOk());
    REQUIRE(exported.value().find("café") != std::string::npos);
    REQUIRE(exported.value().find("日本語") != std::string::npos);
    REQUIRE(exported.value().find("🔥") != std::string::npos);

    ConfigManager mgr2;
    REQUIRE(mgr2.importJson(exported.value()).isOk());
    auto label = mgr2.customOption("label");
    REQUIRE(label.isOk());
    REQUIRE(label.value() == "café 日本語");
}

SECTION("import malformed json returns error") {
    ConfigManager mgr;
    auto result = mgr.importJson("{invalid json here");
    REQUIRE(result.isError());
}

SECTION("import empty json returns error") {
    ConfigManager mgr;
    auto result = mgr.importJson("");
    REQUIRE(result.isError());
}

SECTION("import type-mismatched json handled gracefully") {
    // parseConfigFromJson silently skips non-object sections — import succeeds
    ConfigManager mgr;
    auto result = mgr.importJson(R"({"runtime": "not_an_object"})");
    REQUIRE(result.isOk());
}

}

// ---------------------------------------------------------------------------
// TelemetryCollector JSON export regression tests
// ---------------------------------------------------------------------------

TEST_CASE("TelemetryJsonTest", "[json][regression][telemetry]") {

SECTION("exportJson contains valid json") {
    TelemetryCollector c;
    c.enable();
    c.setSessionId("sess_001");

    TelemetryEvent ev;
    ev.id = "ev1";
    ev.name = "test_event";
    c.recordEvent(ev);

    auto json = c.exportJson();
    REQUIRE(json.isOk());
    REQUIRE(json.value().find("sessionId") != std::string::npos);
    REQUIRE(json.value().find("sess_001") != std::string::npos);
}

SECTION("exportJson with unicode data") {
    TelemetryCollector c;
    c.enable();

    TelemetryEvent ev;
    ev.id = "unicode_test";
    ev.name = "café 日本語 test";
    ev.data["label"] = "🔥🚀";
    c.recordEvent(ev);

    auto json = c.exportJson();
    REQUIRE(json.isOk());

    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed["sessionId"].is_string());
}

SECTION("export with special characters in event names") {
    TelemetryCollector c;
    c.enable();

    TelemetryEvent ev;
    ev.id = "esc_test";
    ev.name = "line1\nline2\ttab";
    c.recordEvent(ev);

    auto json = c.exportJson();
    REQUIRE(json.isOk());
    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed.contains("events"));
}

SECTION("empty collector export") {
    TelemetryCollector c;
    c.enable();
    auto json = c.exportJson();
    REQUIRE(json.isOk());
    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed.contains("events"));
}

}

// ---------------------------------------------------------------------------
// BenchmarkRunner JSON export regression tests
// ---------------------------------------------------------------------------

TEST_CASE("BenchmarkJsonTest", "[json][regression][benchmark]") {

SECTION("exportJson produces valid json") {
    BenchmarkRunner runner;
    runner.registerBenchmark("dummy", []() -> Result<benchmark::BenchmarkResult> {
        return dummyBenchReal();
    });
    REQUIRE(runner.runAll().isOk());

    auto json = runner.exportJson();
    REQUIRE(json.isOk());
    REQUIRE(!json.value().empty());

    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed["suite"] == "MBootCore Benchmark Suite");
}

SECTION("empty benchmark export") {
    BenchmarkRunner runner;
    auto json = runner.exportJson();
    REQUIRE(json.isOk());
    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed["results"].empty());
}

}

// ---------------------------------------------------------------------------
// File-based JSON persistence regression tests
// ---------------------------------------------------------------------------

TEST_CASE("JsonFilePersistenceTest", "[json][regression][persistence]") {

SECTION("round-trip through temp file") {
    auto tmpDir = fs::temp_directory_path() / "mboot_json_test";
    fs::create_directories(tmpDir);
    auto jsonPath = (tmpDir / "test_config.json").string();

    ConfigManager mgr;
    REQUIRE(mgr.setConfigPath(jsonPath).isOk());

    RuntimeConfig rc;
    rc.threadPoolSize = 32;
    rc.logLevel = "verbose";
    REQUIRE(mgr.setRuntimeConfig(rc).isOk());

    FullConfig cfg;
    cfg.runtime = rc;
    REQUIRE(mgr.save(cfg).isOk());

    ConfigManager mgr2;
    REQUIRE(mgr2.setConfigPath(jsonPath).isOk());
    auto loaded = mgr2.load();
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().runtime.threadPoolSize == 32u);

    fs::remove_all(tmpDir);
}

SECTION("save and load with unicode config values") {
    auto tmpDir = fs::temp_directory_path() / "mboot_json_uni";
    fs::create_directories(tmpDir);
    auto jsonPath = (tmpDir / "unicode_config.json").string();

    ConfigManager mgr;
    REQUIRE(mgr.setConfigPath(jsonPath).isOk());

    FullConfig cfg;
    cfg.customOptions["name"] = "Über-Config テスト";
    REQUIRE(mgr.save(cfg).isOk());

    ConfigManager mgr2;
    REQUIRE(mgr2.setConfigPath(jsonPath).isOk());
    auto loaded = mgr2.load();
    REQUIRE(loaded.isOk());

    auto it = loaded.value().customOptions.find("name");
    REQUIRE(it != loaded.value().customOptions.end());
    REQUIRE(it->second == "Über-Config テスト");

    fs::remove_all(tmpDir);
}

SECTION("load from corrupted json returns error") {
    auto tmpDir = fs::temp_directory_path() / "mboot_json_corrupt";
    fs::create_directories(tmpDir);
    auto jsonPath = (tmpDir / "corrupt.json").string();

    std::ofstream(jsonPath) << R"({corrupted json: [})";

    ConfigManager mgr;
    REQUIRE(mgr.setConfigPath(jsonPath).isOk());
    auto result = mgr.load();
    REQUIRE(result.isError());

    fs::remove_all(tmpDir);
}

SECTION("empty file load returns error") {
    auto tmpDir = fs::temp_directory_path() / "mboot_json_empty";
    fs::create_directories(tmpDir);
    auto jsonPath = (tmpDir / "empty.json").string();

    std::ofstream(jsonPath) << "";

    ConfigManager mgr;
    REQUIRE(mgr.setConfigPath(jsonPath).isOk());
    auto result = mgr.load();
    REQUIRE(result.isError());

    fs::remove_all(tmpDir);
}

}

// ---------------------------------------------------------------------------
// StructuredLogger JSON correctness regression tests
// ---------------------------------------------------------------------------

TEST_CASE("StructuredLoggerJsonTest", "[json][regression][logging]") {

SECTION("exportJson is valid json") {
    StructuredLogger logger;
    logger.setSessionId("sess_json_test");
    logger.log(LogLevel::Info, LogCategory::General, "test message");

    auto json = logger.exportJson();
    REQUIRE(json.isOk());

    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed.is_array());
    REQUIRE(parsed.size() >= 1);
    REQUIRE(parsed[0].contains("message"));
    REQUIRE(parsed[0]["message"] == "test message");
}

SECTION("exportJson with unicode content") {
    StructuredLogger logger;
    logger.setSessionId("sess_uni");
    logger.log(LogLevel::Info, LogCategory::General, "café 日本語 🔥");

    auto json = logger.exportJson();
    REQUIRE(json.isOk());
    auto parsed = nlohmann::json::parse(json.value());
    REQUIRE(parsed.is_array());
    REQUIRE(parsed[0].contains("message"));
}

}

// ---------------------------------------------------------------------------
// Profiler JSON export regression tests
// ---------------------------------------------------------------------------

TEST_CASE("ProfilerJsonTest", "[json][regression][profiler]") {

SECTION("exportJson is valid json or empty") {
    Profiler profiler;
    profiler.begin("test_frame");
    profiler.end("test_frame");

    auto json = profiler.exportJson();
    if (json.isOk() && !json.value().empty()) {
        REQUIRE_NOTHROW(nlohmann::json::parse(json.value()));
    }
}

}
