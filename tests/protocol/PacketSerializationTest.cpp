#include <catch2/catch_test_macros.hpp>

// Serialization layer headers
#include <mbootcore/protocol/Packet.hpp>
#include <mbootcore/protocol/IMessageEncoder.hpp>
#include <mbootcore/protocol/IMessageDecoder.hpp>
#include <mbootcore/protocol/SerializationContext.hpp>

// Semantic layer headers (must compile without serialization types)
#include <mbootcore/protocol/Request.hpp>
#include <mbootcore/protocol/Response.hpp>
#include <mbootcore/protocol/Event.hpp>
#include <mbootcore/protocol/StatusCode.hpp>
#include <mbootcore/protocol/Payload.hpp>

// ============================================================
// Compile-time checks
// ============================================================

// Packet must be move-constructible
static_assert(std::is_move_constructible_v<mbootcore::protocol::Packet>);

// Endianness must be trivially copyable
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::Endianness>);

// Identity types must be trivially copyable
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::MessageId>);
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::TransactionId>);
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::CommandId>);

// ============================================================
// Mock encoder/decoder for testing
// ============================================================

class MockEncoder : public mbootcore::protocol::IMessageEncoder {
public:
    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Packet>
    encode(const mbootcore::protocol::Request& req) override
    {
        auto cmd = req.command();
        auto idVal = cmd.id.value();
        mbootcore::ByteBuffer raw;
        raw.push_back(static_cast<uint8_t>(idVal & 0xFF));
        raw.push_back(static_cast<uint8_t>((idVal >> 8) & 0xFF));
        raw.push_back(static_cast<uint8_t>((idVal >> 16) & 0xFF));
        raw.push_back(static_cast<uint8_t>((idVal >> 24) & 0xFF));
        return mbootcore::protocol::Packet{std::move(raw)};
    }

    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Packet>
    encode(const mbootcore::protocol::Response& res) override
    {
        mbootcore::ByteBuffer raw;
        raw.push_back(static_cast<uint8_t>(res.status().isSuccess() ? 0 : res.status().reason()));
        return mbootcore::protocol::Packet{std::move(raw)};
    }

    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Packet>
    encode(const mbootcore::protocol::Event& ev) override
    {
        mbootcore::ByteBuffer raw;
        raw.reserve(ev.payload().size());
        for (auto b : ev.payload())
            raw.push_back(static_cast<uint8_t>(b));
        return mbootcore::protocol::Packet{std::move(raw)};
    }
};

class MockDecoder : public mbootcore::protocol::IMessageDecoder {
public:
    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Request>
    decodeRequest(const mbootcore::protocol::Packet& pkt) override
    {
        if (pkt.data().empty())
            return mbootcore::ErrorCode::ProtocolError;
        auto id = mbootcore::protocol::CommandId{pkt.data()[0]};
        mbootcore::protocol::Command cmd{id, "mock", ""};
        return mbootcore::protocol::Request{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            cmd,
            mbootcore::protocol::Payload{}
        };
    }

    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Response>
    decodeResponse(const mbootcore::protocol::Packet& pkt) override
    {
        if (pkt.data().empty())
            return mbootcore::ErrorCode::ProtocolError;
        auto status = (pkt.data()[0] == 0)
            ? mbootcore::protocol::StatusCode::success()
            : mbootcore::protocol::StatusCode::failure(pkt.data()[0]);
        return mbootcore::protocol::Response{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            status,
            mbootcore::protocol::Payload{}
        };
    }

    mbootcore::protocol::ProtocolResult<mbootcore::protocol::Event>
    decodeEvent(const mbootcore::protocol::Packet& pkt) override
    {
        if (pkt.data().empty())
            return mbootcore::ErrorCode::ProtocolError;
        auto payload = mbootcore::protocol::Payload::copy(pkt.data().data(), pkt.data().size());
        return mbootcore::protocol::Event{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            std::move(payload)
        };
    }
};

// ============================================================
// Tests
// ============================================================

