#include "CliCommands.hpp"

#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/job/GenericJobs.hpp>

#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace mboot {
namespace cli {

CliCommands::CliCommands(Runtime& runtime, CliParser& parser, CliFormatter& formatter)
    : m_runtime(runtime), m_parser(parser), m_formatter(formatter) {}

void CliCommands::ensureInitialized() {
    if (!m_runtime.isInitialized()) {
        auto result = m_runtime.initialize();
        if (result.isError()) {
            throw std::runtime_error("Failed to initialize runtime: " +
                std::string(mbootcore::toString(result.error())));
        }
    }
}

std::string CliCommands::getError(const std::string& op, const mbootcore::ErrorCode& ec, const std::string& msg) {
    m_lastOp = op;
    auto fullMsg = msg.empty() ? std::string(mbootcore::toString(ec)) : msg;
    return m_formatter.formatError(op, fullMsg, exitCodeFor(ec));
}

int CliCommands::exitCodeFor(const mbootcore::ErrorCode& ec) const {
    using EC = mbootcore::ErrorCode;
    switch (ec) {
        case EC::Success: return 0;
        case EC::InvalidArgument: return 2;
        case EC::DeviceNotFound: return 3;
        case EC::TransportError:
        case EC::TransportTimeout:
        case EC::TransportDisconnected:
        case EC::TransportWriteFailed:
        case EC::TransportReadFailed:
        case EC::TransportNotOpen:
        case EC::TransportAlreadyOpen:
        case EC::TransportBusy:
        case EC::TransportReconnectFailed:
            return 4;
        case EC::WorkflowExecutionFailed:
        case EC::WorkflowStepFailed:
        case EC::WorkflowRecoveryFailed:
            return 5;
        case EC::FirehoseProgramFailed:
        case EC::FirehoseEraseFailed:
        case EC::FirmwareValidationFailed:
            return 6;
        case EC::FirmwareHashMismatch:
            return 7;
        case EC::SessionTimeout:
            return 8;
        case EC::Cancelled:
            return 9;
        default: return 1;
    }
}

std::string CliCommands::execute(const ParsedCommand& cmd) {
    try {
        ensureInitialized();
    } catch (const std::exception& e) {
        return m_formatter.formatError("init", e.what(), 10);
    }

    switch (cmd.type) {
        case CommandType::Help:        return help(cmd);
        case CommandType::Version:     return version(cmd);
        case CommandType::List:        return list(cmd);
        case CommandType::Discover:    return discover(cmd);
        case CommandType::Connect:     return connect(cmd);
        case CommandType::Disconnect:  return disconnect(cmd);
        case CommandType::Reconnect:   return reconnect(cmd);
        case CommandType::Flash:       return flash(cmd);
        case CommandType::Read:        return read(cmd);
        case CommandType::Write:       return write(cmd);
        case CommandType::Erase:       return erase(cmd);
        case CommandType::Verify:      return verify(cmd);
        case CommandType::Backup:      return backup(cmd);
        case CommandType::Restore:     return restore(cmd);
        case CommandType::Workflow:    return workflow(cmd);
        case CommandType::Job:         return job(cmd);
        case CommandType::Package:     return package(cmd);
        case CommandType::Plugin:      return plugin(cmd);
        case CommandType::Vendor:      return vendor(cmd);
        case CommandType::Transport:   return transport(cmd);
        case CommandType::Session:     return session(cmd);
        case CommandType::Monitor:     return monitor(cmd);
        case CommandType::Statistics:  return statistics(cmd);
        case CommandType::Health:      return healthCmd(cmd);
        case CommandType::Capabilities: return capabilities(cmd);
        case CommandType::Reset:       return reset(cmd);
        case CommandType::Cancel:      return cancel(cmd);
        case CommandType::Pause:       return pause(cmd);
        case CommandType::Resume:      return resume(cmd);
        case CommandType::Reboot:      return reboot(cmd);
        case CommandType::Shutdown:    return shutdown(cmd);
        case CommandType::Shell:       return shell(cmd);
        case CommandType::Script:      return script(cmd);
        case CommandType::Completion:  return completion(cmd);
        default: return getError("execute", mbootcore::ErrorCode::Unknown, "Unknown command");
    }
}

std::string CliCommands::help(const ParsedCommand&) {
    return m_formatter.helpText();
}

std::string CliCommands::version(const ParsedCommand&) {
    return m_formatter.formatVersion(MBOOTCORE_VERSION);
}

std::string CliCommands::list(const ParsedCommand&) {
    return m_parser.commandHelp();
}

std::string CliCommands::discover(const ParsedCommand&) {
    auto result = m_runtime.discover(std::chrono::seconds(5));
    if (result.isError()) {
        return getError("discover", result.error());
    }
    return m_formatter.formatDevices(result.value());
}

std::string CliCommands::connect(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("connect", mbootcore::ErrorCode::InvalidArgument, "Usage: connect <index>");
    }
    size_t idx = 0;
    try { idx = std::stoul(cmd.args[0]); }
    catch (...) { return getError("connect", mbootcore::ErrorCode::InvalidArgument, "Invalid index"); }

