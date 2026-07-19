#include <mbootcore/runtime/RuntimeBuilder.hpp>

#include "DeviceService.hpp"

namespace mbootcore {
namespace runtime {

RuntimeBuilder& RuntimeBuilder::withConfig(const RuntimeConfig& config) {
    m_config = config;
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withLogger(std::unique_ptr<ILogger> logger) {
    m_loggerOverride = std::move(logger);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withTransport(std::unique_ptr<transport::TransportManager> manager) {
    m_transportOverride = std::move(manager);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withProtocolRegistry(std::unique_ptr<discovery::ProtocolRegistry> registry) {
    m_registryOverride = std::move(registry);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withVendorRuntime(std::unique_ptr<vendor::VendorRuntime> runtime) {
    m_vendorOverride = std::move(runtime);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withPluginManager(std::unique_ptr<plugin::PluginManager> manager) {
    m_pluginOverride = std::move(manager);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withLoaderFramework(std::unique_ptr<LoaderFramework> framework) {
    m_loaderOverride = std::move(framework);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withDiscoveryEngine(std::unique_ptr<discovery::DeviceDiscoveryEngine> engine) {
    m_discoveryOverride = std::move(engine);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withNegotiationEngine(std::unique_ptr<discovery::ProtocolNegotiationEngine> engine) {
    m_negotiationOverride = std::move(engine);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withDeviceManager(std::unique_ptr<session::DeviceManager> manager) {
    m_sessionOverride = std::move(manager);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withWorkflowFactory(std::unique_ptr<workflow::WorkflowFactory> factory) {
    m_workflowOverride = std::move(factory);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withJobScheduler(std::unique_ptr<job::JobScheduler> scheduler) {
    m_jobOverride = std::move(scheduler);
    return *this;
}

RuntimeBuilder& RuntimeBuilder::withBootPipeline(std::unique_ptr<pipeline::BootPipeline> pipeline) {
    m_pipelineOverride = std::move(pipeline);
    return *this;
}

Runtime RuntimeBuilder::build() {
    Runtime rt;
    rt.m_config = m_config;

    if (m_loggerOverride) rt.m_logger = std::move(m_loggerOverride);
    if (m_transportOverride) rt.m_transportManager = std::move(m_transportOverride);
    if (m_vendorOverride) rt.m_vendorRuntimeOverride = std::move(m_vendorOverride);
    if (m_pluginOverride) rt.m_pluginManagerOverride = std::move(m_pluginOverride);
    if (m_loaderOverride) rt.m_loaderOverride = std::move(m_loaderOverride);
    if (m_workflowOverride) rt.m_workflowFactoryOverride = std::move(m_workflowOverride);
    if (m_jobOverride) rt.m_jobSchedulerOverride = std::move(m_jobOverride);
    if (m_pipelineOverride) rt.m_bootPipelineOverride = std::move(m_pipelineOverride);

    // Build DeviceService from overrides or defaults
    auto registry = m_registryOverride
        ? std::move(m_registryOverride)
        : std::make_unique<discovery::ProtocolRegistry>();
    auto* regPtr = registry.get();

    auto discoveryEngine = m_discoveryOverride
        ? std::move(m_discoveryOverride)
        : std::make_unique<discovery::DeviceDiscoveryEngine>(*regPtr);

    auto negEngine = m_negotiationOverride
        ? std::move(m_negotiationOverride)
        : std::make_unique<discovery::ProtocolNegotiationEngine>(*regPtr);

    auto sessionMgr = m_sessionOverride
        ? std::move(m_sessionOverride)
        : std::make_unique<session::DeviceManager>(*regPtr);

    rt.m_deviceService = std::make_unique<DeviceService>(
        std::move(registry),
        std::move(discoveryEngine),
        std::move(negEngine),
        std::move(sessionMgr));

    return rt;
}

} // namespace runtime
} // namespace mbootcore