TEST_CASE("PacketSerializationTest", "[protocol]") {
    SECTION("testPacketDefaultEmpty")
    {
        mbootcore::protocol::Packet p;
        REQUIRE(p.empty());
        REQUIRE(p.size() == 0u);
    }

    SECTION("testPacketFromData")
    {
        mbootcore::ByteBuffer data{static_cast<uint8_t>(0x01), static_cast<uint8_t>(0x02), static_cast<uint8_t>(0x03)};
        mbootcore::protocol::Packet p{data};
        REQUIRE(!p.empty());
        REQUIRE(p.size() == 3u);
        REQUIRE(p.data() == data);
    }

    SECTION("testPacketEquality")
    {
        auto mkPkt = [](uint8_t a, uint8_t b, uint8_t c) {
            return mbootcore::protocol::Packet{mbootcore::ByteBuffer{a, b, c}};
        };
        REQUIRE(mkPkt(1, 2, 3) == mkPkt(1, 2, 3));
        REQUIRE(mkPkt(1, 2, 3) != mkPkt(4, 5, 6));
    }

    SECTION("testPacketClear")
    {
        mbootcore::protocol::Packet p{mbootcore::ByteBuffer{static_cast<uint8_t>(1)}};
        REQUIRE(!p.empty());
        p.clear();
        REQUIRE(p.empty());
    }

    SECTION("testPacketMoveSemantics")
    {
        mbootcore::protocol::Packet p{mbootcore::ByteBuffer{static_cast<uint8_t>(1)}};
        mbootcore::protocol::Packet moved{std::move(p)};
        REQUIRE(moved.size() == 1u);
    }

    SECTION("testMockEncoderRequest")
    {
        MockEncoder encoder;
        auto cmd = mbootcore::protocol::Command{mbootcore::protocol::CommandId{0xAA}, "test", ""};
        mbootcore::protocol::Request req{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            cmd,
            mbootcore::protocol::Payload{}
        };
        auto result = encoder.encode(req);
        REQUIRE(result.isOk());
        REQUIRE(result.value().data().size() == 4u);
    }

    SECTION("testMockEncoderResponse")
    {
        MockEncoder encoder;
        mbootcore::protocol::Response res{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            mbootcore::protocol::StatusCode::success(),
            mbootcore::protocol::Payload{}
        };
        auto result = encoder.encode(res);
        REQUIRE(result.isOk());
        REQUIRE(result.value().data()[0] == 0u);
    }

    SECTION("testMockEncoderResponseFailure")
    {
        MockEncoder encoder;
        mbootcore::protocol::Response res{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            mbootcore::protocol::StatusCode::failure(0xFF),
            mbootcore::protocol::Payload{}
        };
        auto result = encoder.encode(res);
        REQUIRE(result.isOk());
        REQUIRE(result.value().data()[0] == 0xFFu);
    }

    SECTION("testMockEncoderEvent")
    {
        MockEncoder encoder;
        const uint8_t payloadData[] = {0x10, 0x20, 0x30};
        auto payload = mbootcore::protocol::Payload::copy(payloadData, 3);
        mbootcore::protocol::Event ev{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{1}},
            std::move(payload)
        };
        auto result = encoder.encode(ev);
        REQUIRE(result.isOk());
        REQUIRE(result.value().size() == 3u);
    }

    SECTION("testMockDecoderRequest")
    {
        MockDecoder decoder;
        mbootcore::ByteBuffer data{static_cast<uint8_t>(0x42)};
        mbootcore::protocol::Packet pkt{std::move(data)};
        auto result = decoder.decodeRequest(pkt);
        REQUIRE(result.isOk());
        REQUIRE(result.value().command().id == mbootcore::protocol::CommandId{0x42});
    }

    SECTION("testMockDecoderResponse")
    {
        MockDecoder decoder;
        mbootcore::ByteBuffer data{static_cast<uint8_t>(0x00)};
        mbootcore::protocol::Packet pkt{std::move(data)};
        auto result = decoder.decodeResponse(pkt);
        REQUIRE(result.isOk());
        REQUIRE(result.value().status().isSuccess());
    }

    SECTION("testMockDecoderResponseFailure")
    {
        MockDecoder decoder;
        mbootcore::ByteBuffer data{static_cast<uint8_t>(0xAB)};
        mbootcore::protocol::Packet pkt{std::move(data)};
        auto result = decoder.decodeResponse(pkt);
        REQUIRE(result.isOk());
        REQUIRE(result.value().status().isFailure());
        REQUIRE(result.value().status().reason() == 0xABu);
    }

    SECTION("testMockEncoderDecodeRoundTrip")
    {
        MockEncoder encoder;
        MockDecoder decoder;

        auto cmd = mbootcore::protocol::Command{mbootcore::protocol::CommandId{0x77}, "roundtrip", ""};
        mbootcore::protocol::Request req{
            mbootcore::protocol::MessageMetadata{mbootcore::protocol::MessageId{10}},
            cmd,
            mbootcore::protocol::Payload{}
        };

        auto encodeResult = encoder.encode(req);
        REQUIRE(encodeResult.isOk());

        auto decodeResult = decoder.decodeRequest(encodeResult.value());
        REQUIRE(decodeResult.isOk());
        REQUIRE(decodeResult.value().command().id == mbootcore::protocol::CommandId{0x77});
    }

    SECTION("testSerializationContextDefaults")
    {
        mbootcore::protocol::SerializationContext ctx;
        REQUIRE(ctx.version.major == 0u);
        REQUIRE(ctx.version.minor == 0u);
        REQUIRE(static_cast<int>(ctx.endianness) ==
                 static_cast<int>(mbootcore::protocol::Endianness::Little));
        REQUIRE(ctx.negotiatedCapabilities.empty());
    }

    SECTION("testSerializationContextCustomVersion")
    {
        mbootcore::protocol::SerializationContext ctx{
            mbootcore::protocol::ProtocolVersion{2, 1},
            mbootcore::protocol::Endianness::Big,
            {}
        };
        REQUIRE(ctx.version.major == 2u);
        REQUIRE(ctx.version.minor == 1u);
        REQUIRE(static_cast<int>(ctx.endianness) ==
                 static_cast<int>(mbootcore::protocol::Endianness::Big));
    }
}
