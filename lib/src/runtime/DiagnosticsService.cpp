#include "DiagnosticsService.hpp"
#include "RuntimeState.hpp"
#include "DeviceService.hpp"
#include "PluginService.hpp"

#include <mbootcore/transport/usb/UsbDeviceEnumerator.hpp>
#include <mbootcore/transport/SerialEnumerator.hpp>
#include <mbootcore/transport/usb/makeUsbBackend.hpp>

#include <chrono>
#include <ctime>
#include <string>
#include <vector>

namespace mbootcore {
namespace runtime {

RuntimeStatistics DiagnosticsService::statistics() const {
    std::lock_guard<std::mutex> lock(m_state.statsMutex);
    auto stats = m_state.stats;

    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - m_state.startTime);
    stats.uptimeSeconds = elapsed.count();

    return stats;
}

RuntimeHealth DiagnosticsService::health() const {
    RuntimeHealth h;

    h.activeSessions = static_cast<uint32_t>(m_deviceService.sessionManager().activeCount());
    h.connectedDevices = m_state.connected ? 1u : 0u;
    h.loadedPlugins = static_cast<uint32_t>(m_pluginService.pluginManager().pluginCount());
    h.loadedVendors = static_cast<uint32_t>(m_pluginService.vendorRuntime().registry().vendorCount());
    h.queuedJobs = 0;
    h.transportState = m_state.connected ? "Connected" : "Disconnected";

    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - m_state.startTime);
    h.uptimeSeconds = elapsed.count();

    h.threadCount = 0;
    h.memoryUsageBytes = 0;

    return h;
}

HardwareDiagnosticReport DiagnosticsService::hardwareReport() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char tsBuf[64];
    strftime(tsBuf, sizeof(tsBuf), "%Y-%m-%d %H:%M:%S", &tm);

    HardwareDiagnosticReport report;
    report.timestamp = tsBuf;
    report.mbootVersion = versionString();
    report.winUsbAvailable = transport::usb::isWinUsbAvailable();

#ifdef _WIN32
    report.osVersion = "Windows";
#elif __linux__
    report.osVersion = "Linux";
#elif __APPLE__
    report.osVersion = "macOS";
#else
    report.osVersion = "Unknown";
#endif

    report.detectedDevices = connectedHardware();

    auto allIds = m_transportManager.ids();
    for (const auto& id : allIds) {
        auto* tr = m_transportManager.get(id);
        if (!tr) continue;
        auto s = tr->statistics();
        HardwareTransportStats hts;
        hts.bytesRead = s.bytesRead;
        hts.bytesWritten = s.bytesWritten;
        hts.readOps = static_cast<uint32_t>(s.readOperations);
        hts.writeOps = static_cast<uint32_t>(s.writeOperations);
        hts.reconnects = s.reconnectCount;
        hts.timeouts = s.timeoutCount;
        hts.errors = s.errorCount;
        hts.avgLatencyMs = s.averageLatency;
        hts.peakThroughputBps = s.peakThroughput;

        switch (tr->transportType()) {
            case transport::TransportType::USB: report.usbStats = hts; break;
            case transport::TransportType::Serial: report.serialStats = hts; break;
            case transport::TransportType::TCP: report.tcpStats = hts; break;
            default: break;
        }
    }

    return report;
}

std::vector<HardwareDeviceEntry> DiagnosticsService::connectedHardware() const {
    std::vector<HardwareDeviceEntry> entries;

    auto usbDevs = usbDevices();
    for (const auto& dev : usbDevs) {
        HardwareDeviceEntry entry;
        entry.name = dev.product.empty() ? ("Device " + std::to_string(dev.vendorId) + ":" + std::to_string(dev.productId)) : dev.product;
        entry.vendor = dev.manufacturer;
        entry.serial = dev.serialNumber;
        entry.vendorId = dev.vendorId;
        entry.productId = dev.productId;
        entry.transportType = "USB";
        entry.bootMode = dev.isBootMode() ? "Yes" : "No";
        entry.connected = dev.isAvailable;
        entries.push_back(entry);
    }

    return entries;
}

HardwareTransportStats DiagnosticsService::transportStatistics(transport::TransportType type) const {
    HardwareTransportStats hts;

    auto allIds = m_transportManager.ids();
    for (const auto& id : allIds) {
        auto* t = m_transportManager.get(id);
        if (!t || t->transportType() != type) continue;
        auto s = t->statistics();
        hts.bytesRead = s.bytesRead;
        hts.bytesWritten = s.bytesWritten;
        hts.readOps = static_cast<uint32_t>(s.readOperations);
        hts.writeOps = static_cast<uint32_t>(s.writeOperations);
        hts.reconnects = s.reconnectCount;
        hts.timeouts = s.timeoutCount;
        hts.errors = s.errorCount;
        hts.avgLatencyMs = s.averageLatency;
        hts.peakThroughputBps = s.peakThroughput;
        break;
    }
    return hts;
}

std::vector<transport::usb::UsbDeviceInfo> DiagnosticsService::usbDevices() const {
#ifdef _WIN32
    return transport::usb::UsbDeviceEnumerator::enumerate();
#else
    return {};
#endif
}

std::vector<std::string> DiagnosticsService::serialPorts() const {
    auto entries = transport::SerialEnumerator::enumerate();
    std::vector<std::string> ports;
    for (const auto& e : entries) ports.push_back(e.portName);
    return ports;
}

std::string DiagnosticsService::versionString() const {
    return "MBootCore " MBOOTCORE_VERSION;
}

} // namespace runtime
} // namespace mbootcore
