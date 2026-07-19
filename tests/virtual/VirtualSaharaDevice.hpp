#pragma once

#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"
#include "MockTransport.hpp"
#include "MockLogger.hpp"

#include <vector>
#include <string>
#include <chrono>

namespace mbootcore {

// ─── Protocol Trace ────────────────────────────────────────────────────────────

struct TraceRecord {
    enum Type : uint8_t { Tx, Rx, StateChange, Error };
    Type type;
    uint32_t command{0};
    std::string description;
    std::chrono::steady_clock::time_point timestamp;
};

class ProtocolTrace {
public:
    void recordTx(const IPacket& pkt);
    void recordRx(const IPacket& pkt);
    const std::vector<TraceRecord>& records() const noexcept { return m_records; }
    size_t count() const noexcept { return m_records.size(); }
    bool isEmpty() const noexcept { return m_records.empty(); }
    std::string dump() const;

private:
    std::vector<TraceRecord> m_records;
    static std::string_view typeName(TraceRecord::Type t);
};

// ─── VirtualSaharaDevice ───────────────────────────────────────────────────────

class VirtualSaharaDevice {
public:
    struct Behavior {
        uint32_t version{2};
        uint32_t versionSupported{2};
        uint32_t mode{0};
        uint32_t chipIdLo{0x00100000};
        uint32_t chipIdHi{0x00000001};
        uint32_t serialNumber{0x12345678};
        uint32_t msmId{0x00070001};
        uint32_t oemId{0x00000001};
        uint32_t modelId{0x00000005};
        uint32_t imageSize{65536};
        uint32_t chunkSize{4096};
        bool replyToExec{true};
        bool sendNakOnEndImage{false};
        uint32_t nakStatus{0};
        bool timeoutOnRead{false};
        bool disconnectOnRead{false};
    };

    VirtualSaharaDevice(MockTransport& transport, const Behavior& behavior);

    void queueHello();
    void queueChipIdV3();
    void queueUpload();
    void queueEndImage(uint32_t imageId = 0, uint32_t status = 0);
    void queueDoneResponse(uint32_t status = 0);
    void queueResetResponse();
    void queueNakEndImage(uint32_t status, uint32_t imageId = 0);
    void queueUnexpected(uint32_t cmd);

    void push(const IPacket& pkt);

    Behavior& behavior() noexcept { return m_behavior; }
    const Behavior& behavior() const noexcept { return m_behavior; }
    ProtocolTrace& trace() noexcept { return m_trace; }
    const ProtocolTrace& trace() const noexcept { return m_trace; }

private:
    MockTransport& m_transport;
    Behavior m_behavior;
    SaharaPacketSerializer m_serializer;
    ProtocolTrace m_trace;