    auto devices = m_runtime.discover(std::chrono::seconds(5));
    if (devices.isError()) return getError("discover", devices.error());
    if (idx >= devices.value().size()) {
        return getError("connect", mbootcore::ErrorCode::DeviceNotFound,
            "Index " + std::to_string(idx) + " out of range (0-" +
            std::to_string(devices.value().size() - 1) + ")");
    }
    auto result = m_runtime.connect(devices.value()[idx]);
    if (result.isError()) return getError("connect", result.error());
    return m_formatter.formatResult("connect", true, "Connected to " + devices.value()[idx].friendlyName);
}

std::string CliCommands::disconnect(const ParsedCommand&) {
    m_runtime.disconnect();
    return m_formatter.formatResult("disconnect", true, "Disconnected");
}

std::string CliCommands::reconnect(const ParsedCommand&) {
    auto result = m_runtime.reconnect();
    if (result.isError()) return getError("reconnect", result.error());
    return m_formatter.formatResult("reconnect", true, "Reconnected");
}

std::string CliCommands::flash(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("flash", mbootcore::ErrorCode::InvalidArgument, "Usage: flash <package_path>");
    }
    std::string path = cmd.args[0];
    if (cmd.options.dryRun) {
        return m_formatter.formatResult("flash (dry-run)", true,
            "Would flash package: " + path);
    }

    auto pkg = m_runtime.loadFirmwarePackage(path);
    if (pkg.isError()) return getError("flash", pkg.error());

    auto result = m_runtime.flash(*pkg.value());
    if (result.isError()) return getError("flash", result.error());
    return m_formatter.formatResult("flash", true, "Flashed " + path);
}

std::string CliCommands::read(const ParsedCommand& cmd) {
    if (cmd.args.size() < 2) {
        return getError("read", mbootcore::ErrorCode::InvalidArgument,
            "Usage: read <address> <size>");
    }
    uint64_t addr = 0; size_t size = 0;
    try {
        addr = std::stoull(cmd.args[0], nullptr, 0);
        size = static_cast<size_t>(std::stoul(cmd.args[1]));
    } catch (...) {
        return getError("read", mbootcore::ErrorCode::InvalidArgument, "Invalid address/size");
    }
    auto result = m_runtime.read(addr, size);
    if (result.isError()) return getError("read", result.error());
    return m_formatter.formatResult("read", true,
        "Read " + std::to_string(result.value().size()) + " bytes from 0x" +
        std::to_string(addr));
}

std::string CliCommands::write(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("write", mbootcore::ErrorCode::InvalidArgument,
            "Usage: write <address> <hex_data>");
    }
    uint64_t addr = 0;
    try { addr = std::stoull(cmd.args[0], nullptr, 0); }
    catch (...) { return getError("write", mbootcore::ErrorCode::InvalidArgument, "Invalid address"); }
    mbootcore::ByteBuffer data;
    if (cmd.args.size() > 1) {
        auto hex = cmd.args[1];
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            data.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        }
    }
    auto result = m_runtime.write(addr, data);
    if (result.isError()) return getError("write", result.error());
    return m_formatter.formatResult("write", true,
        "Wrote " + std::to_string(data.size()) + " bytes to 0x" + std::to_string(addr));
}

