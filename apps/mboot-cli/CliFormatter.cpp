#include "CliFormatter.hpp"

#include <iomanip>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace mboot {
namespace cli {

using namespace mbootcore::discovery;
using namespace mbootcore::runtime;

CliFormatter::CliFormatter(OutputFormat fmt) : m_format(fmt) {}

std::string CliFormatter::jsonEscape(const std::string& s) const {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

std::string CliFormatter::xmlEscape(const std::string& s) const {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string CliFormatter::formatDevices(const std::vector<DeviceDescriptor>& devices) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < devices.size(); ++i) {
            if (i > 0) ss << ",";
            ss << formatDevice(devices[i]);
        }
        ss << "]";
        return ss.str();
    }
    if (m_format == OutputFormat::Xml) {
        std::ostringstream ss;
        ss << "<devices>";
        for (auto& d : devices) ss << formatDevice(d);
        ss << "</devices>";
        return ss.str();
    }
    std::ostringstream ss;
    for (size_t i = 0; i < devices.size(); ++i) {
        auto& d = devices[i];
        ss << "[" << i << "] " << d.friendlyName
           << " (vendor=" << static_cast<int>(d.vendor)
           << ", protocol=" << static_cast<int>(d.protocolHint) << ")";
        if (i + 1 < devices.size()) ss << "\n";
    }
    return ss.str();
}

std::string CliFormatter::formatDevice(const DeviceDescriptor& d) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"friendlyName\":\"" << jsonEscape(d.friendlyName)
           << "\",\"vendor\":" << static_cast<int>(d.vendor)
           << ",\"protocol\":" << static_cast<int>(d.protocolHint)
           << ",\"vid\":\"" << d.usbVid << "\""
           << ",\"pid\":\"" << d.usbPid << "\"}";
        return ss.str();
    }
    if (m_format == OutputFormat::Xml) {
        std::ostringstream ss;
        ss << "<device><name>" << xmlEscape(d.friendlyName)
           << "</name><vendor>" << static_cast<int>(d.vendor)
           << "</vendor><protocol>" << static_cast<int>(d.protocolHint)
           << "</protocol><usbVid>" << d.usbVid << "</usbVid>"
           << "<usbPid>" << d.usbPid << "</usbPid></device>";
        return ss.str();
    }
    std::ostringstream ss;
    ss << d.friendlyName << " (vendor=" << static_cast<int>(d.vendor)
       << ", protocol=" << static_cast<int>(d.protocolHint) << ")";
    return ss.str();
}

std::string CliFormatter::formatStatistics(const RuntimeStatistics& stats) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"devicesConnected\":" << stats.devicesConnected
           << ",\"workflowsExecuted\":" << stats.workflowsExecuted
           << ",\"jobsExecuted\":" << stats.jobsExecuted
           << ",\"packagesInstalled\":" << stats.packagesInstalled
           << ",\"pluginsLoaded\":" << stats.pluginsLoaded
           << ",\"vendorsLoaded\":" << stats.vendorsLoaded
           << ",\"totalErrors\":" << stats.totalErrors
           << ",\"totalRecoveries\":" << stats.totalRecoveries
           << ",\"uptimeSeconds\":" << stats.uptimeSeconds
           << ",\"averageFlashSpeedBps\":" << stats.averageFlashSpeedBps
           << "}";
        return ss.str();
    }
    if (m_format == OutputFormat::Xml) {
        std::ostringstream ss;
        ss << "<statistics>"
           << "<devicesConnected>" << stats.devicesConnected << "</devicesConnected>"
           << "<workflowsExecuted>" << stats.workflowsExecuted << "</workflowsExecuted>"
           << "<jobsExecuted>" << stats.jobsExecuted << "</jobsExecuted>"
           << "<packagesInstalled>" << stats.packagesInstalled << "</packagesInstalled>"
           << "<uptimeSeconds>" << stats.uptimeSeconds << "</uptimeSeconds>"
           << "</statistics>";
        return ss.str();
    }
    std::ostringstream ss;
    ss << "Statistics:\n"
       << "  Devices connected: " << stats.devicesConnected << "\n"
       << "  Workflows executed: " << stats.workflowsExecuted << "\n"
       << "  Jobs executed: " << stats.jobsExecuted << "\n"
       << "  Packages installed: " << stats.packagesInstalled << "\n"
       << "  Plugins loaded: " << stats.pluginsLoaded << "\n"
       << "  Vendors loaded: " << stats.vendorsLoaded << "\n"
       << "  Total errors: " << stats.totalErrors << "\n"
       << "  Total recoveries: " << stats.totalRecoveries << "\n"
       << "  Uptime: " << std::fixed << std::setprecision(1) << stats.uptimeSeconds << "s";
    return ss.str();
}

