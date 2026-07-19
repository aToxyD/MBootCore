#include "mbootcore/discovery/VirtualDeviceDetector.hpp"

#include <thread>
#include <algorithm>

namespace mbootcore {
namespace discovery {

VirtualDeviceDetector::VirtualDeviceDetector()
    : m_rng(42) {}

Result<std::vector<DeviceDescriptor>> VirtualDeviceDetector::enumerate() {
    if (shouldFail()) {
        return ErrorCode::DeviceNotFound;
    }
    applyDelay();

    std::vector<DeviceDescriptor> results;
    for (const auto& spec : m_specs) {
        if (spec.connectable) {
            DeviceDescriptor desc;
            desc.vendor = spec.vendor;
            desc.bootMode = spec.bootMode;
            desc.transport = spec.transport;
            desc.protocolHint = spec.protocolHint;
            desc.usbVid = spec.usbVid;
            desc.usbPid = spec.usbPid;
            desc.friendlyName = spec.friendlyName;
            desc.connectionPath = spec.connectionPath;
            results.push_back(std::move(desc));
        }
    }
    return results;
}

Result<DeviceDescriptor> VirtualDeviceDetector::identify(const DeviceDescriptor& hint) {
    if (shouldFail()) {
        return ErrorCode::DeviceNotFound;
    }
    applyDelay();

    for (const auto& spec : m_specs) {
        if (spec.connectionPath == hint.connectionPath || spec.friendlyName == hint.friendlyName) {
            DeviceDescriptor desc;
            desc.vendor = spec.vendor;
            desc.bootMode = spec.bootMode;
            desc.transport = spec.transport;
            desc.protocolHint = spec.protocolHint;
            desc.usbVid = spec.usbVid;
            desc.usbPid = spec.usbPid;
            desc.friendlyName = spec.friendlyName;
            desc.connectionPath = spec.connectionPath;
            return desc;
        }
    }
    return ErrorCode::DeviceNotFound;
}

Result<void> VirtualDeviceDetector::probe(DeviceDescriptor& descriptor) {
    if (shouldFail()) {
        return ErrorCode::TransportError;
    }
    applyDelay();

    for (const auto& spec : m_specs) {
        if (spec.connectionPath == descriptor.connectionPath) {
            descriptor.vendor = spec.vendor;
            descriptor.bootMode = spec.bootMode;
            descriptor.protocolHint = spec.protocolHint;
            descriptor.friendlyName = spec.friendlyName;
            return {};
        }
    }
    return ErrorCode::DeviceNotFound;
}

Result<void> VirtualDeviceDetector::refresh() {
    return {};
}

void VirtualDeviceDetector::addDevice(const VirtualDeviceSpec& spec) {
    m_specs.push_back(spec);
}

void VirtualDeviceDetector::removeDevice(const std::string& connectionPath) {
    m_specs.erase(std::remove_if(m_specs.begin(), m_specs.end(),
        [&](const auto& s) { return s.connectionPath == connectionPath; }),
        m_specs.end());
}

void VirtualDeviceDetector::clearDevices() {
    m_specs.clear();
}

void VirtualDeviceDetector::setFailProbability(double probability) {
    m_failProbability = std::clamp(probability, 0.0, 1.0);
}

void VirtualDeviceDetector::setProbeDelay(int ms) {
    m_probeDelayMs = std::max(0, ms);
}

void VirtualDeviceDetector::setRandomSeed(uint32_t seed) {
    m_rng.seed(seed);
}

bool VirtualDeviceDetector::shouldFail() {
    if (m_failProbability <= 0.0) return false;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(m_rng) < m_failProbability;
}

void VirtualDeviceDetector::applyDelay() {
    if (m_probeDelayMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_probeDelayMs));
    }
}

