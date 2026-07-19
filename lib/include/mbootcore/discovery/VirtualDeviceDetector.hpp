#pragma once

#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>

namespace mbootcore {
namespace discovery {

struct VirtualDeviceSpec {
    Vendor        vendor;
    BootMode      bootMode;
    TransportType transport;
    ProtocolType  protocolHint;
    uint16_t      usbVid;
    uint16_t      usbPid;
    std::string   friendlyName;
    std::string   connectionPath;
    bool          connectable{true};
    int           probeDelayMs{0};
    double        failProbability{0.0};
};

class VirtualDeviceDetector : public IDeviceDetector {
public:
    VirtualDeviceDetector();

    std::string name() const override { return "VirtualDeviceDetector"; }

    Result<std::vector<DeviceDescriptor>> enumerate() override;

    Result<DeviceDescriptor> identify(const DeviceDescriptor& hint) override;

    Result<void> probe(DeviceDescriptor& descriptor) override;

    Result<void> refresh() override;

    void addDevice(const VirtualDeviceSpec& spec);
    void removeDevice(const std::string& connectionPath);
    void clearDevices();
    void setFailProbability(double probability);
    void setProbeDelay(int ms);
    void setRandomSeed(uint32_t seed);

    std::size_t deviceCount() const { return m_specs.size(); }

    // Preset device configurations
    static VirtualDeviceSpec createQualcommEDL(uint16_t vid = 0x05C6, uint16_t pid = 0x9008);
    static VirtualDeviceSpec createQualcommFirehose(uint16_t vid = 0x05C6, uint16_t pid = 0x900E);
    static VirtualDeviceSpec createMediaTekPreloader(uint16_t vid = 0x0E8D, uint16_t pid = 0x2000);
    static VirtualDeviceSpec createUNISOCBootROM(uint16_t vid = 0x1782, uint16_t pid = 0x4D00);
    static VirtualDeviceSpec createSamsungDownload(uint16_t vid = 0x04E8, uint16_t pid = 0x685D);
    static VirtualDeviceSpec createRockchipMaskROM(uint16_t vid = 0x2207, uint16_t pid = 0x350A);
    static VirtualDeviceSpec createUnknownDevice();
    static VirtualDeviceSpec createDisconnectedDevice();
    static VirtualDeviceSpec createTimeoutDevice();

private:
    std::vector<VirtualDeviceSpec> m_specs;
    double m_failProbability{0.0};
    int m_probeDelayMs{0};
    std::mt19937 m_rng;

    bool shouldFail();
    void applyDelay();
};

class VirtualProtocolNegotiator : public IProtocolNegotiator {
public:
    explicit VirtualProtocolNegotiator(ProtocolType type, int baseConfidence = 100);

    std::string name() const override;

    NegotiationResult negotiate(const DeviceDescriptor& descriptor) override;

    void setConfidence(int confidence) { m_baseConfidence = confidence; }
    int confidence() const { return m_baseConfidence; }

private:
    ProtocolType m_type;
    int m_baseConfidence;
};

} // namespace discovery
} // namespace mbootcore