    ByteBuffer serialize(const IPacket& pkt);
};

// ─── Scenario Runner ───────────────────────────────────────────────────────────

struct ScenarioResult {
    bool handshakeOk{false};
    bool uploadOk{false};
    bool resetOk{false};
    ErrorCode error{ErrorCode::Success};
    size_t bytesUploaded{0};
    ProtocolTrace trace;
};

class SaharaScenario {
public:
    static ScenarioResult handshakeV2Success(MockTransport& transport, MockLogger& logger);
    static ScenarioResult handshakeV3Success(MockTransport& transport, MockLogger& logger);
    static ScenarioResult handshakeVersionMismatch(MockTransport& transport, MockLogger& logger);
    static ScenarioResult handshakeTimeout(MockTransport& transport, MockLogger& logger);
    static ScenarioResult uploadSuccess(MockTransport& transport, MockLogger& logger,
                                         size_t imageSize = 65536, uint32_t chunkSize = 4096);
    static ScenarioResult uploadNak(MockTransport& transport, MockLogger& logger);
    static ScenarioResult uploadTimeout(MockTransport& transport, MockLogger& logger);
    static ScenarioResult uploadUnexpectedPacket(MockTransport& transport, MockLogger& logger);
    static ScenarioResult resetSuccess(MockTransport& transport, MockLogger& logger);
    static ScenarioResult deviceDisconnect(MockTransport& transport, MockLogger& logger);
};

// ─── Inline Implementations ────────────────────────────────────────────────────

inline void ProtocolTrace::recordTx(const IPacket& pkt) {
    m_records.push_back({TraceRecord::Tx, pkt.command(),
                         std::string(pkt.toString()),
                         std::chrono::steady_clock::now()});
}

inline void ProtocolTrace::recordRx(const IPacket& pkt) {
    m_records.push_back({TraceRecord::Rx, pkt.command(),
                         std::string(pkt.toString()),
                         std::chrono::steady_clock::now()});
}

inline std::string_view ProtocolTrace::typeName(TraceRecord::Type t) {
    switch (t) {
        case TraceRecord::Tx: return "TX";
        case TraceRecord::Rx: return "RX";
        case TraceRecord::StateChange: return "ST";
        case TraceRecord::Error: return "ER";
    }
    return "??";
}

inline std::string ProtocolTrace::dump() const {
    std::string out;
    for (const auto& r : m_records) {
        out += std::string(typeName(r.type)) + " ";
        out += "cmd=0x" + std::to_string(r.command);
        out += " " + r.description + "\n";
    }
    return out;
}

inline VirtualSaharaDevice::VirtualSaharaDevice(MockTransport& transport,
                                                 const Behavior& behavior)
    : m_transport(transport)
    , m_behavior(behavior) {}

inline ByteBuffer VirtualSaharaDevice::serialize(const IPacket& pkt) {
    auto result = m_serializer.serialize(pkt);
    if (!result.isOk()) return {};
    return std::move(result.value());
}

inline void VirtualSaharaDevice::push(const IPacket& pkt) {
    auto bytes = serialize(pkt);
    if (!bytes.empty()) {
        m_transport.setReadData(bytes);
        m_trace.recordTx(pkt);
    }
}

inline void VirtualSaharaDevice::queueHello() {
    HelloPacket hello(m_behavior.version, m_behavior.versionSupported,
                      4096, m_behavior.mode);
    push(hello);
}

inline void VirtualSaharaDevice::queueChipIdV3() {
    ReadChipIdPacket chip(m_behavior.chipIdLo, m_behavior.chipIdHi,
                           m_behavior.serialNumber, m_behavior.msmId,
                           m_behavior.oemId, m_behavior.modelId);
    push(chip);
}

inline void VirtualSaharaDevice::queueUpload() {
    uint32_t offset = 0;
    while (offset < m_behavior.imageSize) {
        uint32_t len = std::min(m_behavior.chunkSize, m_behavior.imageSize - offset);
        ReadDataPacket readPkt(0, offset, len);
        push(readPkt);
        offset += len;
    }
}

inline void VirtualSaharaDevice::queueEndImage(uint32_t imageId, uint32_t status) {
    EndImageTransferPacket endImg(imageId, status);
    push(endImg);
}

inline void VirtualSaharaDevice::queueDoneResponse(uint32_t status) {
    DoneResponsePacket doneResp(status);
    push(doneResp);
}

inline void VirtualSaharaDevice::queueResetResponse() {
    DoneResponsePacket doneResp(0);
    push(doneResp);
    ResetResponsePacket resetResp;
    push(resetResp);
}

inline void VirtualSaharaDevice::queueNakEndImage(uint32_t status, uint32_t imageId) {
    queueEndImage(imageId, status);
}

inline void VirtualSaharaDevice::queueUnexpected(uint32_t cmd) {
    // Build a minimal valid packet with the given command
    ByteBuffer buf;
    buf.push_back(static_cast<uint8_t>(cmd & 0xFF));
    buf.push_back(static_cast<uint8_t>((cmd >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((cmd >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((cmd >> 24) & 0xFF));
    buf.push_back(0x08); buf.push_back(0x00); buf.push_back(0x00); buf.push_back(0x00); // len=8
    m_transport.setReadData(buf);
    // Try parsing for trace
    SaharaPacketParser parser;
    auto parsed = parser.parse(buf);
    if (parsed.isOk()) {
        m_trace.recordTx(*parsed.value());
    }
}

// ─── Scenario Implementations ───────────────────────────────────────────────────

inline ScenarioResult SaharaScenario::handshakeV2Success(MockTransport& transport,
                                                          MockLogger& logger) {
    VirtualSaharaDevice device(transport, VirtualSaharaDevice::Behavior{});
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    ScenarioResult result;
    result.handshakeOk = protocol.handshake().isOk();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::handshakeV3Success(MockTransport& transport,
                                                          MockLogger& logger) {
    VirtualSaharaDevice::Behavior beh;
    beh.version = 3;
    beh.chunkSize = 4096;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();
    device.queueChipIdV3();

    SaharaProtocol protocol(transport, logger);
    ScenarioResult result;
    result.handshakeOk = protocol.handshake().isOk();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::handshakeVersionMismatch(MockTransport& transport,
                                                                MockLogger& logger) {
    VirtualSaharaDevice::Behavior beh;
    beh.version = 1;
    beh.versionSupported = 1;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    ScenarioResult result;
    auto hs = protocol.handshake();
    result.handshakeOk = hs.isOk();
    if (hs.isError()) result.error = hs.error();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::handshakeTimeout(MockTransport& transport,
                                                        MockLogger& logger) {
    // No data queued → transport returns default Ok(0) → parse fails → timeout
    SaharaProtocol protocol(transport, logger);
    ScenarioResult result;
    auto hs = protocol.handshake();
    result.handshakeOk = hs.isOk();
    if (hs.isError()) result.error = hs.error();
    return result;
}

inline ScenarioResult SaharaScenario::uploadSuccess(MockTransport& transport,
                                                     MockLogger& logger,
                                                     size_t imageSize,
                                                     uint32_t chunkSize) {
    VirtualSaharaDevice::Behavior beh;
    beh.imageSize = static_cast<uint32_t>(imageSize);
    beh.chunkSize = chunkSize;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();
    if (beh.version >= 3) device.queueChipIdV3();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        r.error = hs.error();
        return r;
    }

    // Clear transport state after handshake
    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    // Queue upload sequence
    device.queueUpload();
    if (!beh.sendNakOnEndImage) {
        device.queueEndImage();
    }

    ByteBuffer progData(imageSize, 0xBE);
    ScenarioResult result;
    result.handshakeOk = true;
    auto up = protocol.uploadProgrammer(progData);
    result.uploadOk = up.isOk();
    if (up.isOk()) {
        result.bytesUploaded = imageSize;
    } else {
        result.error = up.error();
    }
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::uploadNak(MockTransport& transport,
                                                 MockLogger& logger) {
    VirtualSaharaDevice::Behavior beh;
    beh.imageSize = 4096;
    beh.chunkSize = 4096;
    beh.sendNakOnEndImage = true;
    beh.nakStatus = 0x0D; // READ_DATA_ERROR
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        return r;
    }

    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    device.queueUpload();
    device.queueNakEndImage(0x0D);

    ByteBuffer progData(4096, 0xBE);
    ScenarioResult result;
    result.handshakeOk = true;
    auto up = protocol.uploadProgrammer(progData);
    result.uploadOk = up.isOk();
    if (up.isError()) result.error = up.error();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::uploadTimeout(MockTransport& transport,
                                                     MockLogger& logger) {
    // Queue handshake, then make transport timeout on read
    VirtualSaharaDevice::Behavior beh;
    beh.imageSize = 4096;
    beh.timeoutOnRead = true;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        return r;
    }

    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    // No read data queued → subsequent read will fail

    ByteBuffer progData(4096, 0xBE);
    ScenarioResult result;
    result.handshakeOk = true;
    auto up = protocol.uploadProgrammer(progData);
    result.uploadOk = up.isOk();
    if (up.isError()) result.error = up.error();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::uploadUnexpectedPacket(MockTransport& transport,
                                                              MockLogger& logger) {
    VirtualSaharaDevice::Behavior beh;
    beh.imageSize = 4096;
    beh.chunkSize = 4096;
    VirtualSaharaDevice device(transport, beh);
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        return r;
    }

    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    // Send unexpected packet (e.g. HELLO_REQ again)
    device.queueUnexpected(0x01);

    ByteBuffer progData(4096, 0xBE);
    ScenarioResult result;
    result.handshakeOk = true;
    auto up = protocol.uploadProgrammer(progData);
    result.uploadOk = up.isOk();
    if (up.isError()) result.error = up.error();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::resetSuccess(MockTransport& transport,
                                                    MockLogger& logger) {
    VirtualSaharaDevice device(transport, VirtualSaharaDevice::Behavior{});
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        return r;
    }

    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    device.queueResetResponse();

    ScenarioResult result;
    result.handshakeOk = true;
    auto rs = protocol.reset();
    result.resetOk = rs.isOk();
    if (rs.isError()) result.error = rs.error();
    result.trace = device.trace();
    return result;
}

inline ScenarioResult SaharaScenario::deviceDisconnect(MockTransport& transport,
                                                        MockLogger& logger) {
    VirtualSaharaDevice device(transport, VirtualSaharaDevice::Behavior{});
    device.queueHello();

    SaharaProtocol protocol(transport, logger);
    auto hs = protocol.handshake();
    if (!hs.isOk()) {
        ScenarioResult r;
        r.handshakeOk = false;
        return r;
    }

    transport.clearWrites();
    transport.clearReadQueue();
    transport.resetReadResult();

    // Transport read fails with disconnect
    transport.setReadResult(Result<size_t>::Error(ErrorCode::TransportDisconnected));

    ScenarioResult result;
    result.handshakeOk = true;
    auto rs = protocol.reset();
    result.resetOk = rs.isOk();
    if (rs.isError()) result.error = rs.error();
    result.trace = device.trace();
    return result;
}

} // namespace mbootcore