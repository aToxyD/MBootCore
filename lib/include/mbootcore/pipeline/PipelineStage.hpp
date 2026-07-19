#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mbootcore {
namespace pipeline {

enum class PipelineStage : uint8_t {
    Disconnected = 0,
    Connected,
    SaharaHandshake,
    VersionNegotiation,
    DeviceDiscovery,
    LoaderSelection,
    ElfParsing,
    MemoryImageBuild,
    ProgrammerUpload,
    ProgrammerExecute,
    FirehoseDetection,
    FirehoseConfiguration,
    Ready,
    Error,
    Cancelled
};

enum class PipelineEvent : uint8_t {
    StageCompleted,
    StageFailed,
    Cancelled,
    RecoveryRetry,
    RecoveryRollback,
    RecoveryRestart,
    RecoveryAbort,
    Reset
};

constexpr std::string_view toString(PipelineStage stage) {
    switch (stage) {
        case PipelineStage::Disconnected:         return "Disconnected";
        case PipelineStage::Connected:            return "Connected";
        case PipelineStage::SaharaHandshake:      return "SaharaHandshake";
        case PipelineStage::VersionNegotiation:   return "VersionNegotiation";
        case PipelineStage::DeviceDiscovery:      return "DeviceDiscovery";
        case PipelineStage::LoaderSelection:      return "LoaderSelection";
        case PipelineStage::ElfParsing:           return "ElfParsing";
        case PipelineStage::MemoryImageBuild:     return "MemoryImageBuild";
        case PipelineStage::ProgrammerUpload:     return "ProgrammerUpload";
        case PipelineStage::ProgrammerExecute:    return "ProgrammerExecute";
        case PipelineStage::FirehoseDetection:    return "FirehoseDetection";
        case PipelineStage::FirehoseConfiguration: return "FirehoseConfiguration";
        case PipelineStage::Ready:                return "Ready";
        case PipelineStage::Error:                return "Error";
        case PipelineStage::Cancelled:            return "Cancelled";
    }
    return "Unknown";
}

constexpr std::string_view toString(PipelineEvent event) {
    switch (event) {
        case PipelineEvent::StageCompleted:   return "StageCompleted";
        case PipelineEvent::StageFailed:      return "StageFailed";
        case PipelineEvent::Cancelled:        return "Cancelled";
        case PipelineEvent::RecoveryRetry:    return "RecoveryRetry";
        case PipelineEvent::RecoveryRollback: return "RecoveryRollback";
        case PipelineEvent::RecoveryRestart:  return "RecoveryRestart";
        case PipelineEvent::RecoveryAbort:    return "RecoveryAbort";
        case PipelineEvent::Reset:            return "Reset";
    }
    return "Unknown";
}

} // namespace pipeline
} // namespace mbootcore
