#include <catch2/catch_test_macros.hpp>

#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/pipeline/BootPipelineConfig.hpp>
#include <mbootcore/pipeline/BootPipelineFactory.hpp>
#include <mbootcore/pipeline/RecoveryStrategy.hpp>
#include <mbootcore/domain/Error.hpp>

#include "MockTransport.hpp"
#include "MockLogger.hpp"

#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

using namespace mbootcore;
using namespace mbootcore::pipeline;

namespace {
MockTransport mockTransport;
MockLogger mockLogger;
}

TEST_CASE("BootPipelineTest", "[pipeline]") {
    mockTransport.setAutoOpen(true);

SECTION("testAllStagesComplete") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    int stageCount = 0;

    auto makeHandler = [&]() {
        return [&](BootContext&) -> Result<void> {
            ++stageCount;
            return {};
        };
    };

    pipeline.setHandler(PipelineStage::Connected, makeHandler());
    pipeline.setHandler(PipelineStage::SaharaHandshake, makeHandler());
    pipeline.setHandler(PipelineStage::VersionNegotiation, makeHandler());
    pipeline.setHandler(PipelineStage::DeviceDiscovery, makeHandler());
    pipeline.setHandler(PipelineStage::LoaderSelection, makeHandler());
    pipeline.setHandler(PipelineStage::ElfParsing, makeHandler());
    pipeline.setHandler(PipelineStage::MemoryImageBuild, makeHandler());
    pipeline.setHandler(PipelineStage::ProgrammerUpload, makeHandler());
    pipeline.setHandler(PipelineStage::ProgrammerExecute, makeHandler());
    pipeline.setHandler(PipelineStage::FirehoseDetection, makeHandler());
    pipeline.setHandler(PipelineStage::FirehoseConfiguration, makeHandler());

    auto result = pipeline.run();
    REQUIRE(result.isOk());
    REQUIRE(pipeline.currentStage() == PipelineStage::Ready);
    REQUIRE(stageCount == 11);
}

SECTION("testPartialHandlersCompletes") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    pipeline.setHandler(PipelineStage::SaharaHandshake, [](BootContext&) -> Result<void> {
        return {};
    });

    auto result = pipeline.run();
    REQUIRE(result.isOk());
    REQUIRE(pipeline.currentStage() == PipelineStage::Ready);
}

SECTION("testStageFailureStopsPipeline") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    pipeline.setHandler(PipelineStage::Connected, [](BootContext&) -> Result<void> {
        return {};
    });
    pipeline.setHandler(PipelineStage::SaharaHandshake, [](BootContext&) -> Result<void> {
        return ErrorCode::ProtocolMismatch;
    });

    auto result = pipeline.run();
    REQUIRE(result.isError());
    REQUIRE(pipeline.currentStage() == PipelineStage::Error);
    REQUIRE(pipeline.lastError() == ErrorCode::ProtocolMismatch);
}

SECTION("testFailureWithRecoveryDisabled") {
    BootPipeline pipeline;
    auto cfg = BootPipelineConfig::defaults();
    cfg.enableRecovery = false;
    pipeline.setConfig(cfg);
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    pipeline.setHandler(PipelineStage::SaharaHandshake, [](BootContext&) -> Result<void> {
        return ErrorCode::TransportError;
    });

    auto result = pipeline.run();
    REQUIRE(result.isError());
    REQUIRE(pipeline.currentStage() == PipelineStage::Error);
}

SECTION("testRetryAfterFailure") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    std::atomic<int> attemptCount{0};

    pipeline.setHandler(PipelineStage::SaharaHandshake, [&](BootContext&) -> Result<void> {
        int attempt = ++attemptCount;
        if (attempt <= 2) {
            return ErrorCode::TransportError;
        }
        return {};
    });

    auto result = pipeline.run();
    REQUIRE(result.isOk());
    REQUIRE(pipeline.currentStage() == PipelineStage::Ready);
    REQUIRE(static_cast<int>(attemptCount.load()) == 3);
}

SECTION("testRecoveryExhaustsRetries") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    std::atomic<int> attemptCount{0};

    pipeline.setHandler(PipelineStage::SaharaHandshake, [&](BootContext&) -> Result<void> {
        ++attemptCount;
        return ErrorCode::TransportError;
    });

    auto result = pipeline.run();
    REQUIRE(result.isError());
    REQUIRE(pipeline.currentStage() == PipelineStage::Error);
    REQUIRE(attemptCount.load() > 2);
}

