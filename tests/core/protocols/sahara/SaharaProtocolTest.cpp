#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"
#include "MockTransport.hpp"
#include "MockLogger.hpp"

namespace {

mbootcore::ByteBuffer serialize(const mbootcore::IPacket& pkt) {
    mbootcore::SaharaPacketSerializer ser;
    auto result = ser.serialize(pkt);
    if (!result.isOk()) return {};
    return result.value();
}

void setupV2Hello(mbootcore::MockTransport& transport) {
    mbootcore::HelloPacket hello(2, 2, 4096, 0);
    transport.setReadData(serialize(hello));
}

void setupV3Hello(mbootcore::MockTransport& transport) {
    mbootcore::HelloPacket hello(3, 2, 4096, 0);
    transport.setReadData(serialize(hello));
}

void setupV3ChipId(mbootcore::MockTransport& transport) {
    mbootcore::ReadChipIdPacket chip(0x12345678, 0x00000001, 0xAABBCCDD, 0x0000C0DE, 0x00000001, 0x00000005);
    chip.setV3(true);
    transport.setReadData(serialize(chip));
}

void setupReadData(mbootcore::MockTransport& transport,
                   uint32_t imageId, uint32_t offset, uint32_t length) {
    mbootcore::ReadDataPacket readPkt(imageId, offset, length);
    transport.setReadData(serialize(readPkt));
}

void setupEndImage(mbootcore::MockTransport& transport,
                   uint32_t imageId = 0, uint32_t status = 0) {
    mbootcore::EndImageTransferPacket endImg(imageId, status);
    transport.setReadData(serialize(endImg));
}

// setupDoneResponse intentionally unused; kept for reference

void setupResetResponse(mbootcore::MockTransport& transport) {
    mbootcore::DoneResponsePacket doneResp(0);
    transport.setReadData(serialize(doneResp));
    mbootcore::ResetResponsePacket resetResp;
    transport.setReadData(serialize(resetResp));
}

mbootcore::ByteBuffer buildV2Response(uint32_t clientCmd, uint32_t respLength,
                                       const mbootcore::ByteBuffer& payload) {
    auto le32 = [](uint32_t v) -> mbootcore::ByteBuffer {
        return {static_cast<uint8_t>(v & 0xFF),
                static_cast<uint8_t>((v >> 8) & 0xFF),
                static_cast<uint8_t>((v >> 16) & 0xFF),
                static_cast<uint8_t>((v >> 24) & 0xFF)};
    };
    mbootcore::ByteBuffer buf;
    buf.reserve(16 + payload.size());
    auto cmd = le32(0x0E);
    buf.insert(buf.end(), cmd.begin(), cmd.end());
    auto len = le32(16);
    buf.insert(buf.end(), len.begin(), len.end());
    auto cc = le32(clientCmd);
    buf.insert(buf.end(), cc.begin(), cc.end());
    auto rl = le32(respLength);
    buf.insert(buf.end(), rl.begin(), rl.end());
    buf.insert(buf.end(), payload.begin(), payload.end());
    return buf;
}

void setupV2HelloWithDiscovery(mbootcore::MockTransport& transport,
                                const std::vector<mbootcore::ByteBuffer>& responses) {
    mbootcore::HelloPacket hello(2, 2, 4096, 0);
    transport.setReadData(serialize(hello));
    for (const auto& rsp : responses) {
        transport.setReadData(rsp);
    }
}

} // anonymous namespace

