#include "gui/runtime/RuntimeErrorMapper.hpp"

namespace gui {
namespace runtime {

RuntimeError RuntimeErrorMapper::map(mbootcore::ErrorCode code)
{
    RuntimeError err;
    err.coreCode = code;
    err.severity = severityFor(code);
    err.title = toString(code);
    err.message = std::string(mbootcore::toString(code));
    return err;
}

RuntimeError RuntimeErrorMapper::map(const mbootcore::ErrorInfo& info)
{
    RuntimeError err;
    err.coreCode = info.code;
    err.severity = severityFor(info.code);
    err.title = toString(info.code);
    err.message = info.context.empty()
        ? std::string(mbootcore::toString(info.code))
        : info.context;
    err.details = info.context;
    err.source = info.source;
    return err;
}

RuntimeError RuntimeErrorMapper::map(const std::string& context, mbootcore::ErrorCode code)
{
    RuntimeError err;
    err.coreCode = code;
    err.severity = severityFor(code);
    err.title = toString(code);
    err.message = context.empty()
        ? std::string(mbootcore::toString(code))
        : context;
    err.details = std::string(mbootcore::toString(code));
    return err;
}

std::string RuntimeErrorMapper::toString(mbootcore::ErrorCode code)
{
    switch (code) {
        case mbootcore::ErrorCode::Success:
            return "Success";
        case mbootcore::ErrorCode::Unknown:
            return "Unknown Error";
        case mbootcore::ErrorCode::NotSupported:
            return "Not Supported";
        case mbootcore::ErrorCode::InvalidArgument:
            return "Invalid Argument";
        case mbootcore::ErrorCode::Cancelled:
            return "Cancelled";

        case mbootcore::ErrorCode::TransportError:
        case mbootcore::ErrorCode::TransportTimeout:
        case mbootcore::ErrorCode::TransportDisconnected:
        case mbootcore::ErrorCode::TransportWriteFailed:
        case mbootcore::ErrorCode::TransportReadFailed:
        case mbootcore::ErrorCode::TransportNotOpen:
        case mbootcore::ErrorCode::TransportAlreadyOpen:
        case mbootcore::ErrorCode::TransportBusy:
        case mbootcore::ErrorCode::TransportInvalidEndpoint:
        case mbootcore::ErrorCode::TransportBufferFull:
        case mbootcore::ErrorCode::TransportPartialTransfer:
        case mbootcore::ErrorCode::TransportReconnectFailed:
        case mbootcore::ErrorCode::TransportHotplugRemoved:
        case mbootcore::ErrorCode::TransportHotplugArrived:
        case mbootcore::ErrorCode::TransportEnumerationFailed:
        case mbootcore::ErrorCode::TransportBackendUnavailable:
        case mbootcore::ErrorCode::TransportAsyncCancelled:
        case mbootcore::ErrorCode::TransportRetryExhausted:
        case mbootcore::ErrorCode::TransportBufferTooSmall:
            return "Transport Error";

        case mbootcore::ErrorCode::ProtocolError:
        case mbootcore::ErrorCode::ProtocolMismatch:
        case mbootcore::ErrorCode::InvalidPacket:
        case mbootcore::ErrorCode::UnexpectedPacket:
        case mbootcore::ErrorCode::InvalidState:
        case mbootcore::ErrorCode::UnsupportedVersion:
            return "Protocol Error";

        case mbootcore::ErrorCode::DeviceNotFound:
        case mbootcore::ErrorCode::DeviceDisconnected:
        case mbootcore::ErrorCode::DeviceAccessDenied:
            return "Device Error";

        case mbootcore::ErrorCode::LoaderNotFound:
        case mbootcore::ErrorCode::LoaderRejected:
        case mbootcore::ErrorCode::LoaderInvalidFormat:
        case mbootcore::ErrorCode::LoaderVersionMismatch:
        case mbootcore::ErrorCode::InvalidElf:
        case mbootcore::ErrorCode::CacheMiss:
            return "Loader Error";

        case mbootcore::ErrorCode::FirehoseNak:
        case mbootcore::ErrorCode::FirehoseSectorOutOfRange:
        case mbootcore::ErrorCode::FirehoseProgramFailed:
        case mbootcore::ErrorCode::FirehoseEraseFailed:
        case mbootcore::ErrorCode::FirehoseUnsupportedMemory:
        case mbootcore::ErrorCode::FirehoseInvalidParam:
        case mbootcore::ErrorCode::FirehoseSeqError:
        case mbootcore::ErrorCode::FirehoseTimeout:
        case mbootcore::ErrorCode::FirehoseCrcMismatch:
        case mbootcore::ErrorCode::FirehosePartialWrite:
            return "Firehose Error";

        case mbootcore::ErrorCode::GPTCorrupted:
        case mbootcore::ErrorCode::GPTInvalidHeader:
        case mbootcore::ErrorCode::GPTInvalidEntry:
        case mbootcore::ErrorCode::GPTPrimaryMissing:
        case mbootcore::ErrorCode::GPTBackupMissing:
        case mbootcore::ErrorCode::GPTCrcMismatch:
        case mbootcore::ErrorCode::PartitionNotFound:
        case mbootcore::ErrorCode::PartitionOverlap:
        case mbootcore::ErrorCode::InvalidImage:
        case mbootcore::ErrorCode::ImageTooLarge:
            return "GPT / Partition Error";

        case mbootcore::ErrorCode::DeviceNotIdentified:
        case mbootcore::ErrorCode::NegotiationFailed:
        case mbootcore::ErrorCode::NoMatchingProtocol:
        case mbootcore::ErrorCode::EnumerationFailed:
        case mbootcore::ErrorCode::ProbeFailed:
        case mbootcore::ErrorCode::DetectorConflict:
        case mbootcore::ErrorCode::RegistryEmpty:
            return "Discovery Error";

        case mbootcore::ErrorCode::SessionNotConnected:
        case mbootcore::ErrorCode::SessionAlreadyConnected:
        case mbootcore::ErrorCode::SessionBusy:
        case mbootcore::ErrorCode::SessionTimeout:
        case mbootcore::ErrorCode::SessionTerminal:
        case mbootcore::ErrorCode::SessionLimitExceeded:
            return "Session Error";

        case mbootcore::ErrorCode::PluginLoadFailed:
        case mbootcore::ErrorCode::PluginInitFailed:
        case mbootcore::ErrorCode::PluginShutdownFailed:
        case mbootcore::ErrorCode::PluginNotFound:
        case mbootcore::ErrorCode::PluginIncompatible:
        case mbootcore::ErrorCode::PluginDependencyMissing:
        case mbootcore::ErrorCode::PluginCircularDependency:
        case mbootcore::ErrorCode::PluginDuplicate:
        case mbootcore::ErrorCode::PluginAlreadyLoaded:
        case mbootcore::ErrorCode::PluginNotLoaded:
        case mbootcore::ErrorCode::PluginRegistrationFailed:
        case mbootcore::ErrorCode::PluginUnregistrationFailed:
        case mbootcore::ErrorCode::PluginVersionMismatch:
        case mbootcore::ErrorCode::PluginConfigInvalid:
            return "Plugin Error";

        case mbootcore::ErrorCode::JobFailed:
        case mbootcore::ErrorCode::JobCancelled:
        case mbootcore::ErrorCode::JobTimeout:
        case mbootcore::ErrorCode::JobInvalidState:
        case mbootcore::ErrorCode::JobDependencyFailed:
        case mbootcore::ErrorCode::JobNotFound:
        case mbootcore::ErrorCode::JobAlreadyRunning:
        case mbootcore::ErrorCode::JobQueueFull:
        case mbootcore::ErrorCode::JobRecoveryFailed:
        case mbootcore::ErrorCode::JobRollbackFailed:
        case mbootcore::ErrorCode::JobInvalidConfig:
        case mbootcore::ErrorCode::JobDeviceNotReady:
        case mbootcore::ErrorCode::JobDataMismatch:
        case mbootcore::ErrorCode::JobInsufficientSpace:
            return "Job Error";

        case mbootcore::ErrorCode::FirmwareInvalidFormat:
        case mbootcore::ErrorCode::FirmwareImageNotFound:
        case mbootcore::ErrorCode::FirmwareHashMismatch:
        case mbootcore::ErrorCode::FirmwareInvalidManifest:
        case mbootcore::ErrorCode::FirmwareDependencyConflict:
        case mbootcore::ErrorCode::FirmwareUnsupportedDevice:
        case mbootcore::ErrorCode::FirmwareUnsupportedVendor:
        case mbootcore::ErrorCode::FirmwareUnsupportedStorage:
        case mbootcore::ErrorCode::FirmwareDuplicatePartition:
        case mbootcore::ErrorCode::FirmwareMissingProgrammer:
        case mbootcore::ErrorCode::FirmwareValidationFailed:
        case mbootcore::ErrorCode::FirmwareExtractionFailed:
        case mbootcore::ErrorCode::FirmwarePackageCorrupted:
        case mbootcore::ErrorCode::FirmwareVersionMismatch:
        case mbootcore::ErrorCode::FirmwarePackageNotFound:
        case mbootcore::ErrorCode::FirmwareNotEnoughImages:
            return "Firmware Error";

        case mbootcore::ErrorCode::WorkflowInvalidState:
        case mbootcore::ErrorCode::WorkflowStepFailed:
        case mbootcore::ErrorCode::WorkflowRollbackFailed:
        case mbootcore::ErrorCode::WorkflowCancelled:
        case mbootcore::ErrorCode::WorkflowTimeout:
        case mbootcore::ErrorCode::WorkflowStepNotFound:
        case mbootcore::ErrorCode::WorkflowExecutionFailed:
        case mbootcore::ErrorCode::WorkflowRecoveryFailed:
            return "Workflow Error";

        case mbootcore::ErrorCode::ElfSegmentOverlap:
        case mbootcore::ErrorCode::ElfTruncated:
        case mbootcore::ErrorCode::ElfUnsupportedMachine:
            return "ELF Error";

        case mbootcore::ErrorCode::AlreadyExists:
            return "Already Exists";

        default:
            return "Unknown Error";
    }
}

ErrorSeverity RuntimeErrorMapper::severityFor(mbootcore::ErrorCode code)
{
    switch (code) {
        case mbootcore::ErrorCode::Success:
            return ErrorSeverity::Info;
        case mbootcore::ErrorCode::Cancelled:
        case mbootcore::ErrorCode::NotSupported:
            return ErrorSeverity::Info;
        case mbootcore::ErrorCode::AlreadyExists:
            return ErrorSeverity::Warning;
        case mbootcore::ErrorCode::InvalidArgument:
        case mbootcore::ErrorCode::DeviceNotFound:
        case mbootcore::ErrorCode::PartitionNotFound:
        case mbootcore::ErrorCode::PluginNotFound:
        case mbootcore::ErrorCode::JobNotFound:
        case mbootcore::ErrorCode::FirmwarePackageNotFound:
        case mbootcore::ErrorCode::WorkflowStepNotFound:
            return ErrorSeverity::Warning;
        default:
            return ErrorSeverity::Error;
    }
}

} // namespace runtime
} // namespace gui
