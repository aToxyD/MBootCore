#pragma once

#include <mbootcore/runtime/RuntimeConfig.hpp>
#include <mbootcore/runtime/Runtime.hpp>

#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/transport/TransportManager.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/vendor/VendorRuntime.hpp>
#include <mbootcore/plugin/PluginManager.hpp>
#include <mbootcore/loader/LoaderFramework.hpp>
#include <mbootcore/session/DeviceManager.hpp>
#include <mbootcore/workflow/WorkflowFactory.hpp>
#include <mbootcore/job/JobScheduler.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>

#include <memory>

namespace mbootcore {
namespace runtime {

class RuntimeBuilder {
public:
    RuntimeBuilder() = default;

    RuntimeBuilder& withConfig(const RuntimeConfig& config);
    RuntimeBuilder& withLogger(std::unique_ptr<ILogger> logger);
    RuntimeBuilder& withTransport(std::unique_ptr<transport::TransportManager> manager);
    RuntimeBuilder& withProtocolRegistry(std::unique_ptr<discovery::ProtocolRegistry> registry);
    RuntimeBuilder& withVendorRuntime(std::unique_ptr<vendor::VendorRuntime> runtime);
    RuntimeBuilder& withPluginManager(std::unique_ptr<plugin::PluginManager> manager);
    RuntimeBuilder& withLoaderFramework(std::unique_ptr<LoaderFramework> framework);
    RuntimeBuilder& withDiscoveryEngine(std::unique_ptr<discovery::DeviceDiscoveryEngine> engine);
    RuntimeBuilder& withNegotiationEngine(std::unique_ptr<discovery::ProtocolNegotiationEngine> engine);
    RuntimeBuilder& withDeviceManager(std::unique_ptr<session::DeviceManager> manager);
    RuntimeBuilder& withWorkflowFactory(std::unique_ptr<workflow::WorkflowFactory> factory);
    RuntimeBuilder& withJobScheduler(std::unique_ptr<job::JobScheduler> scheduler);
    RuntimeBuilder& withBootPipeline(std::unique_ptr<pipeline::BootPipeline> pipeline);

    Runtime build();

private:
    RuntimeConfig m_config;
    std::unique_ptr<ILogger> m_loggerOverride;
    std::unique_ptr<transport::TransportManager> m_transportOverride;
    std::unique_ptr<discovery::ProtocolRegistry> m_registryOverride;
    std::unique_ptr<vendor::VendorRuntime> m_vendorOverride;
    std::unique_ptr<plugin::PluginManager> m_pluginOverride;
    std::unique_ptr<LoaderFramework> m_loaderOverride;
    std::unique_ptr<discovery::DeviceDiscoveryEngine> m_discoveryOverride;
    std::unique_ptr<discovery::ProtocolNegotiationEngine> m_negotiationOverride;
    std::unique_ptr<session::DeviceManager> m_sessionOverride;
    std::unique_ptr<workflow::WorkflowFactory> m_workflowOverride;
    std::unique_ptr<job::JobScheduler> m_jobOverride;
    std::unique_ptr<pipeline::BootPipeline> m_pipelineOverride;
};

} // namespace runtime
} // namespace mbootcore
