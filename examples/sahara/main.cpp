#include <mbootcore/MBootCore.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sahara <prog_elf>\n";
        return 1;
    }

    mbootcore::Session session;
    session.setLogger(std::make_unique<mbootcore::ConsoleLogger>());

    auto result = session.connect();
    if (!result.isOk()) {
        std::cerr << "Connect failed: " << static_cast<int>(result.error()) << "\n";
        return 1;
    }

    result = session.detectDevice();
    if (!result.isOk()) {
        std::cerr << "Detect failed\n";
        session.disconnect();
        return 1;
    }

    result = session.uploadLoader(argv[1]);
    if (!result.isOk()) {
        std::cerr << "Upload failed\n";
        session.disconnect();
        return 1;
    }

    std::cout << "Uploaded " << argv[1] << "\n";

    result = session.resetDevice();
    if (!result.isOk()) {
        std::cerr << "Reset failed\n";
        session.disconnect();
        return 1;
    }

    std::cout << "Device reset to Firehose mode\n";
    session.disconnect();
    return 0;
}
