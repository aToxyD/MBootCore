#include <catch2/catch_test_macros.hpp>

#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"
#include "mbootcore/core/protocols/sahara/SaharaPackets.hpp"
#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"
#include "mbootcore/elf/ElfParser.hpp"
#include "mbootcore/domain/Types.hpp"

#include <random>
#include <climits>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

using namespace mbootcore;

// ====================================================================
// Helpers
// ====================================================================

namespace {

std::mt19937& robustRng() {
    static std::mt19937 rng{0xDEADBEEF};
    return rng;
}

size_t robustRand(size_t max) {
    std::uniform_int_distribution<size_t> dist(0, max);
    return dist(robustRng());
}

ByteBuffer randomBytes(size_t n) {
    ByteBuffer buf(n);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : buf) b = static_cast<uint8_t>(dist(robustRng()));
    return buf;
}

ByteBuffer makeSaharaPacket(uint32_t cmd, uint32_t len, const ByteBuffer& payload = {}) {
    ByteBuffer header(8);
    header[0] = static_cast<uint8_t>(cmd);
    header[1] = static_cast<uint8_t>(cmd >> 8);
    header[2] = static_cast<uint8_t>(cmd >> 16);
    header[3] = static_cast<uint8_t>(cmd >> 24);
    header[4] = static_cast<uint8_t>(len);
    header[5] = static_cast<uint8_t>(len >> 8);
    header[6] = static_cast<uint8_t>(len >> 16);
    header[7] = static_cast<uint8_t>(len >> 24);
    ByteBuffer buf = header;
    buf.insert(buf.end(), payload.begin(), payload.end());
    return buf;
}

std::string makeFirehoseXml(const std::string& inner) {
    return "<data><parameter name=\"test\" value=\"" + inner + "\"/></data>";
}

ByteBuffer makeMinimalElf64() {
    ByteBuffer buf(64 + 56 + 64, 0);
    auto* p = buf.data();
    p[0] = 0x7F; p[1] = 'E'; p[2] = 'L'; p[3] = 'F';
    p[4] = 2; p[5] = 1; p[6] = 1;
    // type = ET_EXEC (2) at offset 16 (LE)
    p[16] = 2;
    // machine = EM_AARCH64 (0xB7) at offset 18
    p[18] = 0xB7;
    // elf version = 1 at offset 20
    p[20] = 1;
    // e_phoff = 64 at offset 32
    p[32] = 64;
    // e_ehsize = 64 at offset 52
    p[52] = 64;
    // e_phentsize = 56 at offset 54
    p[54] = 56;
    // e_phnum = 1 at offset 56
    p[56] = 1;
    // e_shentsize = 64 at offset 58
    p[58] = 64;
    // e_shnum = 1 at offset 60
    p[60] = 1;
    // e_shstrndx = 0 at offset 62
    p[62] = 0;
    return buf;
}

ByteBuffer makeMinimalElf32() {
    ByteBuffer buf(52 + 32 + 64, 0);
    auto* p = buf.data();
    p[0] = 0x7F; p[1] = 'E'; p[2] = 'L'; p[3] = 'F';
    p[4] = 1; p[5] = 1; p[6] = 1;
    p[16] = 2;       // ET_EXEC
    p[18] = 40;      // EM_ARM
    p[20] = 1;       // elf version
    p[28] = 52;      // e_phoff
    p[40] = 52;      // e_ehsize
    p[42] = 32;      // e_phentsize
    p[44] = 1;       // e_phnum
    p[46] = 40;      // e_shentsize
    p[48] = 1;       // e_shnum
    return buf;
}

} // anonymous namespace

// ====================================================================
// Sahara Parser Robustness
// ====================================================================

