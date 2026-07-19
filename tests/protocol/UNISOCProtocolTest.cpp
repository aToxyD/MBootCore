#include <catch2/catch_test_macros.hpp>

// Protocol Platform headers
#include <mbootcore/protocol/IProtocol.hpp>
#include <mbootcore/protocol/IProtocolFactory.hpp>
#include <mbootcore/protocol/IProtocolSession.hpp>
#include <mbootcore/protocol/IProtocolDiscovery.hpp>
#include <mbootcore/protocol/IMessageEncoder.hpp>
#include <mbootcore/protocol/IMessageDecoder.hpp>
#include <mbootcore/protocol/Packet.hpp>
#include <mbootcore/protocol/Request.hpp>
#include <mbootcore/protocol/Response.hpp>
#include <mbootcore/protocol/StatusCode.hpp>

// UNISOC implementation
#include "protocols/unisoc/UNISOCProtocol.hpp"

#include <cstring>

// ============================================================
// Simulated UNISOC device
// ============================================================

class SimulatedUnisocDevice : public mbootcore::ITransport {
public:
    mbootcore::Result<void> open() override
    {
        m_open = true;
        return {};
    }

    void close() noexcept override
    {
        m_open = false;
    }

    bool isOpen() const noexcept override { return m_open; }

    mbootcore::Result<size_t> write(const mbootcore::ByteBuffer& data,
                                      std::chrono::milliseconds) override
    {
        if (data.empty())
            return 0u;

        m_lastCommand = data[0];
        m_response.clear();

        switch (data[0]) {
        case 0x01: // PROBE
            m_response = {0x00, 0x01, 0x00, 0x02};
            break;
        case 0x02: // HANDSHAKE
            m_response = {0x00, 0x02, 0x00, 0x01, 0x00};
            break;
        case 0x03: // GET_VERSION
        {
            const char chip[] = "UIS8581E";
            m_response = {0x00, 0x12, 0x00};
            m_response.push_back(1);
            m_response.push_back(0);
            for (auto c : chip)
                m_response.push_back(static_cast<uint8_t>(c));
            while (m_response.size() < 21)
                m_response.push_back(0);
            break;
        }
        case 0x04: // GET_CHIP_INFO
        {
            m_response = {0x00, 0x0C, 0x00};
            auto add32 = [&](uint32_t v) {
                m_response.push_back(static_cast<uint8_t>(v & 0xFF));
                m_response.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
                m_response.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
                m_response.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
            };
            add32(0x85810001);
            add32(0x10);
            add32(0x01020304);
            break;
        }
        default:
            m_response = {0x04, 0x00, 0x00};
            break;
        }

        return data.size();
    }

    mbootcore::Result<size_t> read(mbootcore::ByteBuffer& buffer,
                                    size_t minBytes, size_t,
                                    std::chrono::milliseconds) override
    {
        if (m_response.empty())
            return 0u;

        size_t toCopy = std::min(m_response.size(), buffer.size());
        std::memcpy(buffer.data(), m_response.data(), toCopy);
        return toCopy;
    }

    mbootcore::Result<void> sendZLP(std::chrono::milliseconds) override
    {
        return {};
    }

    std::string_view name() const noexcept override { return "sim-unisoc"; }
    void cancel() noexcept override {}

private:
    bool m_open = true;
    mbootcore::ByteBuffer m_response;
    uint8_t m_lastCommand = 0;
};

// Silent device for negative probe test
struct SilentDevice : public mbootcore::ITransport {
    mbootcore::Result<void> open() override { return {}; }
    void close() noexcept override {}
    bool isOpen() const noexcept override { return true; }

    mbootcore::Result<size_t> write(const mbootcore::ByteBuffer&, std::chrono::milliseconds) override
    {
        return 0u;
    }

    mbootcore::Result<size_t> read(mbootcore::ByteBuffer&, size_t, size_t, std::chrono::milliseconds) override
    {
        return 0u;
    }

    mbootcore::Result<void> sendZLP(std::chrono::milliseconds) override
    {
        return {};
    }

    std::string_view name() const noexcept override { return "silent"; }
    void cancel() noexcept override {}
};

// ============================================================
// Tests
// ============================================================

