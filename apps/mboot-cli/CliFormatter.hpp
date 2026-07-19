#pragma once

#include <mbootcore/runtime/RuntimeStatistics.hpp>
#include <mbootcore/runtime/RuntimeEvents.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace mboot {
namespace cli {

enum class OutputFormat {
    Human,
    Json,
    Xml,
    Plain
};

class CliFormatter {
public:
    explicit CliFormatter(OutputFormat fmt = OutputFormat::Human);

    using DeviceDescriptor = mbootcore::discovery::DeviceDescriptor;
    using RuntimeStatistics = mbootcore::runtime::RuntimeStatistics;
    using RuntimeHealth = mbootcore::runtime::RuntimeHealth;
    using RuntimeEvent = mbootcore::runtime::RuntimeEvent;

    std::string formatDevices(const std::vector<DeviceDescriptor>& devices) const;
    std::string formatDevice(const DeviceDescriptor& device) const;
    std::string formatStatistics(const RuntimeStatistics& stats) const;
    std::string formatHealth(const RuntimeHealth& health) const;
    std::string formatEvent(const RuntimeEvent& event) const;
    std::string formatVersion(const std::string& version) const;
    std::string formatCapabilities(const std::vector<std::string>& caps) const;
    std::string formatPlugins(const std::vector<std::string>& plugins) const;
    std::string formatResult(const std::string& operation, bool success, const std::string& msg) const;
    std::string formatError(const std::string& operation, const std::string& error, int exitCode) const;
    std::string formatProgress(const std::string& op, double pct, uint64_t bytes, double speed, int eta) const;
    std::string formatString(const std::string& key, const std::string& value) const;
    std::string formatList(const std::vector<std::string>& items) const;

    void setFormat(OutputFormat fmt) { m_format = fmt; }
    OutputFormat format() const { return m_format; }

    std::string helpText() const;
    std::string welcomeText() const;

private:
    std::string jsonEscape(const std::string& s) const;
    std::string xmlEscape(const std::string& s) const;

    OutputFormat m_format;
};

} // namespace cli
} // namespace mboot
