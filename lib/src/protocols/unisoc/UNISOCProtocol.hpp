#pragma once

#ifdef MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS

#include "UNISOCProtocolTypes.hpp"

#include <mbootcore/protocol/IProtocol.hpp>
#include <mbootcore/protocol/IProtocolSession.hpp>
#include <mbootcore/protocol/IProtocolDiscovery.hpp>
#include <mbootcore/protocol/IProtocolFactory.hpp>
#include <mbootcore/protocol/IMessageEncoder.hpp>
#include <mbootcore/protocol/IMessageDecoder.hpp>
#include <mbootcore/protocol/ProtocolContext.hpp>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ITransport.hpp>

#include <chrono>
#include <cstring>

// ======================================================================
// UNISOC Scaffold Protocol — Reference Implementation
//
// This is a REFERENCE SCAFFOLD, not a production implementation.
// It demonstrates the Protocol Platform extensibility pattern.
// Not tested against real hardware. Not suitable for production use.
// ======================================================================

namespace mbootcore::protocol::unisoc {

class UNISOCScaffoldEncoder : public IMessageEncoder {
public:
    ProtocolResult<Packet> encode(const Request& req) override
    {
        auto val = req.command().id.value();
        auto cmd = static_cast<UnisocCommand>(val & 0xFF);
        auto raw = makeRequestPacket(cmd,
                                     reinterpret_cast<const uint8_t*>(req.payload().data()),
                                     static_cast<uint16_t>(req.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }

    ProtocolResult<Packet> encode(const Response& res) override
    {
        auto status = static_cast<UnisocStatus>(res.status().reason() & 0xFF);
        auto raw = makeResponsePacket(status,
                                      reinterpret_cast<const uint8_t*>(res.payload().data()),
                                      static_cast<uint16_t>(res.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }

    ProtocolResult<Packet> encode(const Event& ev) override
    {
        auto raw = makeResponsePacket(UnisocStatus::Success,
                                      reinterpret_cast<const uint8_t*>(ev.payload().data()),
                                      static_cast<uint16_t>(ev.payload().size()));
        return ProtocolResult<Packet>::Ok(Packet{ByteBuffer{raw.begin(), raw.end()}});
    }
};

class UNISOCScaffoldDecoder : public IMessageDecoder {
public:
    ProtocolResult<Request> decodeRequest(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 3)
            return ProtocolResult<Request>::Error(ErrorCode::ProtocolError);

        auto cmdId = CommandId{d[0]};
        Command cmd{cmdId, "unisoc", ""};
        uint16_t plen = static_cast<uint16_t>(d[1]) | (static_cast<uint16_t>(d[2]) << 8);
        Payload payload;
        if (plen > 0 && 3 + plen <= d.size())
            payload = Payload::copy(d.data() + 3, plen);

        return ProtocolResult<Request>::Ok(Request{
            MessageMetadata{MessageId{1}},
            cmd,
            std::move(payload)
        });
    }

    ProtocolResult<Response> decodeResponse(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 3)
            return ProtocolResult<Response>::Error(ErrorCode::ProtocolError);

        auto st = d[0];
        auto status = (st == 0)
            ? StatusCode::success()
            : StatusCode::failure(st);
        uint16_t plen = static_cast<uint16_t>(d[1]) | (static_cast<uint16_t>(d[2]) << 8);
        Payload payload;
        if (plen > 0 && 3 + plen <= d.size())
            payload = Payload::copy(d.data() + 3, plen);

        return ProtocolResult<Response>::Ok(Response{
            MessageMetadata{MessageId{1}},
            status,
            std::move(payload)
        });
    }

    ProtocolResult<Event> decodeEvent(const Packet& pkt) override
    {
        auto& d = pkt.data();
        if (d.size() < 3)
            return ProtocolResult<Event>::Error(ErrorCode::ProtocolError);

        uint16_t plen = static_cast<uint16_t>(d[1]) | (static_cast<uint16_t>(d[2]) << 8);
        Payload payload;
        if (plen > 0 && 3 + plen <= d.size())
            payload = Payload::copy(d.data() + 3, plen);

        return ProtocolResult<Event>::Ok(Event{
            MessageMetadata{MessageId{1}},
            std::move(payload)
        });
    }
};

class UNISOCScaffoldSession : public IProtocolSession {
public:
    explicit UNISOCScaffoldSession(const ProtocolId& protocolId, ProtocolVersion version)
        : m_protocolId(protocolId)
        , m_version(version)
        , m_sessionId(SessionId{reinterpret_cast<uintptr_t>(this)})
    {}

    SessionId id() const override { return m_sessionId; }

    CapabilitySet capabilities() const override
    {
        CapabilitySet caps;
        caps.add(CapabilityId{static_cast<uint32_t>(UnisocCommand::Probe)});
        caps.add(CapabilityId{static_cast<uint32_t>(UnisocCommand::Handshake)});
        caps.add(CapabilityId{static_cast<uint32_t>(UnisocCommand::GetVersion)});
        caps.add(CapabilityId{static_cast<uint32_t>(UnisocCommand::GetChipInfo)});
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
        uint8_t verPayload[2] = {
            static_cast<uint8_t>(m_version.major),
            static_cast<uint8_t>(m_version.minor)
        };
        auto reqPkt = makeRequestPacket(UnisocCommand::Handshake, verPayload, 2);
        auto writeResult = m_transport->write(reqPkt, DefaultTimeout);
        if (writeResult.isError())
            return ProtocolResult<void>::Error(writeResult.error());

        ByteBuffer respBuf(64);
        auto readResult = m_transport->read(respBuf, 3, 64, DefaultTimeout);
        if (readResult.isError())
            return ProtocolResult<void>::Error(readResult.error());

        size_t bytesRead = readResult.value();
        if (bytesRead < 3 || respBuf[0] != 0)
            return ProtocolResult<void>::Error(ErrorCode::ProtocolError);

        return ProtocolResult<void>::Ok();
    }

    const ProtocolId     m_protocolId;
    const ProtocolVersion m_version;
    const SessionId      m_sessionId;
    ITransport*          m_transport = nullptr;
};

class UNISOCScaffoldDiscovery : public IProtocolDiscovery {
public:
    ProtocolResult<bool> probe(ITransport& transport) override
    {
        auto reqPkt = makeRequestPacket(UnisocCommand::Probe, nullptr, 0);
        auto writeResult = transport.write(reqPkt, ProbeTimeout);
        if (writeResult.isError())
            return ProtocolResult<bool>::Ok(false);

        ByteBuffer respBuf(64);
        auto readResult = transport.read(respBuf, 1, 64, ProbeTimeout);
        if (readResult.isError())
            return ProtocolResult<bool>::Ok(false);

        size_t bytesRead = readResult.value();
        bool detected = (bytesRead >= 1 && respBuf[0] == 0);
        return ProtocolResult<bool>::Ok(detected);
    }

private:
    static constexpr auto ProbeTimeout = std::chrono::milliseconds{2000};
};

class UNISOCScaffoldFactory : public IProtocolFactory {
public:
    std::unique_ptr<IProtocolDiscovery> createDiscovery() override
    {
        return std::make_unique<UNISOCScaffoldDiscovery>();
    }

    std::unique_ptr<IProtocolSession> createSession() override
    {
        return std::make_unique<UNISOCScaffoldSession>(m_protocolId, m_version);
    }

private:
    ProtocolId      m_protocolId{"unisoc"};
    ProtocolVersion m_version{UnisocMajorVersion, UnisocMinorVersion};
};

class UNISOCScaffoldProtocol : public IProtocol {
public:
    ProtocolId      id()      const override { return ProtocolId{"unisoc"}; }
    ProtocolVersion version() const override { return ProtocolVersion{UnisocMajorVersion, UnisocMinorVersion}; }

    std::unique_ptr<IProtocolFactory> factory() const override
    {
        return std::make_unique<UNISOCScaffoldFactory>();
    }
};

} // namespace mbootcore::protocol::unisoc

#else
#error "MBOOTCORE_ENABLE_VENDOR_SCAFFOLDS is not defined. \
Vendor scaffold protocols are not available in this build. \
Enable the CMake option to use them."
#endif