std::string CliCommands::erase(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("erase", mbootcore::ErrorCode::InvalidArgument,
            "Usage: erase <address> [size]");
    }
    uint64_t addr = 0; size_t size = 4096;
    try {
        addr = std::stoull(cmd.args[0], nullptr, 0);
        if (cmd.args.size() > 1) size = static_cast<size_t>(std::stoul(cmd.args[1]));
    } catch (...) {
        return getError("erase", mbootcore::ErrorCode::InvalidArgument, "Invalid address/size");
    }
    auto result = m_runtime.erase(addr, size);
    if (result.isError()) return getError("erase", result.error());
    return m_formatter.formatResult("erase", true,
        "Erased " + std::to_string(size) + " bytes at 0x" + std::to_string(addr));
}

std::string CliCommands::verify(const ParsedCommand& cmd) {
    if (cmd.args.size() < 2) {
        return getError("verify", mbootcore::ErrorCode::InvalidArgument,
            "Usage: verify <address> <expected_hex>");
    }
    uint64_t addr = 0;
    try { addr = std::stoull(cmd.args[0], nullptr, 0); }
    catch (...) { return getError("verify", mbootcore::ErrorCode::InvalidArgument, "Invalid address"); }
    mbootcore::ByteBuffer expected;
    auto hex = cmd.args[1];
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        expected.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    auto result = m_runtime.verify(addr, expected);
    if (result.isError()) return getError("verify", result.error());
    return m_formatter.formatResult("verify", true, "Verification passed");
}

std::string CliCommands::backup(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("backup", mbootcore::ErrorCode::InvalidArgument,
            "Usage: backup <partition> [output_file]");
    }
    auto result = m_runtime.backup(cmd.args[0]);
    if (result.isError()) return getError("backup", result.error());

    if (cmd.args.size() > 1) {
        std::ofstream ofs(cmd.args[1], std::ios::binary);
        if (!ofs) return getError("backup", mbootcore::ErrorCode::Unknown, "Cannot open output file");
        ofs.write(reinterpret_cast<const char*>(result.value().data()), static_cast<std::streamsize>(result.value().size()));
    }
    return m_formatter.formatResult("backup", true,
        "Backed up " + cmd.args[0] + " (" + std::to_string(result.value().size()) + " bytes)");
}

std::string CliCommands::restore(const ParsedCommand& cmd) {
    if (cmd.args.size() < 2) {
        return getError("restore", mbootcore::ErrorCode::InvalidArgument,
            "Usage: restore <partition> <input_file>");
    }
    std::ifstream ifs(cmd.args[1], std::ios::binary | std::ios::ate);
    if (!ifs) return getError("restore", mbootcore::ErrorCode::Unknown, "Cannot open input file");
    auto size = ifs.tellg();
    ifs.seekg(0);
    mbootcore::ByteBuffer data(static_cast<size_t>(size));
    ifs.read(reinterpret_cast<char*>(data.data()), size);
    auto result = m_runtime.restore(cmd.args[0], data);
    if (result.isError()) return getError("restore", result.error());
    return m_formatter.formatResult("restore", true,
        "Restored " + cmd.args[0] + " (" + std::to_string(data.size()) + " bytes)");
}

std::string CliCommands::workflow(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("workflow", mbootcore::ErrorCode::InvalidArgument,
            "Usage: workflow <type>");
    }
    auto result = m_runtime.executeWorkflow(cmd.args[0]);
    if (result.isError()) return getError("workflow", result.error());
    return m_formatter.formatResult("workflow", true, "Executed " + cmd.args[0]);
}

std::string CliCommands::job(const ParsedCommand& cmd) {
    if (cmd.args.size() < 3) {
        return getError("job", mbootcore::ErrorCode::InvalidArgument,
            "Usage: job <name> <partition> <file>");
    }
    std::ifstream ifs(cmd.args[2], std::ios::binary | std::ios::ate);
    if (!ifs) {
        return getError("job", mbootcore::ErrorCode::Unknown, "Cannot open file: " + cmd.args[2]);
    }
    auto fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    mbootcore::ByteBuffer data(static_cast<size_t>(fileSize));
    ifs.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(fileSize));

    auto flashJob = std::make_unique<mbootcore::job::FlashJob>(
        cmd.args[0], cmd.args[1], std::move(data));
    auto result = m_runtime.runJob(std::move(flashJob));
    if (result.isError()) return getError("job", result.error());
    return m_formatter.formatResult("job", true, "Job " + cmd.args[0] + " completed");
}