SECTION("testCustomRecoveryStrategy") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    auto strategy = std::make_shared<RecoveryStrategy>();
    strategy->setDefaultRule(RecoveryAction::Abort);
    strategy->setRuleForStage(PipelineStage::SaharaHandshake, {
        PipelineStage::SaharaHandshake, 1, RecoveryAction::Retry, RecoveryAction::Abort
    });
    pipeline.setRecoveryStrategy(strategy);

    std::atomic<int> count{0};

    pipeline.setHandler(PipelineStage::SaharaHandshake, [&](BootContext&) -> Result<void> {
        ++count;
        return ErrorCode::TransportError;
    });

    auto result = pipeline.run();
    REQUIRE(result.isError());
    REQUIRE(static_cast<int>(count.load()) == 2);
}

SECTION("testCancelBeforeRun") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    pipeline.cancel();
    auto result = pipeline.run();
    REQUIRE(result.isError());
    REQUIRE(pipeline.currentStage() == PipelineStage::Cancelled);
}

SECTION("testProgressCallback") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    int callbackCount = 0;

    pipeline.setProgressCallback([&](const BootContext&) {
        ++callbackCount;
    });

    pipeline.setHandler(PipelineStage::Connected, [](BootContext&) -> Result<void> {
        return {};
    });

    auto result = pipeline.run();
    REQUIRE(result.isOk());
    REQUIRE(callbackCount > 0);
}

SECTION("testContextSharedBetweenStages") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    bool serialVerified = false;

    pipeline.setHandler(PipelineStage::SaharaHandshake, [](BootContext& ctx) -> Result<void> {
        ctx.saharaDeviceInfo.version.major = 3;
        ctx.saharaDeviceInfo.serialNumber = 0xDEAD;
        return {};
    });
    pipeline.setHandler(PipelineStage::FirehoseConfiguration, [&](BootContext& ctx) -> Result<void> {
        serialVerified = (ctx.saharaDeviceInfo.serialNumber == 0xDEAD);
        ctx.flashConfig.memoryName = "ufs";
        return {};
    });

    auto result = pipeline.run();
    REQUIRE(result.isOk());
    REQUIRE(serialVerified);
    REQUIRE(pipeline.context().flashConfig.memoryName == std::string("ufs"));
}

SECTION("testCustomConfig") {
    auto cfg = BootPipelineConfig::defaults();
    cfg.connectTimeoutMs = 10000;
    cfg.maxStageRetries = 5;

    BootPipeline pipeline;
    pipeline.setConfig(cfg);

    REQUIRE(pipeline.config().connectTimeoutMs == 10000);
    REQUIRE(pipeline.config().maxStageRetries == 5);
}

SECTION("testFactoryCreate") {
    auto pipeline = BootPipelineFactory::createWithTransport(&mockTransport);
    REQUIRE(pipeline != nullptr);
    REQUIRE(pipeline->context().transport != nullptr);
}

SECTION("testFactoryWithBuilder") {
    bool connected = false;
    auto pipeline = BootPipelineFactory::createWithStages(
        [&](PipelineStageBuilder& builder) {
            builder.then(PipelineStage::Connected, [&](BootContext&) -> Result<void> {
                connected = true;
                return {};
            });
        },
        &mockTransport, &mockLogger
    );

    auto result = pipeline->run();
    REQUIRE(result.isOk());
    REQUIRE(connected);
}

SECTION("testResetPipeline") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    (void)pipeline.reset();
    REQUIRE(pipeline.currentStage() == PipelineStage::Disconnected);
    REQUIRE(pipeline.lastError() == ErrorCode::Success);
    REQUIRE(!pipeline.isRunning());
}

SECTION("testRetryAfterError") {
    BootPipeline pipeline;
    pipeline.setTransport(&mockTransport);
    pipeline.setLogger(&mockLogger);

    auto cfg = BootPipelineConfig::defaults();
    cfg.enableRecovery = false;
    pipeline.setConfig(cfg);

    int callCount = 0;

    pipeline.setHandler(PipelineStage::SaharaHandshake, [&](BootContext&) -> Result<void> {
        ++callCount;
        if (callCount <= 1) {
            return ErrorCode::TransportError;
        }
        return {};
    });

    auto r1 = pipeline.run();
    REQUIRE(r1.isError());
    REQUIRE(pipeline.currentStage() == PipelineStage::Error);
    REQUIRE(callCount == 1);

    cfg.enableRecovery = true;
    pipeline.setConfig(cfg);

    auto r2 = pipeline.retry();
    REQUIRE(r2.isOk());
    REQUIRE(pipeline.currentStage() == PipelineStage::Ready);
    REQUIRE(callCount == 2);
}

}