TEST_CASE("SaharaProtocolTest", "[sahara]") {
    SECTION("testV2HandshakeSuccess") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));

        auto writes = transport.writes();
        REQUIRE(writes.size() >= 1);
        REQUIRE(writes[0].data[0] == uint8_t(0x02));
    }

    SECTION("testV3HandshakeSuccess") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV3Hello(transport);
        setupV3ChipId(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(3));

        const auto& info = protocol.deviceInfo();
        REQUIRE(info.serialNumber == uint32_t(0xAABBCCDD));
        REQUIRE(info.id.msmId == uint32_t(0x0000C0DE));
    }

    SECTION("testV3HandshakeFallbackToV2") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV3Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
    }

    SECTION("testVersionMismatch") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        mbootcore::HelloPacket hello(1, 1, 4096, 0);
        transport.setReadData(serialize(hello));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isError());
        REQUIRE(result.error() == mbootcore::ErrorCode::ProtocolMismatch);
    }

    SECTION("testUploadSuccess") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        mbootcore::ByteBuffer programmerData(128, 0xBE);

        setupReadData(transport, 0, 0, 64);
        setupReadData(transport, 0, 64, 64);
        setupEndImage(transport);

        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isOk());

        auto writes = transport.writes();
        REQUIRE(writes.size() >= 2);
        REQUIRE(writes[0].data.size() == size_t(64));
        REQUIRE(writes[1].data.size() == size_t(64));
    }

    SECTION("testUploadWithNakError") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        setupEndImage(transport, 0, 0x0D);

        mbootcore::ByteBuffer programmerData(64, 0xBE);
        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isError());
    }

    SECTION("testUploadOutOfBounds") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        setupReadData(transport, 0, 0, 128);

        mbootcore::ByteBuffer programmerData(64, 0xBE);
        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isError());
        REQUIRE(uploadResult.error() == mbootcore::ErrorCode::InvalidPacket);
    }

    SECTION("testUploadOverflowOffsetPlusLength") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        setupReadData(transport, 0, UINT32_MAX, 1);

        mbootcore::ByteBuffer programmerData(64, 0xBE);
        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isError());
        REQUIRE(uploadResult.error() == mbootcore::ErrorCode::InvalidPacket);
        REQUIRE(transport.writes().empty());
    }

    SECTION("testUploadZeroLengthAtEnd") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        mbootcore::ByteBuffer programmerData(64, 0xBE);
        setupReadData(transport, 0, static_cast<uint32_t>(programmerData.size()), 0);
        setupEndImage(transport);

        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isOk());
    }

    SECTION("testUploadProgressCorrect") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        std::vector<std::pair<size_t, size_t>> progressValues;
        protocol.onProgress([&](size_t current, size_t total) {
            progressValues.emplace_back(current, total);
        });

        mbootcore::ByteBuffer programmerData(128, 0xBE);
        setupReadData(transport, 0, 0, 64);
        setupReadData(transport, 0, 64, 64);
        setupEndImage(transport);

        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isOk());
        REQUIRE(progressValues.size() == size_t(2));
        REQUIRE(progressValues[0].first == size_t(64));
        REQUIRE(progressValues[0].second == size_t(128));
        REQUIRE(progressValues[1].first == size_t(128));
        REQUIRE(progressValues[1].second == size_t(128));
    }

    SECTION("testReset") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.clearReadQueue();
        transport.resetReadResult();

        setupResetResponse(transport);

        auto resetResult = protocol.reset();
        REQUIRE(resetResult.isOk());
        REQUIRE(protocol.phase() == mbootcore::ProtocolPhase::Finished);

        auto writes = transport.writes();
        REQUIRE(writes.size() >= 2);
        REQUIRE(writes[0].data[0] == uint8_t(0x05));
        REQUIRE(writes[1].data[0] == uint8_t(0x07));
    }

    SECTION("testTimeoutDuringHandshake") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        transport.setReadResult(mbootcore::Result<size_t>(
            mbootcore::ErrorCode::TransportTimeout));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isError());
        REQUIRE(result.error() == mbootcore::ErrorCode::TransportTimeout);
    }

    SECTION("testCancelDuringUpload") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();

        protocol.cancel();

        mbootcore::ByteBuffer programmerData(64, 0xBE);
        auto uploadResult = protocol.uploadProgrammer(programmerData);
        REQUIRE(uploadResult.isError());
    }

    SECTION("testResetState") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;

        mbootcore::SaharaProtocol protocol(transport, logger);
        protocol.resetState();

        auto& sm = protocol.stateMachine();
        REQUIRE(sm.currentState().id() == uint32_t(1));
        REQUIRE(protocol.errors().empty());
    }

    SECTION("testNegotiatedVersionAccessor") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        REQUIRE(protocol.negotiatedVersion() == uint32_t(0));

        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));
    }

    SECTION("testProtocolName") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        mbootcore::SaharaProtocol protocol(transport, logger);
        REQUIRE(protocol.name() == std::string_view("Sahara"));
    }

    SECTION("testPhaseProgression") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        REQUIRE(protocol.phase() == mbootcore::ProtocolPhase::Idle);

        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.phase() == mbootcore::ProtocolPhase::Idle);
    }

    SECTION("testErrorCollection") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;

        mbootcore::SaharaProtocol protocol(transport, logger);
        REQUIRE(protocol.errors().empty());

        mbootcore::HelloPacket hello(1, 1, 4096, 0);
        transport.setReadData(serialize(hello));

        auto result = protocol.handshake();
        REQUIRE(result.isError());
        REQUIRE(!protocol.errors().empty());
    }

    // --- V2 Discovery error path coverage (H-04) ---

    // E1 (send failure) is intentionally not covered here.
    // The current MockTransport cannot fail a specific write while allowing
    // previous writes to succeed. Exercising this path would require a
    // transport test double with per-operation failure injection.

    SECTION("v2DiscoverySuccess_allQueries") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;

        mbootcore::ByteBuffer serialPayload = {0xDD, 0xCC, 0xBB, 0xAA};
        mbootcore::ByteBuffer msmPayload = {0x78, 0x56, 0x34, 0x12,
                                             0xAB, 0xCD, 0xEF, 0x01,
                                             0x23, 0x45, 0x67, 0x89};
        mbootcore::ByteBuffer pkhashPayload(32);
        for (size_t i = 0; i < 32; ++i) pkhashPayload[i] = static_cast<uint8_t>(i);

        setupV2HelloWithDiscovery(transport, {
            buildV2Response(1, 4, serialPayload),
            buildV2Response(2, 12, msmPayload),
            buildV2Response(3, 32, pkhashPayload)
        });

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));

        const auto& info = protocol.deviceInfo();
        REQUIRE(info.version.major == uint32_t(2));
        REQUIRE(info.serialNumber == uint32_t(0xAABBCCDD));
        REQUIRE(info.id.msmId == uint32_t(0x12345678));
        REQUIRE(info.id.oemId == uint32_t(0x01EFCDAB));
        REQUIRE(info.id.modelId == uint32_t(0x89674523));
        for (size_t i = 0; i < 32; ++i) {
            REQUIRE(info.id.pkhash[i] == static_cast<uint8_t>(i));
        }
        REQUIRE(info.chipIdLo == uint32_t(0));
        REQUIRE(info.chipIdHi == uint32_t(0));
    }

    SECTION("v2DiscoveryReadFails") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);
        transport.setReadResult(mbootcore::Result<size_t>(
            mbootcore::ErrorCode::TransportError));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));
        REQUIRE(protocol.deviceInfo().serialNumber == uint32_t(0));
    }

    SECTION("v2DiscoveryParseFails") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);
        mbootcore::ByteBuffer garbage = {0xFF, 0xFE, 0xFD, 0xFC,
                                          0xFB, 0xFA, 0xF9, 0xF8,
                                          0xF7, 0xF6, 0xF5, 0xF4,
                                          0xF3, 0xF2, 0xF1, 0xF0};
        transport.setReadData(garbage);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));
        REQUIRE(protocol.deviceInfo().serialNumber == uint32_t(0));
    }

    SECTION("v2DiscoveryWrongCommand") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);
        mbootcore::DonePacket donePkt;
        transport.setReadData(serialize(donePkt));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));
        REQUIRE(protocol.deviceInfo().serialNumber == uint32_t(0));
    }

    SECTION("v2DiscoveryMismatchedClientCmd") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);
        transport.setReadData(buildV2Response(99, 0, {}));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());
        REQUIRE(protocol.negotiatedVersion() == uint32_t(2));
        REQUIRE(protocol.deviceInfo().serialNumber == uint32_t(0));
    }

    SECTION("v2DiscoveryPartialFailure") {
        // Regression contract:
        // V2 discovery is best-effort. Failure of one query aborts subsequent
        // queries, but successfully retrieved information from prior queries
        // must still be preserved.
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;

        mbootcore::ByteBuffer serialPayload = {0xDD, 0xCC, 0xBB, 0xAA};

        setupV2Hello(transport);
        transport.setReadData(buildV2Response(1, 4, serialPayload));
        transport.setReadResult(mbootcore::Result<size_t>(
            mbootcore::ErrorCode::TransportError));

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto result = protocol.handshake();
        REQUIRE(result.isOk());

        const auto& info = protocol.deviceInfo();
        REQUIRE(info.serialNumber == uint32_t(0xAABBCCDD));
        REQUIRE(info.id.msmId == uint32_t(0));
        REQUIRE(info.id.oemId == uint32_t(0));
        REQUIRE(info.id.modelId == uint32_t(0));
        bool anyPkhashSet = false;
        for (auto b : info.id.pkhash) { if (b != 0) anyPkhashSet = true; }
        REQUIRE_FALSE(anyPkhashSet);
    }

    // --- reset() error path coverage (H-05) ---

    // E3 (send failure of ResetPacket) is intentionally not covered here.
    // The current MockTransport cannot fail a specific write while allowing
    // previous writes to succeed. Exercising this path would require a
    // transport test double with per-operation failure injection.

    SECTION("resetSendDoneFails") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.setWriteResult(mbootcore::Result<size_t>(
            mbootcore::ErrorCode::TransportWriteFailed));

        auto resetResult = protocol.reset();
        REQUIRE(resetResult.isError());
        auto writes = transport.writes();
        REQUIRE(writes.size() == 1);
        REQUIRE(writes[0].data[0] == uint8_t(0x05));
    }

    SECTION("resetReceiveDoneRespFails") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        transport.setReadResult(mbootcore::Result<size_t>(
            mbootcore::ErrorCode::TransportError));

        auto resetResult = protocol.reset();
        REQUIRE(resetResult.isError());
        auto writes = transport.writes();
        REQUIRE(writes.size() == 1);
        REQUIRE(writes[0].data[0] == uint8_t(0x05));
    }

    SECTION("resetReceiveDoneRespFails_wrongCommand") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        mbootcore::HelloPacket wrongPkt;
        transport.setReadData(serialize(wrongPkt));

        auto resetResult = protocol.reset();
        REQUIRE(resetResult.isError());
        auto writes = transport.writes();
        REQUIRE(writes.size() == 1);
        REQUIRE(writes[0].data[0] == uint8_t(0x05));
    }

    SECTION("resetReceiveResetRespFails") {
        mbootcore::MockTransport transport;
        mbootcore::MockLogger logger;
        setupV2Hello(transport);

        mbootcore::SaharaProtocol protocol(transport, logger);
        auto handshakeResult = protocol.handshake();
        REQUIRE(handshakeResult.isOk());

        transport.clearWrites();
        mbootcore::DoneResponsePacket doneResp(0);
        transport.setReadData(serialize(doneResp));
        mbootcore::ByteBuffer garbage = {0xFF, 0xFE, 0xFD, 0xFC,
                                          0xFB, 0xFA, 0xF9, 0xF8};
        transport.setReadData(garbage);

        auto resetResult = protocol.reset();
        REQUIRE(resetResult.isError());
        auto writes = transport.writes();
        REQUIRE(writes.size() == 2);
        REQUIRE(writes[0].data[0] == uint8_t(0x05));
        REQUIRE(writes[1].data[0] == uint8_t(0x07));
    }
}