std::string CliCommands::package(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return m_formatter.formatResult("package", false, "Usage: package info <path>");
    }
    if (cmd.args[0] == "info" && cmd.args.size() > 1) {
        auto result = m_runtime.loadFirmwarePackage(cmd.args[1]);
        if (result.isError()) return getError("package", result.error());
        auto* pkg = result.value().get();
        return m_formatter.formatString("package", cmd.args[1]) + "\n" +
               m_formatter.formatString("images", std::to_string(pkg->images().size()));
    }
    return getError("package", mbootcore::ErrorCode::InvalidArgument);
}

std::string CliCommands::plugin(const ParsedCommand& cmd) {
    if (!cmd.args.empty() && cmd.args[0] == "list") {
        auto plugins = m_runtime.listPlugins();
        return m_formatter.formatPlugins(plugins);
    }
    auto plugins = m_runtime.listPlugins();
    return m_formatter.formatPlugins(plugins);
}

std::string CliCommands::vendor(const ParsedCommand& cmd) {
    (void)cmd;
    auto vendors = m_runtime.listPlugins();
    if (vendors.empty()) {
        return m_formatter.formatResult("vendor", true, "No vendors registered");
    }
    return m_formatter.formatList(vendors);
}

std::string CliCommands::transport(const ParsedCommand& cmd) {
    (void)cmd;
    auto ids = m_runtime.transportManager().ids();
    if (ids.empty()) {
        return m_formatter.formatResult("transport", true, "No transports available");
    }
    return m_formatter.formatList(ids);
}

std::string CliCommands::session(const ParsedCommand& cmd) {
    (void)cmd;
    auto* session = m_runtime.activeSession();
    if (!session) return m_formatter.formatResult("session", true, "No active session");
    return m_formatter.formatString("session", "Active");
}

std::string CliCommands::monitor(const ParsedCommand&) {
    return getError("monitor", mbootcore::ErrorCode::NotSupported, "Monitoring is not supported in this version");
}

std::string CliCommands::statistics(const ParsedCommand&) {
    auto stats = m_runtime.statistics();
    return m_formatter.formatStatistics(stats);
}

std::string CliCommands::healthCmd(const ParsedCommand&) {
    auto health = m_runtime.health();
    return m_formatter.formatHealth(health);
}

std::string CliCommands::capabilities(const ParsedCommand&) {
    auto caps = m_runtime.capabilities();
    return m_formatter.formatCapabilities(caps);
}

std::string CliCommands::reset(const ParsedCommand&) {
    auto result = m_runtime.reset();
    if (result.isError()) return getError("reset", result.error());
    return m_formatter.formatResult("reset", true, "Runtime reset");
}

std::string CliCommands::cancel(const ParsedCommand&) {
    m_runtime.cancel();
    return m_formatter.formatResult("cancel", true, "Operation cancelled");
}

std::string CliCommands::pause(const ParsedCommand&) {
    m_runtime.pause();
    return m_formatter.formatResult("pause", true, "Operation paused");
}

std::string CliCommands::resume(const ParsedCommand&) {
    m_runtime.resume();
    return m_formatter.formatResult("resume", true, "Operation resumed");
}

std::string CliCommands::reboot(const ParsedCommand&) {
    auto result = m_runtime.reset();
    if (result.isError()) return getError("reboot", result.error());
    return m_formatter.formatResult("reboot", true, "Device reboot initiated");
}

std::string CliCommands::shutdown(const ParsedCommand&) {
    m_runtime.shutdown();
    return m_formatter.formatResult("shutdown", true, "Runtime shut down");
}

std::string CliCommands::shell(const ParsedCommand&) {
    return m_formatter.formatResult("shell", true, "Use 'mboot shell' to enter interactive mode");
}

