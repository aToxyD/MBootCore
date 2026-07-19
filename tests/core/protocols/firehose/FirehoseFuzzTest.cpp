#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"
#include "mbootcore/core/protocols/firehose/FirehosePackets.hpp"

#include <random>
#include <string>

using namespace mbootcore;

namespace {

std::mt19937 m_rng{42};

std::string randomXml(size_t approxSize) {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789=\"<>/ \n\t";
    std::string result;
    result.reserve(approxSize);
    for (size_t i = 0; i < approxSize; ++i) {
        result += chars[m_rng() % (sizeof(chars) - 1)];
    }
    return result;
}

void fuzzParse(const std::string& xml) {
    auto result = FirehoseXmlEngine::parse(xml);
    (void)result;
}

void fuzzRoundTrip(const std::string& xml) {
    auto parsed = FirehoseXmlEngine::parse(xml);
    if (!parsed.isOk()) return;
    auto serialized = FirehoseXmlEngine::serialize(parsed.value());
    if (!serialized.isOk()) return;
    auto reparsed = FirehoseXmlEngine::parse(serialized.value());
    (void)reparsed;
}

} // anonymous namespace

TEST_CASE("FirehoseFuzzTest", "[firehose]") {
    SECTION("testEmptyString")
    {
        fuzzParse("");
    }

    SECTION("testRandomStrings")
    {
        for (size_t len : {4, 8, 16, 32, 64, 128, 256}) {
            for (int i = 0; i < 20; ++i) {
                fuzzParse(randomXml(len));
            }
        }
    }

    SECTION("testPartialTags")
    {
        fuzzParse("<open");
        fuzzParse("<open att");
        fuzzParse("<open attr=\"");
        fuzzParse("<open attr=\"value");
        fuzzParse("<open attr=\"value\"");
        fuzzParse("<open attr=\"value\" /");
        fuzzParse("</");
        fuzzParse("</close");
    }

    SECTION("testNestedRandom")
    {
        fuzzParse("<a><b><c><d/></c></b></a>");
        fuzzParse("<a><b><c><d/></b></a>");
        fuzzParse("<a><b attr=\"1\" c=\"2\"><d e=\"3\"/></b></a>");
    }

    SECTION("testMalformedUtf8")
    {
        std::string bad;
        bad += '\x80';
        bad += '\xFF';
        bad += '\xFE';
        fuzzParse(bad);
    }

    SECTION("testHugeAttributeValues")
    {
        std::string huge(10000, 'A');
        std::string xml = "<test attr=\"" + huge + "\"/>";
        fuzzParse(xml);
    }

    SECTION("testDeeplyNestedXml")
    {
        std::string xml;
        for (int i = 0; i < 100; ++i) {
            xml += "<a" + std::to_string(i) + ">";
        }
        for (int i = 99; i >= 0; --i) {
            xml += "</a" + std::to_string(i) + ">";
        }
        fuzzParse(xml);
    }

    SECTION("testKnownCommandsWithRandomAttributes")
    {
        std::vector<std::string> cmds = {
            "<configure %s/>",
            "<program %s/>",
            "<read %s/>",
            "<erase %s/>",
            "<peek %s/>",
            "<reset/>",
            "<NOP/>",
            "<getstorageinfo/>",
        };

        for (auto& cmd : cmds) {
            std::string xml = cmd;
            size_t pos = xml.find("%s");
            if (pos != std::string::npos) {
                std::string attrs;
                for (int i = 0; i < 5; ++i) {
                    attrs += " attr" + std::to_string(i) + "=\"value" + std::to_string(m_rng() % 1000) + "\"";
                }
                xml.replace(pos, 2, attrs);
                fuzzParse(xml);
            }
        }
    }

    SECTION("testRoundTripAfterFuzz")
    {
        auto xml = randomXml(64);
        fuzzRoundTrip(xml);
    }

    SECTION("testLargeEmptyTags")
    {
        std::string xml = "<a/>";
        for (int i = 0; i < 1000; ++i) {
            xml += "<b" + std::to_string(i) + "/>";
        }
        fuzzParse(xml);
    }

    SECTION("testResponseParsingWithJunk")
    {
        auto resp = FirehoseResponse::fromXml("junk");
        REQUIRE(!resp.isAck());
        REQUIRE(!resp.isNak());

        resp = FirehoseResponse::fromXml("");
        REQUIRE(!resp.isAck());
        REQUIRE(!resp.isNak());

        resp = FirehoseResponse::fromXml("<><>");
        REQUIRE(!resp.isAck());
    }
}
