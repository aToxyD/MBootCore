#pragma once

#ifdef MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS

#include "MTKProtocolTypes.hpp"

#include <mbootcore/protocol/IProtocol.hpp>
#include <mbootcore/protocol/IProtocolSession.hpp>
#include <mbootcore/protocol/IProtocolDiscovery.hpp>
#include <mbootcore/protocol/IProtocolFactory.hpp>
#include <mbootcore/protocol/IMessageEncoder.hpp>
#include <mbootcore/protocol/IMessageDecoder.hpp>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ITransport.hpp>

#include <chrono>
#include <cstring>

// ======================================================================
// MediaTek Scaffold Protocol — Reference Implementation
//
// This is a REFERENCE SCAFFOLD, not a production implementation.
// It demonstrates the Protocol Platform extensibility pattern.
// Not tested against real hardware. Not suitable for production use.
// ======================================================================

namespace mbootcore::protocol::mediatek {

class MTKScaffoldEncoder : public IMessageEncoder {
public:
    ProtocolResult<Packet> encode(const Request& req) override
    {
        auto val = req.command().id.value();
        auto cmd = static_cast<MTKCommand>(val & 0xFFFF);
        auto raw = makeRequestPacket(cmd,
                                     reinterpret_cast<const uint8_t*>(req.payload().data()),
                                     static_cast<uint16_t>(req.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }

    ProtocolResult<Packet> encode(const Response& res) override
    {
        auto reason = static_cast<uint16_t>(res.status().reason());
        auto status = (res.status().isSuccess() ? MTKStatus::Success
                                                : static_cast<MTKStatus>(reason));
        auto raw = makeResponsePacket(status,
                                      reinterpret_cast<const uint8_t*>(res.payload().data()),
                                      static_cast<uint16_t>(res.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }

    ProtocolResult<Packet> encode(const Event& ev) override
    {
        auto raw = makeResponsePacket(MTKStatus::Success,
                                      reinterpret_cast<const uint8_t*>(ev.payload().data()),
                                      static_cast<uint16_t>(ev.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }
};

class MTKScaffoldDecoder : public IMessageDecoder {
public:
    ProtocolResult<Request> decodeRequest(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 6)
            return ProtocolResult<Request>::Error(ErrorCode::ProtocolError);
        if (d[0] != 0xAA || d[1] != 0x55)
            return ProtocolResult<Request>::Error(ErrorCode::ProtocolError);

        uint16_t cmdVal = static_cast<uint16_t>(d[2]) | (static_cast<uint16_t>(d[3]) << 8);
        uint16_t plen = static_cast<uint16_t>(d[4]) | (static_cast<uint16_t>(d[5]) << 8);

        Command cmd{CommandId{cmdVal}, "mediatek", ""};
        Payload payload;
        if (plen > 0 && 6 + plen <= d.size())
            payload = Payload::copy(d.data() + 6, plen);

        return ProtocolResult<Request>::Ok(Request{
            MessageMetadata{MessageId{1}},
            cmd,
            std::move(payload)
        });
    }

    ProtocolResult<Response> decodeResponse(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 6)
            return ProtocolResult<Response>::Error(ErrorCode::ProtocolError);
        if (d[0] != 0x55 || d[1] != 0xAA)
            return ProtocolResult<Response>::Error(ErrorCode::ProtocolError);

        uint16_t st = static_cast<uint16_t>(d[2]) | (static_cast<uint16_t>(d[3]) << 8);
        uint16_t plen = static_cast<uint16_t>(d[4]) | (static_cast<uint16_t>(d[5]) << 8);

        auto status = (st == 0) ? StatusCode::success() : StatusCode::failure(st);
        Payload payload;
        if (plen > 0 && 6 + plen <= d.size())
            payload = Payload::copy(d.data() + 6, plen);

        return ProtocolResult<Response>::Ok(Response{
            MessageMetadata{MessageId{1}},
            status,
            std::move(payload)
        });
    }

    ProtocolResult<Event> decodeEvent(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 6)
            return ProtocolResult<Event>::Error(ErrorCode::ProtocolError);

        uint16_t plen = static_cast<uint16_t>(d[4]) | (static_cast<uint16_t>(d[5]) << 8);
        Payload payload;
        if (plen > 0 && 6 + plen <= d.size())
            payload = Payload::copy(d.data() + 6, plen);

        return ProtocolResult<Event>::Ok(Event{
            MessageMetadata{MessageId{1}},
            std::move(payload)
        });
    }
};

class MTKScaffoldSession : public IProtocolSession {
public:
    explicit MTKScaffoldSession(const ProtocolId& protocolId, ProtocolVersion version)
        : m_protocolId(protocolId)
        , m_version(version)
        , m_sessionId(SessionId{reinterpret_cast<uintptr_t>(this)})
    {}

    SessionId id() const override { return m_sessionId; }

    CapabilitySet capabilities() const override
    {
        CapabilitySet caps;
        caps.add(CapabilityId{static_cast<uint32_t>(MTKCommand::Probe)});
        caps.add(CapabilityId{static_cast<uint32_t>(MTKCommand::Handshake)});
        caps.add(CapabilityId{static_cast<uint32_t>(MTKCommand::GetVersion)});
        caps.add(CapabilityId{static_cast<uint32_t>(MTKCommand::GetHwCode)});
        return caps;
    }

    bool isOpen() const override { return m_transport != nullptr; }

    ProtocolResult<void> open(ITransport& transport) override
    {
        m_transport = &transport;
        return handshake();
    }

    ProtocolResult<void> close() override
    {
        m_transport = nullptr;
        return ProtocolResult<void>::Ok();
    }

private:
    static constexpr auto DefaultTimeout = std::chrono::milliseconds{5000};

    ProtocolResult<void> handshake()
    {
        uint8_t verPayload[4];
        auto major = static_cast<uint16_t>(m_version.major);
        auto minor = static_cast<uint16_t>(m_version.minor);
        verPayload[0] = static_cast<uint8_t>(major & 0xFF);
        verPayload[1] = static_cast<uint8_t>((major >> 8) & 0xFF);
        verPayload[2] = static_cast<uint8_t>(minor & 0xFF);
        verPayload[3] = static_cast<uint8_t>((minor >> 8) & 0xFF);

        auto reqPkt = makeRequestPacket(MTKCommand::Handshake, verPayload, 4);
        auto writeResult = m_transport->write(reqPkt, DefaultTimeout);
        if (writeResult.isError())
            return ProtocolResult<void>::Error(writeResult.error());

        ByteBuffer respBuf(64);
        auto readResult = m_transport->read(respBuf, 6, 64, DefaultTimeout);
        if (readResult.isError())
            return ProtocolResult<void>::Error(readResult.error());

        size_t bytesRead = readResult.value();
        if (bytesRead < 6 || respBuf[0] != 0x55 || respBuf[1] != 0xAA)
            return ProtocolResult<void>::Error(ErrorCode::ProtocolError);

        uint16_t st = static_cast<uint16_t>(respBuf[2]) | (static_cast<uint16_t>(respBuf[3]) << 8);
        if (st != 0)
            return ProtocolResult<void>::Error(ErrorCode::ProtocolError);

        return ProtocolResult<void>::Ok();
    }

    const ProtocolId     m_protocolId;
    const ProtocolVersion m_version;
    const SessionId      m_sessionId;
    ITransport*          m_transport = nullptr;
};

class MTKScaffoldDiscovery : public IProtocolDiscovery {
public:
    ProtocolResult<bool> probe(ITransport& transport) override
    {
        auto reqPkt = makeRequestPacket(MTKCommand::Probe, nullptr, 0);
        auto writeResult = transport.write(reqPkt, ProbeTimeout);
        if (writeResult.isError())
            return ProtocolResult<bool>::Ok(false);

        ByteBuffer respBuf(64);
        auto readResult = transport.read(respBuf, 6, 64, ProbeTimeout);
        if (readResult.isError())
            return ProtocolResult<bool>::Ok(false);

        size_t bytesRead = readResult.value();
        bool detected = (bytesRead >= 6 && respBuf[0] == 0x55 && respBuf[1] == 0xAA);
        return ProtocolResult<bool>::Ok(detected);
    }

private:
    static constexpr auto ProbeTimeout = std::chrono::milliseconds{2000};
};

class MTKScaffoldFactory : public IProtocolFactory {
public:
    std::unique_ptr<IProtocolDiscovery> createDiscovery() override
    {
        return std::make_unique<MTKScaffoldDiscovery>();
    }

    std::unique_ptr<IProtocolSession> createSession() override
    {
        return std::make_unique<MTKScaffoldSession>(m_protocolId, m_version);
    }

private:
    ProtocolId      m_protocolId{"mediatek"};
    ProtocolVersion m_version{MediatekMajorVersion, MediatekMinorVersion};
};

class MTKScaffoldProtocol : public IProtocol {
public:
    ProtocolId      id()      const override { return ProtocolId{"mediatek"}; }
    ProtocolVersion version() const override { return ProtocolVersion{MediatekMajorVersion, MediatekMinorVersion}; }

    std::unique_ptr<IProtocolFactory> factory() const override
    {
        return std::make_unique<MTKScaffoldFactory>();
    }
};

} // namespace mbootcore::protocol::mediatek

#else
#error "MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS is not defined. \
Vendor scaffold protocols are not available in this build. \
Enable the CMake option to use them."
#endif