std::string CliCommands::script(const ParsedCommand& cmd) {
    if (cmd.args.empty()) {
        return getError("script", mbootcore::ErrorCode::InvalidArgument, "Usage: script <file>");
    }
    std::ifstream file(cmd.args[0]);
    if (!file) {
        return getError("script", mbootcore::ErrorCode::Unknown, "Cannot open script: " + cmd.args[0]);
    }
    std::string line;
    std::string output;
    int count = 0;
    while (std::getline(file, line)) {
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        auto scmd = m_parser.parseLine(trimmed);
        output += execute(scmd) + "\n";
        ++count;
    }
    output += m_formatter.formatResult("script", true,
        "Executed " + std::to_string(count) + " commands from " + cmd.args[0]);
    return output;
}

std::string CliCommands::completion(const ParsedCommand& cmd) {
    std::string shell = cmd.args.empty() ? "bash" : cmd.args[0];
    std::string script;
    if (shell == "bash") {
        script = "_mboot_completions() {\n"
                 "  local commands=\"help version list discover connect disconnect reconnect flash read write erase verify backup restore workflow job package plugin vendor transport session monitor stats health capabilities reset cancel pause resume reboot shutdown shell script completion\"\n"
                 "  COMPREPLY=($(compgen -W \"$commands\" -- \"${COMP_WORDS[COMP_CWORD]}\"))\n"
                 "}\n"
                 "complete -F _mboot_completions mboot\n";
    } else if (shell == "powershell") {
        script = "Register-ArgumentCompleter -Native -CommandName mboot -ScriptBlock {\n"
                 "  param($wordToComplete, $commandAst, $cursorPosition)\n"
                 "  $commands = @('help','version','list','discover','connect','disconnect','reconnect','flash','read','write','erase','verify','backup','restore','workflow','job','package','plugin','vendor','transport','session','monitor','stats','health','capabilities','reset','cancel','pause','resume','reboot','shutdown','shell','script','completion')\n"
                 "  $commands | Where-Object { $_ -like \"$wordToComplete*\" } | ForEach-Object { \"'$_'\" }\n"
                 "}\n";
    } else if (shell == "zsh") {
        script = "#compdef mboot\n"
                 "_mboot() {\n"
                 "  local commands=(\n"
                 "    'help:Show help'\n"
                 "    'version:Show version'\n"
                 "    'list:List commands'\n"
                 "    'discover:Discover devices'\n"
                 "    'connect:Connect to device'\n"
                 "    'disconnect:Disconnect device'\n"
                 "    'reconnect:Reconnect to device'\n"
                 "    'flash:Flash firmware'\n"
                 "    'read:Read memory'\n"
                 "    'write:Write memory'\n"
                 "    'erase:Erase memory'\n"
                 "    'verify:Verify memory'\n"
                 "    'backup:Backup partition'\n"
                 "    'restore:Restore partition'\n"
                 "    'workflow:Execute workflow'\n"
                 "    'job:Run flash job'\n"
                 "    'package:Package commands'\n"
                 "    'plugin:Plugin commands'\n"
                 "    'vendor:Vendor commands'\n"
                 "    'transport:Transport commands'\n"
                 "    'session:Session commands'\n"
                 "    'monitor:Start monitoring'\n"
                 "    'stats:Show statistics'\n"
                 "    'health:Show health'\n"
                 "    'capabilities:Show capabilities'\n"
                 "    'reset:Reset runtime'\n"
                 "    'cancel:Cancel operation'\n"
                 "    'pause:Pause operation'\n"
                 "    'resume:Resume operation'\n"
                 "    'reboot:Reboot device'\n"
                 "    'shutdown:Shutdown runtime'\n"
                 "    'shell:Start interactive shell'\n"
                 "    'script:Execute script'\n"
                 "    'completion:Generate completion'\n"
                 "  )\n"
                 "  _describe 'mboot' commands\n"
                 "}\n"
                 "_mboot \"$@\"\n";
    } else {
        return getError("completion", mbootcore::ErrorCode::InvalidArgument,
            "Unsupported shell: " + shell + " (supported: bash, powershell, zsh)");
    }
    return script;
}

} // namespace cli
} // namespace mboot
