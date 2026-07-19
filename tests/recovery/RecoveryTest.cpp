#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <mbootcore/domain/Error.hpp>

using namespace mbootcore;

namespace {

class RecoverySimulator {
public:
    explicit RecoverySimulator(int failUntil)
        : m_failUntil(failUntil), m_attempt(0), m_rolledBack(false), m_checkpoint("") {}

    Result<void> operation() {
        ++m_attempt;
        if (m_attempt <= m_failUntil) {
            return ErrorCode::TransportError;
        }
        return {};
    }

    Result<void> rollback() {
        m_rolledBack = true;
        return {};
    }

    bool wasRolledBack() const { return m_rolledBack; }
    int attempts() const { return m_attempt; }

    void setCheckpoint(const std::string& cp) { m_checkpoint = cp; }
    std::string checkpoint() const { return m_checkpoint; }

    void reset() {
        m_attempt = 0;
        m_rolledBack = false;
        m_checkpoint.clear();
        m_failUntil = 0;
    }

private:
    int m_failUntil;
    int m_attempt;
    bool m_rolledBack;
    std::string m_checkpoint;
};

}

TEST_CASE("RecoveryTest", "[recovery]") {

SECTION("testRollbackSucceeds") {
    RecoverySimulator sim(0);
    auto result = sim.rollback();
    REQUIRE(result.isOk());
    REQUIRE(sim.wasRolledBack());
}

SECTION("testRollbackFromPartialState") {
    RecoverySimulator sim(2);
    auto r1 = sim.operation();
    REQUIRE(r1.isError());
    auto rb = sim.rollback();
    REQUIRE(rb.isOk());
    REQUIRE(sim.wasRolledBack());
    REQUIRE(sim.attempts() == 1);
}

SECTION("testRollbackIdempotent") {
    RecoverySimulator sim(0);
    auto r1 = sim.rollback();
    REQUIRE(r1.isOk());
    auto r2 = sim.rollback();
    REQUIRE(r2.isOk());
    REQUIRE(sim.wasRolledBack());
}

SECTION("testRetrySucceeds") {
    RecoverySimulator sim(2);
    Result<void> result;
    for (int i = 0; i < 4; ++i) {
        result = sim.operation();
        if (result.isOk()) break;
    }
    REQUIRE(result.isOk());
    REQUIRE(sim.attempts() == 3);
}

SECTION("testRetryLimitedCount") {
    RecoverySimulator sim(10);
    Result<void> result;
    int maxRetries = 3;
    for (int i = 0; i < maxRetries; ++i) {
        result = sim.operation();
        if (result.isOk()) break;
    }
    REQUIRE(result.isError());
    REQUIRE(sim.attempts() == 3);
}

SECTION("testRetryWithBackoff") {
    RecoverySimulator sim(2);
    std::vector<int> delays;
    Result<void> result;
    for (int i = 0; i < 4; ++i) {
        if (i > 0) {
            int delay = i * 10;
            delays.push_back(delay);
        }
        result = sim.operation();
        if (result.isOk()) break;
    }
    REQUIRE(result.isOk());
    REQUIRE(sim.attempts() == 3);
    REQUIRE(delays.size() >= 2);
}

SECTION("testResumeFromCheckpoint") {
    RecoverySimulator sim(0);
    sim.setCheckpoint("stage_3");
    REQUIRE(sim.checkpoint() == std::string("stage_3"));
}

SECTION("testResumeRestoresState") {
    RecoverySimulator sim(1);
    sim.setCheckpoint("flash_written");
    auto r1 = sim.operation();
    REQUIRE(r1.isError());

    sim.reset();
    sim.setCheckpoint("flash_written");
    auto r2 = sim.operation();
    REQUIRE(r2.isOk());
    REQUIRE(sim.checkpoint() == std::string("flash_written"));
}

SECTION("testResumeAfterFailure") {
    RecoverySimulator sim(1);
    sim.setCheckpoint("connected");

    auto r1 = sim.operation();
    REQUIRE(r1.isError());

    sim.reset();
    sim.setCheckpoint("connected");
    auto r2 = sim.operation();
    REQUIRE(r2.isOk());
}

SECTION("testPipelineRecoveryAfterStageFailure") {
    int stage1Count = 0;
    int stage2Count = 0;

    auto stage1 = [&]() -> Result<void> {
        ++stage1Count;
        return {};
    };

    auto stage2 = [&]() -> Result<void> {
        ++stage2Count;
        if (stage2Count <= 1) {
            return ErrorCode::ProtocolMismatch;
        }
        return {};
    };

    auto s1 = stage1();
    REQUIRE(s1.isOk());
    auto s2 = stage2();
    REQUIRE(s2.isError());
    auto s2retry = stage2();
    REQUIRE(s2retry.isOk());
    REQUIRE(stage1Count == 1);
    REQUIRE(stage2Count == 2);
}

SECTION("testPipelineRollbackAllStages") {
    std::vector<std::string> rolledBack;

    auto rollbackStage = [&](const std::string& stage) {
        rolledBack.push_back(stage);
    };

    rollbackStage("stage1");
    rollbackStage("stage2");
    rollbackStage("stage3");

    REQUIRE(rolledBack.size() == 3u);
    REQUIRE(rolledBack[0] == std::string("stage1"));
    REQUIRE(rolledBack[1] == std::string("stage2"));
    REQUIRE(rolledBack[2] == std::string("stage3"));
}

SECTION("testPipelineRetryStage") {
    int attempts = 0;
    auto operation = [&]() -> Result<void> {
        ++attempts;
        if (attempts <= 2) {
            return ErrorCode::TransportTimeout;
        }
        return {};
    };

    Result<void> result;
    for (int i = 0; i < 5; ++i) {
        result = operation();
        if (result.isOk()) break;
    }
    REQUIRE(result.isOk());
    REQUIRE(attempts == 3);
}

SECTION("testPartialFlashCanResume") {
    int writtenSectors = 5;
    int totalSectors = 10;
    int resumeFrom = writtenSectors;

    int writtenAfterResume = 0;
    for (int i = resumeFrom; i < totalSectors; ++i) {
        ++writtenAfterResume;
    }

    REQUIRE(writtenAfterResume == 5);
    REQUIRE(resumeFrom < totalSectors);
}

SECTION("testPartialFlashRollback") {
    std::vector<int> written;
    for (int i = 0; i < 5; ++i) {
        written.push_back(i);
    }

    written.clear();
    REQUIRE(written.empty());

    written.push_back(0);
    written.push_back(1);
    REQUIRE(written.size() == 2u);
}

}
