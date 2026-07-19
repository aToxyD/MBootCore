#include "CliApplication.hpp"
#include "CliParser.hpp"
#include "CliCommands.hpp"

#include <iostream>
#include <cstdlib>

using namespace mboot::cli;

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    CliApplication app;

    if (args.empty()) {
        return app.runInteractive();
    }

    return app.run(args);
}
