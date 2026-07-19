#pragma once

#include "CliParser.hpp"
#include "CliFormatter.hpp"

#include <mbootcore/runtime/Runtime.hpp>

#include <string>
#include <vector>
#include <memory>
namespace mboot {
namespace cli {

class CliCommands {
public:
    using Runtime = mbootcore::runtime::Runtime;

    CliCommands(Runtime& runtime, CliParser& parser, CliFormatter& formatter);

    std::string execute(const ParsedCommand& cmd);

    std::string help(const ParsedCommand& cmd);
    std::string version(const ParsedCommand& cmd);
    std::string list(const ParsedCommand& cmd);
    std::string discover(const ParsedCommand& cmd);
    std::string connect(const ParsedCommand& cmd);
    std::string disconnect(const ParsedCommand& cmd);
    std::string reconnect(const ParsedCommand& cmd);
    std::string flash(const ParsedCommand& cmd);
    std::string read(const ParsedCommand& cmd);
    std::string write(const ParsedCommand& cmd);
    std::string erase(const ParsedCommand& cmd);
    std::string verify(const ParsedCommand& cmd);
    std::string backup(const ParsedCommand& cmd);
    std::string restore(const ParsedCommand& cmd);
    std::string workflow(const ParsedCommand& cmd);
    std::string job(const ParsedCommand& cmd);
    std::string package(const ParsedCommand& cmd);
    std::string plugin(const ParsedCommand& cmd);
    std::string vendor(const ParsedCommand& cmd);
    std::string transport(const ParsedCommand& cmd);
    std::string session(const ParsedCommand& cmd);
    std::string monitor(const ParsedCommand& cmd);
    std::string statistics(const ParsedCommand& cmd);
    std::string healthCmd(const ParsedCommand& cmd);
    std::string capabilities(const ParsedCommand& cmd);
    std::string reset(const ParsedCommand& cmd);
    std::string cancel(const ParsedCommand& cmd);
    std::string pause(const ParsedCommand& cmd);
    std::string resume(const ParsedCommand& cmd);
    std::string reboot(const ParsedCommand& cmd);
    std::string shutdown(const ParsedCommand& cmd);
    std::string shell(const ParsedCommand& cmd);
    std::string script(const ParsedCommand& cmd);
    std::string completion(const ParsedCommand& cmd);

private:
    void ensureInitialized();
    std::string getError(const std::string& op, const mbootcore::ErrorCode& ec, const std::string& msg = {});
    int exitCodeFor(const mbootcore::ErrorCode& ec) const;

    Runtime& m_runtime;
    CliParser& m_parser;
    CliFormatter& m_formatter;
    std::string m_lastOp;
};

} // namespace cli
} // namespace mboot
