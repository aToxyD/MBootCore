#include "CliParser.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace mboot {
namespace cli {

static const char* COMMAND_STRINGS[] = {
    "help", "version", "list", "discover",
    "connect", "disconnect", "reconnect",
    "flash", "read", "write", "erase", "verify",
    "backup", "restore",
    "workflow", "job",
    "package",
    "plugin", "vendor", "transport", "session", "monitor",
    "statistics", "stats", "health", "capabilities",
    "reset", "cancel", "pause", "resume", "reboot", "shutdown",
    "shell", "script", "completion",
    nullptr
};

static CommandType COMMAND_TYPES[] = {
    CommandType::Help, CommandType::Version, CommandType::List, CommandType::Discover,
    CommandType::Connect, CommandType::Disconnect, CommandType::Reconnect,
    CommandType::Flash, CommandType::Read, CommandType::Write, CommandType::Erase, CommandType::Verify,
    CommandType::Backup, CommandType::Restore,
    CommandType::Workflow, CommandType::Job,
    CommandType::Package,
    CommandType::Plugin, CommandType::Vendor, CommandType::Transport, CommandType::Session, CommandType::Monitor,
    CommandType::Statistics, CommandType::Statistics, CommandType::Health, CommandType::Capabilities,
    CommandType::Reset, CommandType::Cancel, CommandType::Pause, CommandType::Resume, CommandType::Reboot, CommandType::Shutdown,
    CommandType::Shell, CommandType::Script, CommandType::Completion,
    CommandType::Invalid
};

CommandType CliParser::resolveCommand(const std::string& name) const {
    auto lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (int i = 0; COMMAND_STRINGS[i] != nullptr; ++i) {
        if (lower == COMMAND_STRINGS[i]) return COMMAND_TYPES[i];
    }
    return CommandType::Invalid;
}

bool CliParser::hasPrefix(const std::string& arg, const std::string& prefix) const {
    if (arg.size() < prefix.size()) return false;
    return arg.compare(0, prefix.size(), prefix) == 0;
}

CliOptions CliParser::parseOptions(std::vector<std::string>& args) const {
    CliOptions opts;
    auto it = args.begin();
    while (it != args.end()) {
        if (hasPrefix(*it, "--device=")) {
            opts.device = it->substr(9); it = args.erase(it);
        } else if (hasPrefix(*it, "--vendor=")) {
            opts.vendor = it->substr(9); it = args.erase(it);
        } else if (hasPrefix(*it, "--transport=")) {
            opts.transport = it->substr(12); it = args.erase(it);
        } else if (hasPrefix(*it, "--protocol=")) {
            opts.protocol = it->substr(11); it = args.erase(it);
        } else if (hasPrefix(*it, "--partition=")) {
            opts.partition = it->substr(12); it = args.erase(it);
        } else if (hasPrefix(*it, "--address=")) {
            opts.address = std::stoull(it->substr(10)); it = args.erase(it);
        } else if (hasPrefix(*it, "--size=")) {
            opts.size = static_cast<size_t>(std::stoul(it->substr(7))); it = args.erase(it);
        } else if (hasPrefix(*it, "--timeout=")) {
            opts.timeoutMs = std::stoi(it->substr(10)); it = args.erase(it);
        } else if (hasPrefix(*it, "--retry=")) {
            opts.retry = std::stoi(it->substr(8)); it = args.erase(it);
        } else if (*it == "--verbose") { opts.verbose = true; it = args.erase(it); }
        else if (*it == "--quiet") { opts.quiet = true; it = args.erase(it); }
        else if (*it == "--json") { opts.json = true; it = args.erase(it); }
        else if (*it == "--xml") { opts.xml = true; it = args.erase(it); }
        else if (*it == "--no-progress") { opts.progress = false; it = args.erase(it); }
        else if (*it == "--force") { opts.force = true; it = args.erase(it); }
        else if (*it == "--dry-run") { opts.dryRun = true; it = args.erase(it); }
        else { ++it; }
    }
    return opts;
}

ParsedCommand CliParser::parse(const std::vector<std::string>& argv) const {
    ParsedCommand cmd;
    if (argv.empty()) {
        cmd.type = CommandType::Help;
        return cmd;
    }
    auto args = argv;
    cmd.type = resolveCommand(args[0]);
    args.erase(args.begin());
    cmd.options = parseOptions(args);
    cmd.args = std::move(args);
    return cmd;
}

std::vector<std::string> CliParser::tokenize(const std::string& line) const {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' || c == '\'') {
            inQuote = !inQuote;
        } else if (c == ' ' && !inQuote) {
            if (!current.empty()) { tokens.push_back(current); current.clear(); }
        } else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

ParsedCommand CliParser::parseLine(const std::string& line) const {
    auto tokens = tokenize(line);
    if (tokens.empty()) {
        ParsedCommand cmd;
        cmd.type = CommandType::Help;
        return cmd;
    }
    return parse(tokens);
}

std::string CliParser::usage() const {
    return "mboot <command> [options]\n"
           "Use 'mboot help' for detailed help.";
}

std::string CliParser::commandHelp() const {
    return
        "Commands:\n"
        "  help                     Show this help\n"
        "  version                  Show version information\n"
        "  list                     List available commands\n"
        "  discover                 Discover devices\n"
        "  connect <index>          Connect to device\n"
        "  disconnect               Disconnect current device\n"
        "  reconnect                Reconnect to device\n"
        "  flash <file>             Flash firmware package\n"
        "  read <addr> <size>       Read memory\n"
        "  write <addr> <data>      Write memory\n"
        "  erase <addr> <size>      Erase memory\n"
        "  verify <addr> <file>     Verify memory\n"
        "  backup <part> <file>     Backup partition\n"
        "  restore <part> <file>    Restore partition\n"
        "  workflow <file|type>     Execute workflow\n"
        "  job <name> <part> <file> Run flash job\n"
        "  package info <file>      Show package info\n"
        "  plugin list              List plugins\n"
        "  vendor list              List vendors\n"
        "  transport list           List transports\n"
        "  session list             List sessions\n"
        "  monitor                  Start monitoring (not supported in this version)\n"
        "  stats                    Show statistics\n"
        "  health                   Show health\n"
        "  capabilities             Show capabilities\n"
        "  reset                    Reset runtime\n"
        "  cancel                   Cancel current operation\n"
        "  pause                    Pause current operation\n"
        "  resume                   Resume current operation\n"
        "  reboot                   Reboot device\n"
        "  shutdown                 Shutdown runtime\n"
        "  shell                    Start interactive shell\n"
        "  script <file>            Execute script\n"
        "  completion <shell>       Generate completion\n";
}

std::string CliParser::optionHelp() const {
    return
        "Options:\n"
        "  --device=<id>            Specify device\n"
        "  --vendor=<name>          Specify vendor\n"
        "  --transport=<type>       Specify transport\n"
        "  --protocol=<type>        Specify protocol\n"
        "  --partition=<name>       Specify partition\n"
        "  --address=<hex|dec>      Specify address\n"
        "  --size=<bytes>           Specify size\n"
        "  --timeout=<ms>           Set timeout (ms)\n"
        "  --retry=<n>              Set retry count\n"
        "  --verbose                Verbose output\n"
        "  --quiet                  Quiet output\n"
        "  --json                   JSON output\n"
        "  --xml                    XML output\n"
        "  --no-progress            Disable progress\n"
        "  --force                  Force operation\n"
        "  --dry-run                Dry run\n";
}

} // namespace cli
} // namespace mboot
