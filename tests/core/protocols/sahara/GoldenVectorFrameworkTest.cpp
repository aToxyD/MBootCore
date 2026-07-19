#include <catch2/catch_test_macros.hpp>
#include <GoldenVectorFramework.hpp>
#include <mbootcore/core/protocols/sahara/SaharaPacketSerializer.hpp>
#include <mbootcore/core/protocols/sahara/SaharaPacketParser.hpp>
#include <mbootcore/core/protocols/sahara/SaharaPackets.hpp>

using namespace mbootcore::testing;
using namespace mbootcore;

template <typename ConcretePacket>
static std::vector<uint8_t> serializePacket(SaharaPacketSerializer& ser, const ConcretePacket& pkt) {
    auto result = ser.serialize(pkt);
    if (result.isError()) return {};
    return result.value();
}

template <typename ConcretePacket>
static ConcretePacket parsePacket(SaharaPacketParser& parser, const std::vector<uint8_t>& bytes) {
    auto result = parser.parse(bytes);
    REQUIRE(result.isOk());
    auto& ptr = result.value();
    auto pkt = dynamic_cast<ConcretePacket*>(ptr.get());
    REQUIRE(pkt != nullptr);
    return *pkt;
}

TEST_CASE("GoldenVectorFramework", "[golden-vector][framework]") {
    SECTION("frameworkCreatesSuite") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, HelloPacket> suite(
            "HelloTest",
            serializePacket<HelloPacket>,
            parsePacket<HelloPacket>);

        suite.addVector({"helloRoundTrip",
                         HelloPacket(2, 2, 4096, 0),
                         {},
                         false,
                         "Hello packet round-trip"});

        REQUIRE(suite.name() == "HelloTest");
        REQUIRE(suite.vectorCount() == 1);
    }

    SECTION("helloPacketRoundTrip") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, HelloPacket> suite(
            "HelloRoundTrip",
            serializePacket<HelloPacket>,
            parsePacket<HelloPacket>);

        suite.addVector({"helloBasic",
                         HelloPacket(2, 2, 4096, 0),
                         {},
                         false,
                         "Hello packet round-trip"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 1);
        REQUIRE(summary.passedVectors == 1);
        REQUIRE(summary.failedVectors == 0);
        REQUIRE(summary.totalTime.count() > 0);

        suite.report(summary);
    }

    SECTION("goldenVectorResultFields") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, HelloPacket> suite(
            "FieldTest",
            serializePacket<HelloPacket>,
            parsePacket<HelloPacket>);

        suite.addVector({"helloVerifyBytes",
                         HelloPacket(2, 2, 4096, 0),
                         {0x01, 0x00, 0x00, 0x00,
                          0x02, 0x00, 0x00, 0x00,
                          0x02, 0x00, 0x00, 0x00,
                          0x00, 0x10, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00},
                         true,
                         "Hello raw bytes verification"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 1);
        REQUIRE(summary.results[0].outputBytes == 48);
    }

    SECTION("multipleHelloVectorsBatch") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, HelloPacket> suite(
            "BatchHello",
            serializePacket<HelloPacket>,
            parsePacket<HelloPacket>);

        suite.addVector({"hello_v2", HelloPacket(2, 2, 4096, 0), {}, false, "v2"});
        suite.addVector({"hello_v3", HelloPacket(3, 3, 4096, 0), {}, false, "v3"});
        suite.addVector({"hello_mode1", HelloPacket(2, 2, 4096, 1), {}, false, "mode1"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 3);
        REQUIRE(summary.passedVectors == 3);
        REQUIRE(summary.failedVectors == 0);
    }

    SECTION("donePacketVectors") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, DonePacket> suite(
            "DoneTest",
            serializePacket<DonePacket>,
            parsePacket<DonePacket>);

        suite.addVector({"doneBasic", DonePacket(), {}, false, "Done packet"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 1);
        REQUIRE(summary.passedVectors == 1);
    }

    SECTION("resetPacketVectors") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, ResetPacket> suite(
            "ResetTest",
            serializePacket<ResetPacket>,
            parsePacket<ResetPacket>);

        suite.addVector({"resetBasic", ResetPacket(), {}, false, "Reset packet"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 1);
        REQUIRE(summary.passedVectors == 1);
    }

    SECTION("readDataPacketVectors") {
        GoldenVectorSuite<SaharaPacketSerializer, SaharaPacketParser, ReadDataPacket> suite(
            "ReadDataTest",
            serializePacket<ReadDataPacket>,
            parsePacket<ReadDataPacket>);

        suite.addVector({"readDataBasic", ReadDataPacket(0, 0x8000, 4096), {}, false, "ReadData packet"});
        suite.addVector({"readDataLarge", ReadDataPacket(1, 0, 0x100000), {}, false, "Large read"});

        auto summary = suite.runAll();
        REQUIRE(summary.totalVectors == 2);
        REQUIRE(summary.passedVectors == 2);
    }
}
