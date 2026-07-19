#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mboot {
namespace cli {

enum class CommandType {
    Help, Version, List, Discover,
    Connect, Disconnect, Reconnect,
    Flash, Read, Write, Erase, Verify,
    Backup, Restore,
    Workflow, Job,
    Package,
    Plugin, Vendor, Transport, Session, Monitor,
    Statistics, Health, Capabilities,
    Reset, Cancel, Pause, Resume, Reboot, Shutdown,
    Shell, Script, Completion,
    Invalid
};

struct CliOptions {
    std::string device;
    std::string vendor;
    std::string transport;
    std::string protocol;
    std::string partition;
    uint64_t address{0};
    size_t size{0};
    int timeoutMs{5000};
    int retry{3};
    bool verbose{false};
    bool quiet{false};
    bool json{false};
    bool xml{false};
    bool progress{true};
    bool force{false};
    bool dryRun{false};
};

struct ParsedCommand {
    CommandType type{CommandType::Invalid};
    std::vector<std::string> args;
    CliOptions options;
    std::string rawLine;
};

class CliParser {
public:
    ParsedCommand parse(const std::vector<std::string>& argv) const;
    ParsedCommand parseLine(const std::string& line) const;
    std::vector<std::string> tokenize(const std::string& line) const;
    std::string usage() const;
    std::string commandHelp() const;
    std::string optionHelp() const;

private:
    CommandType resolveCommand(const std::string& name) const;
    CliOptions parseOptions(std::vector<std::string>& args) const;
    bool hasPrefix(const std::string& arg, const std::string& prefix) const;
};

} // namespace cli
} // namespace mboot
