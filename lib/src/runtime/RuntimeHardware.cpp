#include <mbootcore/runtime/RuntimeHardware.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

namespace mbootcore {
namespace runtime {

std::string HardwareDiagnosticReport::toString() const {
    std::ostringstream ss;
    ss << "=== MBootCore Hardware Diagnostic Report ===\n";
    ss << "Timestamp: " << timestamp << "\n";
    ss << "OS: " << osVersion << "\n";
    ss << "MBoot Version: " << mbootVersion << "\n";
    ss << "WinUSB Available: " << (winUsbAvailable ? "Yes" : "No") << "\n\n";

    ss << "--- Detected Devices (" << detectedDevices.size() << ") ---\n";
    for (const auto& dev : detectedDevices) {
        ss << "  " << dev.name
           << " (VID=0x" << std::hex << dev.vendorId
           << " PID=0x" << dev.productId << std::dec << ")"
           << " [" << dev.transportType << "]"
           << " Serial: " << dev.serial
           << (dev.connected ? " [CONNECTED]" : "")
           << "\n";
    }

    ss << "\n--- Transport Statistics ---\n";
    auto printStats = [&](const std::string& label, const HardwareTransportStats& s) {
        ss << "  " << label << ":\n";
        ss << "    Read: " << s.bytesRead << " bytes in " << s.readOps << " ops\n";
        ss << "    Write: " << s.bytesWritten << " bytes in " << s.writeOps << " ops\n";
        ss << "    Avg Latency: " << s.avgLatencyMs << " ms\n";
        ss << "    Peak Throughput: " << s.peakThroughputBps / 1000000.0 << " MB/s\n";
        ss << "    Errors: " << s.errors << " | Timeouts: " << s.timeouts
           << " | Reconnects: " << s.reconnects << "\n";
    };
    printStats("USB", usbStats);
    printStats("Serial", serialStats);
    printStats("TCP", tcpStats);

    if (!warnings.empty()) {
        ss << "\n--- Warnings ---\n";
        for (const auto& w : warnings) ss << "  ! " << w << "\n";
    }

    if (!failures.empty()) {
        ss << "\n--- Failures ---\n";
        for (const auto& f : failures) ss << "  X " << f << "\n";
    }

    if (!recommendations.empty()) {
        ss << "\n--- Recommendations ---\n";
        for (const auto& r : recommendations) ss << "  * " << r << "\n";
    }

    ss << "\n=== End of Report ===\n";
    return ss.str();
}

std::string HardwareDiagnosticReport::toJson() const {
    try {
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["osVersion"] = osVersion;
        j["mbootVersion"] = mbootVersion;
        j["winUsbAvailable"] = winUsbAvailable;

        auto devices = nlohmann::json::array();
        for (const auto& d : detectedDevices) {
            nlohmann::json dev;
            dev["name"] = d.name;
            dev["vendorId"] = d.vendorId;
            dev["productId"] = d.productId;
            dev["transport"] = d.transportType;
            dev["connected"] = d.connected;
            devices.push_back(std::move(dev));
        }
        j["detectedDevices"] = std::move(devices);
        j["warnings"] = warnings;
        j["failures"] = failures;
        return j.dump(2);
    } catch (...) {
        return "{}";
    }
}

} // namespace runtime
} // namespace mbootcore
