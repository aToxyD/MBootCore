#include <catch2/catch_test_macros.hpp>

#include <mbootcore/telemetry/TelemetryTypes.hpp>
#include <mbootcore/telemetry/TelemetryCollector.hpp>

namespace t = mbootcore::telemetry;

t::TelemetryCollector createEnabledCollector() {
    t::TelemetryCollector c;
    c.enable();
    return c;
}

TEST_CASE("TelemetryTypesTest", "[telemetry]") {
    SECTION("testCategoryEnumValues") {
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::Performance) == 0u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::Error) == 1u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::Crash) == 2u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::Statistics) == 3u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::Usage) == 4u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryCategory::System) == 5u);
    }

    SECTION("testStateEnumValues") {
        REQUIRE(static_cast<uint32_t>(t::TelemetryState::Disabled) == 0u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryState::Enabled) == 1u);
        REQUIRE(static_cast<uint32_t>(t::TelemetryState::Paused) == 2u);
    }

    SECTION("testEventStructDefaults") {
        t::TelemetryEvent ev;
        REQUIRE(ev.category == t::TelemetryCategory::Usage);
        REQUIRE(ev.id.empty());
        REQUIRE(ev.name.empty());
        REQUIRE(ev.data.empty());
    }

    SECTION("testReportStructDefaults") {
        t::TelemetryReport r;
        REQUIRE(r.sessionId.empty());
        REQUIRE(r.events.empty());
        REQUIRE(r.eventCounts.empty());
    }
}

TEST_CASE("TelemetryCollectorTest", "[telemetry]") {
    SECTION("testInitiallyDisabled") {
        t::TelemetryCollector c;
        REQUIRE(c.state() == t::TelemetryState::Disabled);
    }

    SECTION("testEnableDisableStateTransition") {
        t::TelemetryCollector c;
        REQUIRE(c.enable().isOk());
        REQUIRE(c.state() == t::TelemetryState::Enabled);
        REQUIRE(c.disable().isOk());
        REQUIRE(c.state() == t::TelemetryState::Disabled);
    }

    SECTION("testPauseResumeStateTransition") {
        t::TelemetryCollector c;
        c.enable();
        REQUIRE(c.pause().isOk());
        REQUIRE(c.state() == t::TelemetryState::Paused);
        REQUIRE(c.resume().isOk());
        REQUIRE(c.state() == t::TelemetryState::Enabled);
    }

    SECTION("testPauseWhenDisabledDoesNothing") {
        t::TelemetryCollector c;
        REQUIRE(c.pause().isOk());
        REQUIRE(c.state() == t::TelemetryState::Disabled);
    }

    SECTION("testRecordEventWhenDisabledFails") {
        t::TelemetryCollector c;
        t::TelemetryEvent ev;
        auto r = c.recordEvent(ev);
        REQUIRE(r.isError());
    }

    SECTION("testRecordEventWhenEnabledSucceeds") {
        auto c = createEnabledCollector();
        t::TelemetryEvent ev;
        ev.id = "test_event";
        ev.name = "test";
        auto r = c.recordEvent(ev);
        REQUIRE(r.isOk());
    }

    SECTION("testRecordEventWhenPausedFails") {
        t::TelemetryCollector c;
        c.enable();
        c.pause();
        t::TelemetryEvent ev;
        auto r = c.recordEvent(ev);
        REQUIRE(r.isError());
    }

    SECTION("testRecordMetric") {
        auto c = createEnabledCollector();
        auto r = c.recordMetric("cpu_temp", "75");
        REQUIRE(r.isOk());
        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{1});
        REQUIRE(report.value().events[0].category == t::TelemetryCategory::Statistics);
    }

    SECTION("testRecordError") {
        auto c = createEnabledCollector();
        auto r = c.recordError("usb", "timeout");
        REQUIRE(r.isOk());
        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{1});
        REQUIRE(report.value().events[0].category == t::TelemetryCategory::Error);
    }

    SECTION("testRecordCrash") {
        auto c = createEnabledCollector();
        auto r = c.recordCrash("loader", "segfault at 0x0");
        REQUIRE(r.isOk());
        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{1});
        REQUIRE(report.value().events[0].category == t::TelemetryCategory::Crash);
    }

    SECTION("testGenerateReportWithEventCounts") {
        auto c = createEnabledCollector();
        t::TelemetryEvent ev1;
        ev1.name = "click";
        t::TelemetryEvent ev2;
        ev2.name = "click";
        t::TelemetryEvent ev3;
        ev3.name = "view";

        c.recordEvent(ev1);
        c.recordEvent(ev2);
        c.recordEvent(ev3);

        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{3});
        REQUIRE(report.value().eventCounts["click"] == uint64_t{2});
        REQUIRE(report.value().eventCounts["view"] == uint64_t{1});
    }

    SECTION("testExportJson") {
        auto c = createEnabledCollector();
        c.setSessionId("sess_001");
        t::TelemetryEvent ev;
        ev.id = "ev1";
        ev.name = "test";
        c.recordEvent(ev);

        auto json = c.exportJson();
        REQUIRE(json.isOk());
        REQUIRE(json.value().find("\"sessionId\"") != std::string::npos);
        REQUIRE(json.value().find("sess_001") != std::string::npos);
        REQUIRE(json.value().find("\"events\"") != std::string::npos);
        REQUIRE(json.value().find("\"eventCounts\"") != std::string::npos);
    }

    SECTION("testClear") {
        auto c = createEnabledCollector();
        t::TelemetryEvent ev;
        ev.name = "test";
        c.recordEvent(ev);
        REQUIRE(c.clear().isOk());
        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{0});
    }

    SECTION("testSessionId") {
        t::TelemetryCollector c;
        REQUIRE(c.sessionId().empty());
        c.setSessionId("sess_xyz");
        REQUIRE(c.sessionId() == "sess_xyz");
    }

    SECTION("testMultipleEventsInReport") {
        auto c = createEnabledCollector();
        for (int i = 0; i < 5; ++i) {
            t::TelemetryEvent ev;
            ev.id = "ev" + std::to_string(i);
            ev.name = (i % 2 == 0) ? "even" : "odd";
            c.recordEvent(ev);
        }

        auto report = c.generateReport();
        REQUIRE(report.isOk());
        REQUIRE(report.value().events.size() == size_t{5});
        REQUIRE(report.value().eventCounts["even"] == uint64_t{3});
        REQUIRE(report.value().eventCounts["odd"] == uint64_t{2});
    }
}