TEST_CASE("SaharaParserRobustness", "[security][sahara][robustness]") {

    SaharaPacketParser parser;

    SECTION("empty input") {
        auto r = parser.parse({});
        REQUIRE(r.isError());
    }

    SECTION("single byte") {
        for (int b = 0; b < 256; ++b) {
            auto r = parser.parse({static_cast<uint8_t>(b)});
            REQUIRE(r.isError());
        }
    }

    SECTION("2-7 bytes (below minPacketSize)") {
        for (size_t len = 2; len < 8; ++len) {
            auto r = parser.parse(randomBytes(len));
            REQUIRE(r.isError());
        }
    }

    SECTION("exactly 8 bytes (min header)") {
        auto r = parser.parse(makeSaharaPacket(0x05, 8));
        REQUIRE(r.isOk());
    }

    SECTION("truncated — length field larger than data") {
        auto r = parser.parse(makeSaharaPacket(0x01, 1024));
        REQUIRE(r.isError());
    }

    SECTION("DonePacket with zero length is rejected") {
        auto buf = makeSaharaPacket(0x05, 0);
        buf.resize(8);
        auto r = parser.parse(buf);
        REQUIRE(r.isError());
    }

    SECTION("length field = 4 (smaller than header)") {
        auto r = parser.parse(makeSaharaPacket(0x05, 4));
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("oversized packet — declared 1MB, actual 8 bytes") {
        auto buf = makeSaharaPacket(0x01, 1024 * 1024);
        auto r = parser.parse(buf);
        REQUIRE(r.isError());
    }

    SECTION("random payloads at various sizes") {
        for (size_t len : {8, 16, 32, 64, 128, 256, 512, 1024}) {
            for (int i = 0; i < 10; ++i) {
                auto r = parser.parse(randomBytes(len));
                REQUIRE((r.isOk() || r.isError()));
            }
        }
    }

    SECTION("all valid command IDs") {
        for (uint32_t cmd = 0x01; cmd <= 0x13; ++cmd) {
            auto r = parser.parse(makeSaharaPacket(cmd, 24));
            REQUIRE((r.isOk() || r.isError()));
        }
    }

    SECTION("unknown command IDs — 0x14 through 0xFF") {
        for (uint32_t cmd = 0x14; cmd <= 0xFF; ++cmd) {
            auto r = parser.parse(makeSaharaPacket(cmd, 8));
            REQUIRE(r.isError());
        }
    }

    SECTION("max uint32 command ID") {
        auto r = parser.parse(makeSaharaPacket(0xFFFFFFFF, 8));
        REQUIRE(r.isError());
    }

    SECTION("max uint32 length field") {
        ByteBuffer buf(8, 0);
        buf[4] = 0xFF; buf[5] = 0xFF; buf[6] = 0xFF; buf[7] = 0xFF;
        buf[0] = 0x05;
        auto r = parser.parse(buf);
        REQUIRE(r.isError());
    }

    SECTION("all zero bytes") {
        auto r = parser.parse(ByteBuffer(128, 0));
        REQUIRE(r.isError());
    }

    SECTION("all 0xFF bytes") {
        auto r = parser.parse(ByteBuffer(128, 0xFF));
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("corrupted header — each byte flipped") {
        for (size_t i = 0; i < 8; ++i) {
            for (int bit = 0; bit < 8; ++bit) {
                auto buf = makeSaharaPacket(0x01, 24);
                buf[i] ^= static_cast<uint8_t>(1u << bit);
                auto r = parser.parse(buf);
                REQUIRE((r.isOk() || r.isError()));
            }
        }
    }

    SECTION("integer overflow attempt — length field near UINT32_MAX") {
        ByteBuffer buf(12, 0);
        buf[0] = 0x05;
        buf[4] = 0xFF; buf[5] = 0xFF; buf[6] = 0xFF; buf[7] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE(r.isError());
    }

    SECTION("isComplete safety") {
        REQUIRE(!parser.isComplete({}));
        REQUIRE(!parser.isComplete({0x01}));
        REQUIRE(parser.isComplete(makeSaharaPacket(0x05, 8)));
    }

    SECTION("duplicate sequential parse calls — no state leakage") {
        auto r1 = parser.parse(makeSaharaPacket(0x05, 8));
        REQUIRE(r1.isOk());
        auto r2 = parser.parse(makeSaharaPacket(0x07, 8));
        REQUIRE(r2.isOk());
        auto r3 = parser.parse(randomBytes(4));
        REQUIRE(r3.isError());
    }
}

// ====================================================================
// Firehose XML Engine Robustness
// ====================================================================

TEST_CASE("FirehoseXmlEngineRobustness", "[security][firehose][robustness]") {

    SECTION("empty string") {
        auto r = FirehoseXmlEngine::parse("");
        REQUIRE(r.isError());
    }

    SECTION("single byte — not a tag") {
        auto r = FirehoseXmlEngine::parse("x");
        REQUIRE(r.isError());
    }

    SECTION("lone opening bracket") {
        auto r = FirehoseXmlEngine::parse("<");
        REQUIRE(r.isError());
    }

    SECTION("lone closing bracket") {
        auto r = FirehoseXmlEngine::parse(">");
        REQUIRE(r.isError());
    }

    SECTION("incomplete tag — missing closing bracket") {
        auto r = FirehoseXmlEngine::parse("<data");
        REQUIRE(r.isError());
    }

    SECTION("incomplete attribute value — no closing quote") {
        auto r = FirehoseXmlEngine::parse("<data attr=\"");
        REQUIRE(r.isError());
    }

    SECTION("incomplete closing tag") {
        auto r = FirehoseXmlEngine::parse("<data></");
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("mismatched close tag") {
        auto r = FirehoseXmlEngine::parse("<data></wrong>");
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("self-closing tag") {
        auto r = FirehoseXmlEngine::parse("<data/>");
        REQUIRE(r.isOk());
    }

    SECTION("valid simple XML") {
        auto r = FirehoseXmlEngine::parse("<data value=\"ACK\"/>");
        REQUIRE(r.isOk());
        REQUIRE(r.value().name == "data");
    }

    SECTION("nested tags") {
        auto r = FirehoseXmlEngine::parse("<a><b><c/></b></a>");
        REQUIRE(r.isOk());
        REQUIRE(r.value().children.size() == 1);
    }

    SECTION("deeply nested — 200 levels") {
        std::string xml;
        for (int i = 0; i < 200; ++i) xml += "<a>";
        for (int i = 0; i < 200; ++i) xml += "</a>";
        auto r = FirehoseXmlEngine::parse(xml);
        REQUIRE(r.isError());
    }

    SECTION("depth limit — just under limit") {
        constexpr size_t kLimit = 64;
        std::string xml;
        for (size_t i = 0; i < kLimit - 1; ++i) xml += "<a>";
        for (size_t i = 0; i < kLimit - 1; ++i) xml += "</a>";
        auto r = FirehoseXmlEngine::parse(xml);
        REQUIRE(r.isOk());
    }

    SECTION("depth limit — exactly at limit") {
        constexpr size_t kLimit = 64;
        std::string xml;
        for (size_t i = 0; i < kLimit; ++i) xml += "<a>";
        for (size_t i = 0; i < kLimit; ++i) xml += "</a>";
        auto r = FirehoseXmlEngine::parse(xml);
        REQUIRE(r.isError());
    }

    SECTION("depth limit — sibling tags do not count") {
        std::string xml;
        for (size_t i = 0; i < 100; ++i) xml += "<a/>";
        auto r = FirehoseXmlEngine::parse("<root>" + xml + "</root>");
        REQUIRE(r.isOk());
    }

    SECTION("huge attribute value — 100KB") {
        std::string huge(100000, 'A');
        auto r = FirehoseXmlEngine::parse("<data v=\"" + huge + "\"/>");
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("null bytes in input") {
        std::string input = "<data\x00value=\"test\"/>";
        auto r = FirehoseXmlEngine::parse(input);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("invalid UTF-8 sequences") {
        std::string bad;
        bad += '\x80';
        bad += '\xFF';
        bad += '\xFE';
        bad += '\xC0';
        bad += '\xE0';
        bad += '\xF0';
        auto r = FirehoseXmlEngine::parse(bad);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("XML injection — entity expansion attempt") {
        std::string xml = "<!DOCTYPE foo [<!ENTITY xxe \"BOOM\">]><data>&xxe;</data>";
        auto r = FirehoseXmlEngine::parse(xml);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("random payloads at various sizes") {
        for (size_t len : {1, 2, 4, 8, 16, 32, 64, 128, 256, 512}) {
            for (int i = 0; i < 5; ++i) {
                std::string s;
                auto bytes = randomBytes(len);
                s.reserve(len);
                for (auto b : bytes) s += static_cast<char>(b);
                auto r = FirehoseXmlEngine::parse(s);
                REQUIRE((r.isOk() || r.isError()));
            }
        }
    }

    SECTION("serialize round-trip — no crash on empty element") {
        XmlElement elem;
        elem.name = "test";
        auto r = FirehoseXmlEngine::serialize(elem);
        REQUIRE(r.isOk());
    }

    SECTION("escape functions — no crash on empty input") {
        REQUIRE(FirehoseXmlEngine::escapeAttribute("") == "");
        REQUIRE(FirehoseXmlEngine::escapeContent("") == "");
    }

    SECTION("escape functions — special characters") {
        REQUIRE(FirehoseXmlEngine::escapeAttribute("a&b\"c") == "a&amp;b&quot;c");
        REQUIRE(FirehoseXmlEngine::escapeContent("a<b>c") == "a&lt;b&gt;c");
    }

    SECTION("utility functions on empty element") {
        XmlElement elem;
        elem.name = "test";
        REQUIRE(!FirehoseXmlEngine::hasAttribute(elem, "x"));
        REQUIRE(FirehoseXmlEngine::getAttribute(elem, "x", "default") == "default");
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "x", 42) == 42);
        REQUIRE(!FirehoseXmlEngine::isAck(elem));
        REQUIRE(!FirehoseXmlEngine::isNak(elem));
        REQUIRE(FirehoseXmlEngine::nakDescription(elem).empty());
    }

    SECTION("getUintAttribute — malformed numbers") {
        XmlElement elem;
        elem.name = "test";
        elem.attributes.push_back({"val", ""});
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "not_a_number";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "0xFFFFFFFFFFFFFFFF";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "0x";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 77) == 77);
    }

    SECTION("getUintAttribute — boundary values") {
        XmlElement elem;
        elem.name = "test";
        elem.attributes.push_back({"val", ""});

        elem.attributes[0].value = "0";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 0);

        elem.attributes[0].value = "1";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 1);

        elem.attributes[0].value = "4294967295";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == UINT32_MAX);

        elem.attributes[0].value = "4294967296";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "0xFFFFFFFF";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == UINT32_MAX);

        elem.attributes[0].value = "0x100000000";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "-1";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "99999999999";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "+42";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 42);

        elem.attributes[0].value = "  42  ";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 42);

        elem.attributes[0].value = "0x0";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 0);

        elem.attributes[0].value = "12abc";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);

        elem.attributes[0].value = "12.34";
        REQUIRE(FirehoseXmlEngine::getUintAttribute(elem, "val", 99) == 99);
    }

    SECTION("serialize with special characters in attribute") {
        XmlElement elem;
        elem.name = "param";
        elem.attributes.push_back({"v", "a&b\"c<d>e"});
        auto r = FirehoseXmlEngine::serialize(elem);
        REQUIRE(r.isOk());
        REQUIRE(r.value().find("a&amp;b&quot;c&lt;d&gt;e") != std::string::npos);
    }

    SECTION("serialize deeply nested") {
        XmlElement root;
        root.name = "root";
        auto* cur = &root;
        for (int i = 0; i < 50; ++i) {
            XmlElement child;
            child.name = "level" + std::to_string(i);
            cur->children.push_back(std::move(child));
            cur = &cur->children.back();
        }
        auto r = FirehoseXmlEngine::serialize(root);
        REQUIRE(r.isOk());
    }
}