TEST_CASE("UNISOCProtocolTest", "[protocol]") {
    SECTION("testProtocolIdAndVersion")
    {
        mbootcore::protocol::unisoc::UNISOCScaffoldProtocol proto;
        REQUIRE(proto.id().name() == std::string("unisoc"));
        REQUIRE(proto.version().major == 1u);
        REQUIRE(proto.version().minor == 0u);
    }

    SECTION("testFactoryCreatesDiscovery")
    {
        mbootcore::protocol::unisoc::UNISOCScaffoldProtocol proto;
        auto factory = proto.factory();
        REQUIRE(factory != nullptr);

        auto discovery = factory->createDiscovery();
        REQUIRE(discovery != nullptr);
    }

    SECTION("testFactoryCreatesSession")
    {
        mbootcore::protocol::unisoc::UNISOCScaffoldProtocol proto;
        auto factory = proto.factory();
        REQUIRE(factory != nullptr);

        auto session = factory->createSession();
        REQUIRE(session != nullptr);
        REQUIRE(!session->isOpen());
    }

    SECTION("testProbeDetectsUnisocDevice")
    {
        SimulatedUnisocDevice device;
        mbootcore::protocol::unisoc::UNISOCScaffoldDiscovery discovery;

        auto result = discovery.probe(device);
        REQUIRE(result.isOk());
        REQUIRE(result.value());
    }

    SECTION("testProbeReturnsFalseOnSilentDevice")
    {
        SilentDevice device;
        mbootcore::protocol::unisoc::UNISOCScaffoldDiscovery discovery;

        auto result = discovery.probe(device);
        REQUIRE(result.isOk());
        REQUIRE(!result.value());
    }

    SECTION("testVerticalSliceProbeHandshakeGetVersion")
    {
        SimulatedUnisocDevice device;
        device.open();

        mbootcore::protocol::unisoc::UNISOCScaffoldDiscovery discovery;
        auto probeResult = discovery.probe(device);
        REQUIRE(probeResult.isOk());
        REQUIRE(probeResult.value());

        mbootcore::protocol::unisoc::UNISOCScaffoldProtocol proto;
        auto factory = proto.factory();
        auto session = factory->createSession();
        auto openResult = session->open(device);
        REQUIRE(openResult.isOk());
        REQUIRE(session->isOpen());

        mbootcore::protocol::unisoc::UNISOCScaffoldEncoder encoder;
        mbootcore::protocol::unisoc::UNISOCScaffoldDecoder decoder;

        auto cmd = mbootcore::protocol::Command{
            mbootcore::protocol::CommandId{0x03},
            "get_version", ""
        };
        mbootcore::protocol::Request req{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{100}},
            cmd,
            mbootcore::protocol::Payload{}
        };

        auto encResult = encoder.encode(req);
        REQUIRE(encResult.isOk());

        auto writeResult = device.write(encResult.value().data(), std::chrono::milliseconds{1000});
        REQUIRE(writeResult.isOk());

        mbootcore::ByteBuffer respBuf(64);
        auto readResult = device.read(respBuf, 3, 64, std::chrono::milliseconds{1000});
        REQUIRE(readResult.isOk());
        respBuf.resize(readResult.value());

        mbootcore::protocol::Packet respPkt{std::move(respBuf)};
        auto decResult = decoder.decodeResponse(respPkt);
        REQUIRE(decResult.isOk());
        REQUIRE(decResult.value().status().isSuccess());
        REQUIRE(!decResult.value().payload().empty());

        auto closeResult = session->close();
        REQUIRE(closeResult.isOk());
        REQUIRE(!session->isOpen());
    }

    SECTION("testEncoderDecoderRoundTrip")
    {
        mbootcore::protocol::unisoc::UNISOCScaffoldEncoder encoder;
        mbootcore::protocol::unisoc::UNISOCScaffoldDecoder decoder;

        auto cmd = mbootcore::protocol::Command{
            mbootcore::protocol::CommandId{0x01},
            "probe", ""
        };
        mbootcore::protocol::Request req{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{10}},
            cmd,
            mbootcore::protocol::Payload{}
        };
        auto encReq = encoder.encode(req);
        REQUIRE(encReq.isOk());
        auto decReq = decoder.decodeRequest(encReq.value());
        REQUIRE(decReq.isOk());
        REQUIRE(decReq.value().command().id == mbootcore::protocol::CommandId{0x01});

        auto payload = mbootcore::protocol::Payload::copy("\x01\x00", 2);
        mbootcore::protocol::Response res{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{11}},
            mbootcore::protocol::StatusCode::success(),
            std::move(payload)
        };
        auto encRes = encoder.encode(res);
        REQUIRE(encRes.isOk());
        auto decRes = decoder.decodeResponse(encRes.value());
        REQUIRE(decRes.isOk());
        REQUIRE(decRes.value().status().isSuccess());
    }
}
