#include <mbootcore/MBootCore.hpp>
#include <mbootcore/core/protocols/firehose/FirehoseProtocol.hpp>
#include <mbootcore/transport/TransportFactory.hpp>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: firehose <image.bin>\n";
        return 1;
    }

    // Step 1: Sahara handshake + programmer upload via Session
    mbootcore::Session session;
    session.setLogger(std::make_unique<mbootcore::ConsoleLogger>());

    if (!session.connect().isOk()) { return 1; }
    if (!session.detectDevice().isOk()) { session.disconnect(); return 1; }
    if (!session.uploadLoader("prog_ufs.elf").isOk()) {
        session.disconnect(); return 1;
    }
    if (!session.resetDevice().isOk()) { session.disconnect(); return 1; }
    std::cout << "Programmer uploaded, device in Firehose mode\n";
    session.disconnect();

    // Step 2: Reconnect with Firehose protocol
    auto logger = std::make_unique<mbootcore::ConsoleLogger>();
    auto transport = mbootcore::transport::TransportFactory::createMock();
    auto& transportRef = *transport;
    mbootcore::FirehoseProtocol proto(transportRef, *logger);

    if (!transport->open().isOk()) {
        std::cerr << "USB open failed\n";
        return 1;
    }

    if (!proto.handshake().isOk()) {
        std::cerr << "Firehose handshake failed\n";
        transport->close();
        return 1;
    }

    // Configure storage
    mbootcore::ConfigureCommand cfgCmd;
    cfgCmd.memoryName = "ufs";
    cfgCmd.maxPayloadSizeToTarget = 1048576;

    auto cfgResult = proto.sendCommand(cfgCmd);
    if (!cfgResult.isOk()) {
        std::cerr << "Configure failed\n";
        transport->close();
        return 1;
    }
    std::cout << "Configured UFS (" << cfgResult.value().commandName() << ")\n";

    // Read image file
    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    if (!file) { std::cerr << "Cannot open " << argv[1] << "\n"; return 1; }
    auto size = file.tellg();
    file.seekg(0);
    mbootcore::ByteBuffer data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()),
              static_cast<std::streamsize>(data.size()));

    // Program
    mbootcore::ProgramCommand pgmCmd;
    pgmCmd.sectorSize = 4096;
    pgmCmd.numSectorSize = static_cast<uint32_t>(data.size() / 4096 + 1);

    auto pr = proto.program(pgmCmd, data);
    if (!pr.isOk()) {
        std::cerr << "Program failed\n";
        transport->close();
        return 1;
    }
    std::cout << "Programmed " << data.size() << " bytes\n";

    transport->close();
    return 0;
}
