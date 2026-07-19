#pragma once

#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>

#include <mbootcore/discovery/VirtualDeviceDetector.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/discovery/ProtocolNegotiationEngine.hpp>
#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <mbootcore/logging/NullLogger.hpp>

#include <memory>
#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

class VirtualRuntime {
public:
    VirtualRuntime() {
        RuntimeConfig cfg;
        cfg.enableVendorRuntime = false;
        cfg.autoLoadPlugins = false;
        cfg.enableMonitoring = true;
        cfg.enableStatistics = true;

        auto registry = std::make_unique<discovery::ProtocolRegistry>();

        auto virtualDetector = std::make_unique<discovery::VirtualDeviceDetector>();
        virtualDetector->addDevice(discovery::VirtualDeviceDetector::createQualcommEDL());
        virtualDetector->addDevice(discovery::VirtualDeviceDetector::createMediaTekPreloader());
        registry->registerDetector(std::move(virtualDetector));

        registry->registerNegotiator(
            std::make_unique<discovery::VirtualProtocolNegotiator>(
                discovery::ProtocolType::Sahara, 100));

        registry->registerNegotiator(
            std::make_unique<discovery::VirtualProtocolNegotiator>(
                discovery::ProtocolType::Firehose, 80));

        m_registry = registry.get();

        m_runtime = std::make_unique<Runtime>(
            RuntimeBuilder()
                .withConfig(cfg)
                .withLogger(std::make_unique<NullLogger>())
                .withProtocolRegistry(std::move(registry))
                .build()
        );
    }

    Runtime& runtime() { return *m_runtime; }
    const Runtime& runtime() const { return *m_runtime; }

    Result<void> initialize() { return m_runtime->initialize(); }
    void shutdown() { m_runtime->shutdown(); }

    auto discover(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        return m_runtime->discover(timeout);
    }

    auto connect(const discovery::DeviceDescriptor& desc) {
        return m_runtime->connect(desc);
    }

    void disconnect() { m_runtime->disconnect(); }

    // Get virtual devices for tests
    std::vector<discovery::DeviceDescriptor> getVirtualDevices() {
        std::vector<discovery::DeviceDescriptor> result;
        auto& detectors = m_registry->detectors();
        for (auto& d : detectors) {
            auto r = d->enumerate();
            if (r.isOk()) {
                for (auto& dev : r.value()) {
                    result.push_back(std::move(dev));
                }
            }
        }
        return result;
    }

    void addVirtualDevice(const discovery::VirtualDeviceSpec& spec) {
        for (auto& d : m_registry->detectors()) {
            auto* vd = dynamic_cast<discovery::VirtualDeviceDetector*>(d.get());
            if (vd) {
                vd->addDevice(spec);
                return;
            }
        }
    }

    void clearDevices() {
        for (auto& d : m_registry->detectors()) {
            auto* vd = dynamic_cast<discovery::VirtualDeviceDetector*>(d.get());
            if (vd) {
                vd->clearDevices();
                return;
            }
        }
    }

private:
    std::unique_ptr<Runtime> m_runtime;
    discovery::ProtocolRegistry* m_registry{nullptr};
};

} // namespace runtime
} // namespace mbootcore
