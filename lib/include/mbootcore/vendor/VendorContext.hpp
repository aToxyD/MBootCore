#pragma once

#include <mbootcore/vendor/VendorTypes.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace mbootcore {

namespace session { class DeviceSession; }
namespace workflow { class WorkflowEngine; }
namespace job { class JobPipeline; }
namespace pipeline { class BootPipeline; }
class IFlashDevice;
namespace plugin { class PluginManager; }
namespace transport { class TransportManager; }
namespace firmware { class FirmwarePackage; }
namespace loader { class LoaderFramework; }

namespace vendor {

struct VendorContext {
    std::string vendorId;
    session::DeviceSession* deviceSession{nullptr};
    workflow::WorkflowEngine* workflowEngine{nullptr};
    job::JobPipeline* jobPipeline{nullptr};
    pipeline::BootPipeline* bootPipeline{nullptr};
    IFlashDevice* flashDevice{nullptr};
    plugin::PluginManager* pluginManager{nullptr};
    transport::TransportManager* transportManager{nullptr};
    firmware::FirmwarePackage* firmwarePackage{nullptr};
    loader::LoaderFramework* loaderFramework{nullptr};
    std::unordered_map<std::string, std::string> properties;
};

} // namespace vendor
} // namespace mbootcore
