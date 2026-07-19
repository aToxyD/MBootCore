#include "CliApplication.hpp"

#include <mbootcore/runtime/RuntimeConfig.hpp>
#include <mbootcore/domain/Error.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {

std::string safeGetenv(const char* name) {
#ifdef _WIN32
    char buf[1024];
    DWORD len = ::GetEnvironmentVariableA(name, buf, sizeof(buf));
    return (len > 0 && len < sizeof(buf)) ? std::string(buf, len) : std::string();
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : std::string();
#endif
}

} // anonymous namespace

namespace mboot {
namespace cli {

CliApplication::CliApplication()
    : m_runtime(mbootcore::runtime::RuntimeFactory::createCLI())
    , m_commands(m_runtime, m_parser, m_formatter) {}

CliApplication::CliApplication(Runtime runtime)
    : m_runtime(std::move(runtime))
    , m_commands(m_runtime, m_parser, m_formatter) {}

CliApplication::~CliApplication() {
    if (m_runtime.isInitialized()) {
        m_runtime.shutdown();
    }
}

std::string CliApplication::envConfig() const {
    return safeGetenv("MBOOT_CONFIG");
}

std::string CliApplication::envConfigFile() const {
    return safeGetenv("MBOOT_CONFIG_FILE");
}

void CliApplication::setupCallbacks() {
    mbootcore::runtime::RuntimeCallbacks cbs;
    cbs.onLog = [this](const std::string& msg) {
        if (!m_quiet) std::cerr << "[LOG] " << msg << std::endl;
    };
    cbs.onError = [](const std::string& msg) {
        std::cerr << "[ERROR] " << msg << std::endl;
    };
    cbs.onWarning = [this](const std::string& msg) {
        if (!m_quiet) std::cerr << "[WARN] " << msg << std::endl;
    };
    cbs.onStatus = [this](const std::string& msg) {
        if (!m_quiet && m_verbose) std::cout << msg << std::endl;
    };
    m_runtime.setCallbacks(cbs);
}

void CliApplication::setupProgressCallbacks() {
    mbootcore::runtime::RuntimeCallbacks cbs = m_runtime.callbacks();
    cbs.onProgress = [this](const std::string& op, double pct) {
        (void)op;
        if (m_progress.isEnabled()) {
            m_progress.update(static_cast<uint64_t>(pct));
        }
    };
    m_runtime.setCallbacks(cbs);
}

int CliApplication::executeCommand(const std::string& line) {
    auto trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
    if (trimmed.empty()) return 0;
    if (trimmed[0] == '#' || trimmed[0] == ';') return 0;

    auto cmd = m_parser.parseLine(trimmed);
    return executeParsed(cmd);
}

int CliApplication::executeParsed(const ParsedCommand& cmd) {
    std::string result = m_commands.execute(cmd);
    if (!m_quiet) {
        std::cout << result << std::endl;
    }
    if (cmd.options.json) {
        m_formatter.setFormat(OutputFormat::Json);
    }
    return 0;
}

int CliApplication::run(const std::vector<std::string>& args) {
    if (args.empty()) {
        return runInteractive();
    }

    setupCallbacks();

    auto cmd = m_parser.parse(args);

    if (cmd.options.verbose) setVerbose(true);
    if (cmd.options.quiet) setQuiet(true);
    if (cmd.options.json) m_formatter.setFormat(OutputFormat::Json);
    if (cmd.options.xml) m_formatter.setFormat(OutputFormat::Xml);
    if (!cmd.options.progress) m_progress.setEnabled(false);

    if (cmd.type == CommandType::Script) {
        if (cmd.args.empty()) {
            std::cerr << "Usage: mboot script <file>" << std::endl;
            return 2;
        }
        return runScript(cmd.args[0]);
    }

    if (cmd.type == CommandType::Shell) {
        return runInteractive();
    }

    return executeParsed(cmd);
}

int CliApplication::runInteractive() {
    if (!m_runtime.isInitialized()) {
        auto r = m_runtime.initialize();
        if (r.isError()) {
            std::cerr << "Failed to initialize: " << mbootcore::toString(r.error()) << std::endl;
            return 1;
        }
    }

    setupCallbacks();
    m_interactive = true;
    std::cout << m_formatter.welcomeText() << std::endl;
    std::cout << std::endl;

    std::string line;
    std::vector<std::string> history;

    while (true) {
        std::cout << "mboot> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        if (trimmed.empty()) continue;

        if (trimmed == "exit" || trimmed == "quit") {
            std::cout << "Bye!" << std::endl;
            break;
        }

        history.push_back(trimmed);
        executeCommand(trimmed);
    }

    m_interactive = false;
    return 0;
}

int CliApplication::runScript(const std::string& path) {
    if (!m_runtime.isInitialized()) {
        auto r = m_runtime.initialize();
        if (r.isError()) {
            std::cerr << "Failed to initialize: " << mbootcore::toString(r.error()) << std::endl;
            return 1;
        }
    }

    setupCallbacks();
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open script: " << path << std::endl;
        return 2;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        ++lineNum;
        auto cmd = m_parser.parseLine(line);
        if (cmd.type == CommandType::Help && cmd.args.empty()) continue;

        auto trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        if (m_verbose) {
            std::cerr << "[line " << lineNum << "] " << trimmed << std::endl;
        }
        executeCommand(trimmed);
    }
    return 0;
}

} // namespace cli
} // namespace mboot
