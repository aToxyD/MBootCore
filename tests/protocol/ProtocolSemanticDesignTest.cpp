#include <catch2/catch_test_macros.hpp>

// Semantic Vocabulary — must compile without any serialization headers.
//
// This file is itself the design test: if any semantic header introduces a
// transitive dependency on Encoder, Decoder, Packet, or ByteBuffer,
// this file either fails to compile or (when those headers exist) pulls in
// serialization types — which a corresponding negative check will catch.
//
// Negative checks verify that serialization header guards
// (e.g. MBOOTCORE_PROTOCOL_PACKET_HPP) are NOT defined after including
// all semantic headers.

#include <mbootcore/protocol/Command.hpp>
#include <mbootcore/protocol/CommandId.hpp>
#include <mbootcore/protocol/Event.hpp>
#include <mbootcore/protocol/MessageId.hpp>
#include <mbootcore/protocol/MessageMetadata.hpp>
#include <mbootcore/protocol/Payload.hpp>
#include <mbootcore/protocol/Request.hpp>
#include <mbootcore/protocol/Response.hpp>
#include <mbootcore/protocol/StatusCode.hpp>
#include <mbootcore/protocol/TransactionId.hpp>

// ============================================================
// Compile-time checks
// ============================================================

// Types must be move-constructible (no raw owning pointers)
static_assert(std::is_move_constructible_v<mbootcore::protocol::Payload>);

// Identity types must be trivially copyable value types
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::CommandId>);
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::MessageId>);
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::TransactionId>);
static_assert(std::is_trivially_copyable_v<mbootcore::protocol::StatusCode>);

// Command must be a standard-layout aggregate
static_assert(std::is_standard_layout_v<mbootcore::protocol::Command>);
static_assert(std::is_aggregate_v<mbootcore::protocol::Command>);

// ============================================================
// Runtime tests
// ============================================================

TEST_CASE("ProtocolSemanticDesignTest", "[protocol]") {
    SECTION("testCommandIdEquality")
    {
        using mbootcore::protocol::CommandId;
        REQUIRE(CommandId{1} == CommandId{1});
        REQUIRE(CommandId{1} != CommandId{2});
        REQUIRE(CommandId{1} < CommandId{2});
    }

    SECTION("testMessageIdEquality")
    {
        using mbootcore::protocol::MessageId;
        REQUIRE(MessageId{100} == MessageId{100});
        REQUIRE(MessageId{100} != MessageId{200});
    }

    SECTION("testTransactionIdEquality")
    {
        using mbootcore::protocol::TransactionId;
        REQUIRE(TransactionId{42} == TransactionId{42});
        REQUIRE(TransactionId{42} != TransactionId{0});
    }

    SECTION("testStatusCodeSuccess")
    {
        using mbootcore::protocol::StatusCode;
        auto s = StatusCode::success();
        REQUIRE(s.isSuccess());
        REQUIRE(!s.isFailure());
    }

    SECTION("testStatusCodeFailure")
    {
        using mbootcore::protocol::StatusCode;
        auto f = StatusCode::failure(0x1234);
        REQUIRE(!f.isSuccess());
        REQUIRE(f.isFailure());
        REQUIRE(f.reason() == 0x1234u);
    }

    SECTION("testStatusCodeSuccessNotEqualFailure")
    {
        using mbootcore::protocol::StatusCode;
        REQUIRE(StatusCode::success() != StatusCode::failure(0));
    }

    SECTION("testPayloadEmpty")
    {
        using mbootcore::protocol::Payload;
        Payload p;
        REQUIRE(p.empty());
        REQUIRE(p.size() == 0u);
    }

    SECTION("testPayloadCopy")
    {
        using mbootcore::protocol::Payload;
        const char data[] = "hello";
        auto p = Payload::copy(data, 5);
        REQUIRE(!p.empty());
        REQUIRE(p.size() == 5u);
        REQUIRE(p.begin() != nullptr);
        REQUIRE(p.end() == p.begin() + 5);
    }

    SECTION("testPayloadEquality")
    {
        using mbootcore::protocol::Payload;
        auto a = Payload::copy("abc", 3);
        auto b = Payload::copy("abc", 3);
        auto c = Payload::copy("xyz", 3);
        REQUIRE(a == b);
        REQUIRE(a != c);
    }

    SECTION("testRequestComposition")
    {
        using namespace mbootcore::protocol;
        auto cmd = Command{CommandId{1}, "read", "Read memory"};
        auto payload = Payload::copy("data", 4);
        Request req{MessageMetadata{MessageId{10}}, cmd, std::move(payload)};
        REQUIRE(req.metadata().messageId == MessageId{10});
        REQUIRE(req.command().id == CommandId{1});
        REQUIRE(!req.payload().empty());
    }

    SECTION("testResponseComposition")
    {
        using namespace mbootcore::protocol;
        auto payload = Payload::copy("resp", 4);
        Response res{MessageMetadata{MessageId{20}}, StatusCode::success(), std::move(payload)};
        REQUIRE(res.metadata().messageId == MessageId{20});
        REQUIRE(res.status().isSuccess());
        REQUIRE(!res.payload().empty());
    }

    SECTION("testEventComposition")
    {
        using namespace mbootcore::protocol;
        auto payload = Payload::copy("evt", 3);
        Event ev{MessageMetadata{MessageId{30}}, std::move(payload)};
        REQUIRE(ev.metadata().messageId == MessageId{30});
        REQUIRE(!ev.payload().empty());
    }

    SECTION("testEventDefaultTransactionIdIsZero")
    {
        using namespace mbootcore::protocol;
        Event ev{MessageMetadata{MessageId{1}}, Payload{}};
        REQUIRE(ev.metadata().transactionId == TransactionId{0});
    }

    SECTION("testMessageMetadataEquality")
    {
        using namespace mbootcore::protocol;
        REQUIRE(MessageMetadata{MessageId{1}} == MessageMetadata{MessageId{1}});
        REQUIRE(MessageMetadata{MessageId{1}} != MessageMetadata{MessageId{2}});
    }
}