std::string CliFormatter::formatHealth(const RuntimeHealth& health) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"activeSessions\":" << health.activeSessions
           << ",\"connectedDevices\":" << health.connectedDevices
           << ",\"loadedPlugins\":" << health.loadedPlugins
           << ",\"uptimeSeconds\":" << health.uptimeSeconds
           << ",\"transportState\":\"" << jsonEscape(health.transportState) << "\"}";
        return ss.str();
    }
    if (m_format == OutputFormat::Xml) {
        std::ostringstream ss;
        ss << "<health>"
           << "<activeSessions>" << health.activeSessions << "</activeSessions>"
           << "<connectedDevices>" << health.connectedDevices << "</connectedDevices>"
           << "<uptimeSeconds>" << health.uptimeSeconds << "</uptimeSeconds>"
           << "<transportState>" << xmlEscape(health.transportState) << "</transportState>"
           << "</health>";
        return ss.str();
    }
    std::ostringstream ss;
    ss << "Health:\n"
       << "  Active sessions: " << health.activeSessions << "\n"
       << "  Connected devices: " << health.connectedDevices << "\n"
       << "  Loaded plugins: " << health.loadedPlugins << "\n"
       << "  Queued jobs: " << health.queuedJobs << "\n"
       << "  Transport: " << health.transportState << "\n"
       << "  Uptime: " << std::fixed << std::setprecision(1) << health.uptimeSeconds << "s";
    return ss.str();
}

std::string CliFormatter::formatEvent(const RuntimeEvent& event) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"type\":" << static_cast<int>(event.type)
           << ",\"message\":\"" << jsonEscape(event.message)
           << "\",\"success\":" << (event.success ? "true" : "false")
           << ",\"error\":" << static_cast<int>(event.error) << "}";
        return ss.str();
    }
    std::ostringstream ss;
    ss << "[" << static_cast<int>(event.type) << "] " << event.message;
    return ss.str();
}

std::string CliFormatter::formatVersion(const std::string& version) const {
    if (m_format == OutputFormat::Json) {
        return "{\"version\":\"" + jsonEscape(version) + "\"}";
    }
    return "MBootCore CLI v" + version;
}

std::string CliFormatter::formatCapabilities(const std::vector<std::string>& caps) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "[\"";
        for (size_t i = 0; i < caps.size(); ++i) {
            if (i > 0) ss << "\",\"";
            ss << jsonEscape(caps[i]);
        }
        ss << "\"]";
        return ss.str();
    }
    return formatList(caps);
}

std::string CliFormatter::formatPlugins(const std::vector<std::string>& plugins) const {
    return formatList(plugins);
}

std::string CliFormatter::formatResult(const std::string& op, bool success, const std::string& msg) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"operation\":\"" << jsonEscape(op)
           << "\",\"success\":" << (success ? "true" : "false")
           << ",\"message\":\"" << jsonEscape(msg) << "\"}";
        return ss.str();
    }
    if (m_format == OutputFormat::Xml) {
        std::ostringstream ss;
        ss << "<result operation=\"" << xmlEscape(op) << "\">"
           << "<success>" << (success ? "true" : "false") << "</success>"
           << "<message>" << xmlEscape(msg) << "</message></result>";
        return ss.str();
    }
    std::ostringstream ss;
    ss << (success ? "[OK] " : "[FAIL] ") << op;
    if (!msg.empty()) ss << " - " << msg;
    return ss.str();
}