// ====================================================================
// ELF Parser Robustness
// ====================================================================

TEST_CASE("ElfParserRobustness", "[security][elf][robustness]") {

    elf::ElfParser parser;

    SECTION("empty input") {
        auto r = parser.parse({});
        REQUIRE(r.isError());
    }

    SECTION("single byte") {
        auto r = parser.parse({0x7F});
        REQUIRE(r.isError());
    }

    SECTION("wrong magic — first 3 bytes only") {
        auto r = parser.parse({0x7F, 'E', 'L'});
        REQUIRE(r.isError());
    }

    SECTION("wrong magic — null bytes") {
        auto r = parser.parse({0x00, 0x00, 0x00, 0x00});
        REQUIRE(r.isError());
    }

    SECTION("wrong magic — 'PE' header") {
        auto r = parser.parse({0x4D, 0x5A, 0x00, 0x00});
        REQUIRE(r.isError());
    }

    SECTION("valid ELF magic but truncated") {
        auto r = parser.parse({0x7F, 'E', 'L', 'F'});
        REQUIRE(r.isError());
    }

    SECTION("valid ELF64 header — truncated after header") {
        auto full = makeMinimalElf64();
        ByteBuffer truncated(full.begin(), full.begin() + 20);
        auto r = parser.parse(truncated);
        REQUIRE(r.isError());
    }

    SECTION("single byte at a time — incremental corruption") {
        auto full = makeMinimalElf64();
        for (size_t n = 1; n <= full.size(); ++n) {
            ByteBuffer partial(full.begin(), full.begin() + n);
            auto r = parser.parse(partial);
            REQUIRE((r.isOk() || r.isError()));
        }
    }

    SECTION("random payloads at various sizes") {
        for (size_t len : {4, 8, 16, 32, 64, 128, 256, 512}) {
            for (int i = 0; i < 5; ++i) {
                auto r = parser.parse(randomBytes(len));
                REQUIRE((r.isOk() || r.isError()));
            }
        }
    }

    SECTION("corrupted ELF magic — flip each bit") {
        ByteBuffer buf = makeMinimalElf64();
        for (int bit = 0; bit < 8; ++bit) {
            ByteBuffer corrupted = buf;
            corrupted[0] ^= static_cast<uint8_t>(1u << bit);
            auto r = parser.parse(corrupted);
            REQUIRE((r.isOk() || r.isError()));
        }
    }

    SECTION("invalid class field") {
        ByteBuffer buf = makeMinimalElf64();
        buf[4] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("invalid endian field") {
        ByteBuffer buf = makeMinimalElf64();
        buf[5] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("huge phnum — 65535") {
        ByteBuffer buf = makeMinimalElf64();
        buf[56] = 0xFF;
        buf[57] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("huge shnum — 65535") {
        ByteBuffer buf = makeMinimalElf64();
        buf[60] = 0xFF;
        buf[61] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("phoff beyond data") {
        ByteBuffer buf = makeMinimalElf64();
        buf[32] = 0xFF; buf[33] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("shoff beyond data") {
        ByteBuffer buf = makeMinimalElf64();
        buf[40] = 0xFF; buf[41] = 0xFF;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("zero-length program headers") {
        ByteBuffer buf = makeMinimalElf64();
        buf[56] = 0;
        buf[57] = 0;
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("all 0xFF bytes — 1KB") {
        auto r = parser.parse(ByteBuffer(1024, 0xFF));
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("all zero bytes — 1KB") {
        auto r = parser.parse(ByteBuffer(1024, 0));
        REQUIRE((r.isError()));
    }

    SECTION("quick check safety — no crash on any input") {
        for (size_t len : {0, 1, 2, 3, 4, 5, 6, 16, 64, 256}) {
            auto buf = randomBytes(len);
            (void)parser.isElf(buf);
            (void)parser.is32Bit(buf);
            (void)parser.is64Bit(buf);
            (void)parser.identifyClass(buf);
        }
    }

    SECTION("static header readers — no crash on small inputs") {
        for (size_t len = 0; len <= 32; ++len) {
            auto buf = randomBytes(len);
            (void)elf::ElfParser::readHeader(buf);
            (void)elf::ElfParser::readClass(buf);
            (void)elf::ElfParser::readEndian(buf);
            (void)elf::ElfParser::readMachine(buf);
            (void)elf::ElfParser::readEntry(buf);
        }
    }

    SECTION("valid ELF32 parses without crash") {
        auto buf = makeMinimalElf32();
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("valid ELF64 parses without crash") {
        auto buf = makeMinimalElf64();
        auto r = parser.parse(buf);
        REQUIRE((r.isOk() || r.isError()));
    }

    SECTION("duplicate parse calls — no state leakage") {
        auto buf = makeMinimalElf64();
        auto r1 = parser.parse(buf);
        REQUIRE(r1.isOk());
        auto r2 = parser.parse(ByteBuffer(4, 0));
        REQUIRE(r2.isError());
        auto r3 = parser.parse(buf);
        REQUIRE(r3.isOk());
    }
}

// ====================================================================
// Cross-parser Stress — rapid interleaved parsing
// ====================================================================

TEST_CASE("CrossParserStress", "[security][robustness]") {

    SaharaPacketParser sahara;
    elf::ElfParser elf;

    SECTION("interleaved random inputs") {
        for (int i = 0; i < 100; ++i) {
                auto buf = randomBytes(static_cast<size_t>(robustRand(255) + 1));
            auto r1 = sahara.parse(buf);
            REQUIRE((r1.isOk() || r1.isError()));
            auto r2 = elf.parse(buf);
            REQUIRE((r2.isOk() || r2.isError()));
        }
    }

    SECTION("rapid firehose + sahara alternation") {
        for (int i = 0; i < 50; ++i) {
            auto saharaBuf = randomBytes(static_cast<size_t>(robustRand(127) + 8));
            auto r1 = sahara.parse(saharaBuf);
            REQUIRE((r1.isOk() || r1.isError()));

            std::string xml;
            auto xmlBytes = randomBytes(static_cast<size_t>(robustRand(63) + 1));
            xml.reserve(xmlBytes.size());
            for (auto b : xmlBytes) xml += static_cast<char>(b);
            auto r2 = FirehoseXmlEngine::parse(xml);
            REQUIRE((r2.isOk() || r2.isError()));
        }
    }

    SECTION("fuzz all parsers with max-size buffers") {
        auto huge = randomBytes(65536);
        auto r1 = sahara.parse(huge);
        REQUIRE((r1.isOk() || r1.isError()));
        auto r2 = elf.parse(huge);
        REQUIRE((r2.isOk() || r2.isError()));

        std::string hugeXml(huge.begin(), huge.end());
        auto r3 = FirehoseXmlEngine::parse(hugeXml);
        REQUIRE((r3.isOk() || r3.isError()));
    }
}
