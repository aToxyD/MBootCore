#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace mbootcore {
namespace runtime {

struct HardwareTransportStats {
    uint64_t bytesRead{0};
    uint64_t bytesWritten{0};
    uint32_t readOps{0};
    uint32_t writeOps{0};
    uint32_t reconnects{0};
    uint32_t timeouts{0};
    uint32_t errors{0};
    double avgLatencyMs{0.0};
    double peakThroughputBps{0.0};
};

struct HardwareDeviceEntry {
    std::string name;
    std::string vendor;
    std::string serial;
    uint16_t vendorId{0};
    uint16_t productId{0};
    std::string transportType;
    std::string bootMode;
    std::string protocol;
    bool connected{false};
};

struct HardwareDiagnosticReport {
    std::vector<HardwareDeviceEntry> detectedDevices;
    HardwareTransportStats usbStats;
    HardwareTransportStats serialStats;
    HardwareTransportStats tcpStats;
    std::vector<std::string> failures;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
    std::string timestamp;
    std::string osVersion;
    std::string mbootVersion;
    bool winUsbAvailable{false};

    std::string toString() const;
    std::string toJson() const;
};

} // namespace runtime
} // namespace mbootcore