std::string CliFormatter::formatError(const std::string& op, const std::string& error, int exitCode) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"error\":\"" << jsonEscape(error)
           << "\",\"operation\":\"" << jsonEscape(op)
           << "\",\"exitCode\":" << exitCode << "}";
        return ss.str();
    }
    std::ostringstream ss;
    ss << "Error [" << exitCode << "]: " << error;
    if (!op.empty()) ss << " (during: " << op << ")";
    return ss.str();
}

std::string CliFormatter::formatProgress(const std::string& op, double pct,
                                          uint64_t bytes, double speed, int eta) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "{\"operation\":\"" << jsonEscape(op)
           << "\",\"percent\":" << pct
           << ",\"bytes\":" << bytes
           << ",\"speedBps\":" << speed
           << ",\"etaSeconds\":" << eta << "}";
        return ss.str();
    }
    std::ostringstream ss;
    ss << "\r" << op << ": " << std::fixed << std::setprecision(1) << pct << "%"
       << " [" << bytes << " bytes";
    if (speed > 0) ss << " @ " << std::fixed << std::setprecision(0) << speed << " B/s";
    if (eta > 0) ss << " ETA " << eta << "s";
    ss << "]";
    return ss.str();
}

std::string CliFormatter::formatString(const std::string& key, const std::string& value) const {
    if (m_format == OutputFormat::Json) {
        return "{\"" + jsonEscape(key) + "\":\"" + jsonEscape(value) + "\"}";
    }
    return key + ": " + value;
}

std::string CliFormatter::formatList(const std::vector<std::string>& items) const {
    if (m_format == OutputFormat::Json) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) ss << ",";
            ss << "\"" << jsonEscape(items[i]) << "\"";
        }
        ss << "]";
        return ss.str();
    }
    if (items.empty()) {
        return "(none)";
    }
    std::ostringstream ss;
    for (size_t i = 0; i < items.size(); ++i) {
        ss << "  " << items[i];
        if (i + 1 < items.size()) ss << "\n";
    }
    return ss.str();
}

std::string CliFormatter::helpText() const {
    return
        std::string("MBoot CLI v") + MBOOTCORE_VERSION + " - BootROM Flashing Tool\n"
        "========================================\n"
        "\n"
        "Usage: mboot <command> [options]\n"
        "\n"
        "Commands:\n"
        "  discover                 Scan for devices\n"
        "  list                     List available commands\n"
        "  connect <index>          Connect to device\n"
        "  disconnect               Disconnect device\n"
        "  flash <file>             Flash firmware\n"
        "  read <addr> <size>       Read memory\n"
        "  erase <partition>        Erase partition\n"
        "  verify <part> <file>     Verify partition\n"
        "  backup <part> <file>     Backup partition\n"
        "  restore <part> <file>    Restore partition\n"
        "  workflow <type|file>     Execute workflow\n"
        "  job <name> <part> <file> Run flash job\n"
        "  stats                    Show statistics\n"
        "  health                   Show system health\n"
        "  version                  Show version\n"
        "  help                     Show this help\n"
        "  shell                    Interactive mode\n"
        "  script <file>            Run script\n"
        "\n"
        "Options:\n"
        "  --json        JSON output\n"
        "  --xml         XML output\n"
        "  --verbose     Verbose output\n"
        "  --quiet       Suppress output\n"
        "  --timeout=ms  Operation timeout\n"
        "  --retry=n     Retry count\n"
        "  --dry-run     Simulate only\n"
        "  --force       Force operation\n"
        "\n"
        "Exit codes: 0=OK, 1=Error, 2=Bad args, 3=Not found,\n"
        "  4=Transport, 5=Workflow, 6=Flash, 7=Verify, 8=Timeout,\n"
        "  9=Cancelled, 10=Internal\n";
}

std::string CliFormatter::welcomeText() const {
    return std::string("MBoot CLI v") + MBOOTCORE_VERSION + " - Type 'help' for commands, 'exit' to quit.";
}

} // namespace cli
} // namespace mboot