VirtualDeviceSpec VirtualDeviceDetector::createQualcommEDL(uint16_t vid, uint16_t pid) {
    return {Vendor::Qualcomm, BootMode::EDL, TransportType::USB, ProtocolType::Sahara,
            vid, pid, "Qualcomm EDL Device", "usb:05C6:9008", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createQualcommFirehose(uint16_t vid, uint16_t pid) {
    return {Vendor::Qualcomm, BootMode::Firehose, TransportType::USB, ProtocolType::Firehose,
            vid, pid, "Qualcomm Firehose Device", "usb:05C6:900E", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createMediaTekPreloader(uint16_t vid, uint16_t pid) {
    return {Vendor::MediaTek, BootMode::Preloader, TransportType::USB, ProtocolType::MediaTekBROM,
            vid, pid, "MediaTek Preloader Device", "usb:0E8D:2000", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createUNISOCBootROM(uint16_t vid, uint16_t pid) {
    return {Vendor::UNISOC, BootMode::BROM, TransportType::USB, ProtocolType::UNISOCBootROM,
            vid, pid, "UNISOC BootROM Device", "usb:1782:4D00", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createSamsungDownload(uint16_t vid, uint16_t pid) {
    return {Vendor::Samsung, BootMode::DownloadMode, TransportType::USB, ProtocolType::USBStream,
            vid, pid, "Samsung Download Device", "usb:04E8:685D", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createRockchipMaskROM(uint16_t vid, uint16_t pid) {
    return {Vendor::Rockchip, BootMode::BootROM, TransportType::USB, ProtocolType::USBStream,
            vid, pid, "Rockchip MaskROM Device", "usb:2207:350A", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createUnknownDevice() {
    return {Vendor::Unknown, BootMode::Unknown, TransportType::Unknown, ProtocolType::Unknown,
            0x0000, 0x0000, "Unknown Device", "unknown:0000:0000", true, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createDisconnectedDevice() {
    return {Vendor::Unknown, BootMode::Unknown, TransportType::Unknown, ProtocolType::Unknown,
            0x0000, 0x0000, "Disconnected Device", "disconnected:0000:0000", false, 0, 0.0};
}

VirtualDeviceSpec VirtualDeviceDetector::createTimeoutDevice() {
    return {Vendor::Qualcomm, BootMode::EDL, TransportType::USB, ProtocolType::Sahara,
            0x05C6, 0x9008, "Timeout Device", "usb:05C6:9008", true, 5000, 0.0};
}

// VirtualProtocolNegotiator

VirtualProtocolNegotiator::VirtualProtocolNegotiator(ProtocolType type, int baseConfidence)
    : m_type(type), m_baseConfidence(baseConfidence) {}

std::string VirtualProtocolNegotiator::name() const {
    switch (m_type) {
        case ProtocolType::Sahara: return "SaharaNegotiator";
        case ProtocolType::Firehose: return "FirehoseNegotiator";
        case ProtocolType::MediaTekBROM: return "MediaTekBROMNegotiator";
        case ProtocolType::UNISOCBootROM: return "UNISOCBootROMNegotiator";
        case ProtocolType::Fastboot: return "FastbootNegotiator";
        case ProtocolType::Custom: return "CustomNegotiator";
        default: return "UnknownNegotiator";
    }
}

NegotiationResult VirtualProtocolNegotiator::negotiate(const DeviceDescriptor& descriptor) {
    NegotiationResult result;
    result.descriptor = descriptor;
    result.protocol = m_type;
    result.confidence = 0;
    result.reason = "No match";

    switch (m_type) {
        case ProtocolType::Sahara:
            if (descriptor.protocolHint == ProtocolType::Sahara ||
                (descriptor.vendor == Vendor::Qualcomm &&
                 (descriptor.bootMode == BootMode::EDL || descriptor.bootMode == BootMode::BootROM))) {
                result.confidence = m_baseConfidence + 20;
                result.reason = "Qualcomm EDL/BootROM detected";
            } else if (descriptor.vendor == Vendor::Qualcomm) {
                result.confidence = m_baseConfidence - 30;
                result.reason = "Qualcomm vendor match (fallback)";
            }
            break;

        case ProtocolType::Firehose:
            if (descriptor.protocolHint == ProtocolType::Firehose ||
                (descriptor.vendor == Vendor::Qualcomm && descriptor.bootMode == BootMode::Firehose)) {
                result.confidence = m_baseConfidence + 20;
                result.reason = "Firehose mode detected";
            } else if (descriptor.vendor == Vendor::Qualcomm) {
                result.confidence = m_baseConfidence - 20;
                result.reason = "Qualcomm vendor match (fallback)";
            }
            break;

        case ProtocolType::MediaTekBROM:
            if (descriptor.protocolHint == ProtocolType::MediaTekBROM ||
                descriptor.vendor == Vendor::MediaTek) {
                result.confidence = m_baseConfidence + 20;
                result.reason = "MediaTek device detected";
            }
            break;

        case ProtocolType::UNISOCBootROM:
            if (descriptor.protocolHint == ProtocolType::UNISOCBootROM ||
                descriptor.vendor == Vendor::UNISOC) {
                result.confidence = m_baseConfidence + 20;
                result.reason = "UNISOC device detected";
            }
            break;

        default:
            break;
    }

    return result;
}

} // namespace discovery
} // namespace mbootcore
