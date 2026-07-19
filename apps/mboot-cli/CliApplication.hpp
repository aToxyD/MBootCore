#pragma once

#include "CliParser.hpp"
#include "CliFormatter.hpp"
#include "CliProgress.hpp"
#include "CliCommands.hpp"

#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeFactory.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>
#include <mbootcore/logging/ConsoleLogger.hpp>

#include <memory>
#include <string>
#include <functional>

namespace mboot {
namespace cli {

class CliApplication {
public:
    using Runtime = mbootcore::runtime::Runtime;

    CliApplication();
    explicit CliApplication(Runtime runtime);
    ~CliApplication();

    int run(const std::vector<std::string>& args);
    int runInteractive();
    int runScript(const std::string& path);

    void setVerbose(bool v) { m_verbose = v; }
    void setQuiet(bool q) { m_quiet = q; }
    void setOutputFormat(OutputFormat fmt) { m_formatter.setFormat(fmt); }

    CliParser& parser() { return m_parser; }
    CliFormatter& formatter() { return m_formatter; }
    CliProgress& progress() { return m_progress; }

private:
    int executeCommand(const std::string& line);
    int executeParsed(const ParsedCommand& cmd);
    void setupCallbacks();
    void setupProgressCallbacks();

    std::string envConfig() const;
    std::string envConfigFile() const;

    Runtime m_runtime;
    CliParser m_parser;
    CliFormatter m_formatter;
    CliProgress m_progress;
    CliCommands m_commands;

    bool m_verbose{false};
    bool m_quiet{false};
    bool m_interactive{false};
};

} // namespace cli
} // namespace mboot
