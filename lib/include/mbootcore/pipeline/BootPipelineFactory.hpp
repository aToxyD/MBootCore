#pragma once

#include <mbootcore/pipeline/PipelineStage.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/domain/DeviceTypes.hpp>

#include <memory>
#include <vector>
#include <cstdint>
#include <functional>

namespace mbootcore {
namespace pipeline {

class PipelineStageBuilder {
public:
    explicit PipelineStageBuilder(BootPipeline& pipeline) : m_pipeline(pipeline) {}

    PipelineStageBuilder& then(PipelineStage stage, StageHandler handler) {
        m_pipeline.setHandler(stage, std::move(handler));
        return *this;
    }

    PipelineStageBuilder& thenSuccess(PipelineStage stage) {
        m_pipeline.setHandler(stage, [](BootContext&) { return Result<void>::Ok(); });
        return *this;
    }

    PipelineStageBuilder& thenFailing(PipelineStage stage, ErrorCode error) {
        m_pipeline.setHandler(stage, [error](BootContext&) { return Result<void>::Error(error); });
        return *this;
    }

    void build() {}

private:
    BootPipeline& m_pipeline;
};

class BootPipelineFactory {
public:
    static BootPipeline createWithDefaults() {
        return BootPipeline{};
    }

    static std::unique_ptr<BootPipeline> createWithTransport(ITransport* transport) {
        auto pipeline = std::make_unique<BootPipeline>();
        pipeline->setTransport(transport);
        return pipeline;
    }

    static std::unique_ptr<BootPipeline> createFromDescriptor(
        const discovery::DeviceDescriptor& descriptor,
        ITransport* transport = nullptr,
        ILogger* logger = nullptr) {
        auto pipeline = std::make_unique<BootPipeline>();
        if (transport) pipeline->setTransport(transport);
        if (logger) pipeline->setLogger(logger);

        auto& ctx = pipeline->context();
        ctx.properties["vendor"] = std::to_string(static_cast<uint32_t>(descriptor.vendor));
        ctx.properties["bootMode"] = std::to_string(static_cast<uint32_t>(descriptor.bootMode));
        ctx.properties["protocolHint"] = std::to_string(static_cast<uint32_t>(descriptor.protocolHint));
        ctx.properties["friendlyName"] = descriptor.friendlyName;
        ctx.properties["connectionPath"] = descriptor.connectionPath;

        pipeline->setConfig(BootPipelineConfig{});
        return pipeline;
    }

    using StageSetupFn = std::function<void(PipelineStageBuilder&)>;

    static std::unique_ptr<BootPipeline> createWithStages(
        StageSetupFn setupFn,
        ITransport* transport = nullptr,
        ILogger* logger = nullptr) {
        auto pipeline = std::make_unique<BootPipeline>();
        if (transport) pipeline->setTransport(transport);
        if (logger) pipeline->setLogger(logger);
        PipelineStageBuilder builder(*pipeline);
        setupFn(builder);
        return pipeline;
    }
};

} // namespace pipeline
} // namespace mbootcore
