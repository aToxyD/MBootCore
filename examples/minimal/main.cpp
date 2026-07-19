#include <mbootcore/MBootCore.hpp>
#include <iostream>

int main() {
    mbootcore::Session session;

    auto result = session.connect();
    if (!result.isOk()) {
        std::cerr << "Failed to connect\n";
        return 1;
    }

    result = session.detectDevice();
    if (!result.isOk()) {
        std::cerr << "No device detected\n";
        session.disconnect();
        return 1;
    }

    std::cout << "Device detected and connected\n";
    session.disconnect();
    return 0;
}
