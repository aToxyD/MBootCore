#include <catch2/catch_test_macros.hpp>

#include "SafeParser.hpp"

#include <climits>
#include <string>
#include <string_view>

using namespace mbootcore;

// ====================================================================
// fromCharsUint64
// ====================================================================

TEST_CASE("fromCharsUint64", "[security][parser][safe]") {

    SECTION("valid decimal") {
        auto r = fromCharsUint64("0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);

        r = fromCharsUint64("1");
        REQUIRE(r.ok);
        REQUIRE(r.value == 1);

        r = fromCharsUint64("42");
        REQUIRE(r.ok);
        REQUIRE(r.value == 42);

        r = fromCharsUint64("18446744073709551615");
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT64_MAX);
    }

    SECTION("valid hex") {
        auto r = fromCharsUint64("0x0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);

        r = fromCharsUint64("0xFF");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0xFF);

        r = fromCharsUint64("0X100");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0x100);

        r = fromCharsUint64("0xFFFFFFFFFFFFFFFF");
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT64_MAX);
    }

    SECTION("with leading/trailing whitespace") {
        auto r = fromCharsUint64("  42  ");
        REQUIRE(r.ok);
        REQUIRE(r.value == 42);

        r = fromCharsUint64("\t0xFF\n");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0xFF);
    }

    SECTION("with leading plus") {
        auto r = fromCharsUint64("+42");
        REQUIRE(r.ok);
        REQUIRE(r.value == 42);
    }

    SECTION("empty input") {
        REQUIRE_FALSE(fromCharsUint64("").ok);
        REQUIRE_FALSE(fromCharsUint64("   ").ok);
        REQUIRE_FALSE(fromCharsUint64("\t\n").ok);
    }

    SECTION("negative values rejected") {
        REQUIRE_FALSE(fromCharsUint64("-1").ok);
        REQUIRE_FALSE(fromCharsUint64("-0").ok);
        REQUIRE_FALSE(fromCharsUint64(" -42 ").ok);
    }

    SECTION("leading plus then empty rejected") {
        REQUIRE_FALSE(fromCharsUint64("+").ok);
        REQUIRE_FALSE(fromCharsUint64("+  ").ok);
    }

    SECTION("just 0x prefix rejected") {
        REQUIRE_FALSE(fromCharsUint64("0x").ok);
        REQUIRE_FALSE(fromCharsUint64("0X").ok);
        REQUIRE_FALSE(fromCharsUint64("0x ").ok);
    }

    SECTION("overflow rejected") {
        REQUIRE_FALSE(fromCharsUint64("18446744073709551616").ok);
        REQUIRE_FALSE(fromCharsUint64("99999999999999999999").ok);
        REQUIRE_FALSE(fromCharsUint64("0x1FFFFFFFFFFFFFFFF").ok);
    }

    SECTION("non-numeric rejected") {
        REQUIRE_FALSE(fromCharsUint64("abc").ok);
        REQUIRE_FALSE(fromCharsUint64("12abc").ok);
        REQUIRE_FALSE(fromCharsUint64("abc123").ok);
        REQUIRE_FALSE(fromCharsUint64("12.34").ok);
        REQUIRE_FALSE(fromCharsUint64("1e10").ok);
    }

    SECTION("trailing garbage rejected") {
        REQUIRE_FALSE(fromCharsUint64("42abc").ok);
        REQUIRE_FALSE(fromCharsUint64("0xFFzz").ok);
        REQUIRE_FALSE(fromCharsUint64("0xGG").ok);
    }

    SECTION("leading zeros ok") {
        auto r = fromCharsUint64("007");
        REQUIRE(r.ok);
        REQUIRE(r.value == 7);

        r = fromCharsUint64("0000");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);
    }
}

// ====================================================================
// fromCharsUint32
// ====================================================================

TEST_CASE("fromCharsUint32", "[security][parser][safe]") {

    SECTION("valid decimal") {
        auto r = fromCharsUint32("0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);

        r = fromCharsUint32("42");
        REQUIRE(r.ok);
        REQUIRE(r.value == 42);

        r = fromCharsUint32("4294967295");
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT32_MAX);
    }

    SECTION("valid hex") {
        auto r = fromCharsUint32("0xFF");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0xFF);

        r = fromCharsUint32("0xFFFFFFFF");
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT32_MAX);
    }

    SECTION("UINT32_MAX+1 rejected") {
        REQUIRE_FALSE(fromCharsUint32("4294967296").ok);
        REQUIRE_FALSE(fromCharsUint32("0x100000000").ok);
    }

    SECTION("much larger values rejected") {
        REQUIRE_FALSE(fromCharsUint32("99999999999").ok);
        REQUIRE_FALSE(fromCharsUint32("0xFFFFFFFFFF").ok);
    }

    SECTION("negative rejected") {
        REQUIRE_FALSE(fromCharsUint32("-1").ok);
        REQUIRE_FALSE(fromCharsUint32("-42").ok);
    }

    SECTION("empty and whitespace rejected") {
        REQUIRE_FALSE(fromCharsUint32("").ok);
        REQUIRE_FALSE(fromCharsUint32("   ").ok);
        REQUIRE_FALSE(fromCharsUint32("\t").ok);
    }

    SECTION("non-numeric rejected") {
        REQUIRE_FALSE(fromCharsUint32("abc").ok);
        REQUIRE_FALSE(fromCharsUint32("12abc").ok);
        REQUIRE_FALSE(fromCharsUint32("12.34").ok);
    }

    SECTION("hex prefix edge cases") {
        REQUIRE_FALSE(fromCharsUint32("0x").ok);
        REQUIRE_FALSE(fromCharsUint32("0X").ok);

        auto r = fromCharsUint32("0x0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);
    }

    SECTION("with whitespace and plus") {
        auto r = fromCharsUint32("  +42  ");
        REQUIRE(r.ok);
        REQUIRE(r.value == 42);
    }

    SECTION("leading zeros ok") {
        auto r = fromCharsUint32("0007");
        REQUIRE(r.ok);
        REQUIRE(r.value == 7);
    }
}

// ====================================================================
// fromCharsUint16
// ====================================================================

TEST_CASE("fromCharsUint16", "[security][parser][safe]") {

    SECTION("valid decimal base 10") {
        auto r = fromCharsUint16("0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);

        r = fromCharsUint16("65535");
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT16_MAX);
    }

    SECTION("valid hex base 16") {
        auto r = fromCharsUint16("FF", 16);
        REQUIRE(r.ok);
        REQUIRE(r.value == 0xFF);

        r = fromCharsUint16("FFFF", 16);
        REQUIRE(r.ok);
        REQUIRE(r.value == UINT16_MAX);
    }

    SECTION("overflow rejected") {
        REQUIRE_FALSE(fromCharsUint16("65536").ok);
        REQUIRE_FALSE(fromCharsUint16("FFFFF", 16).ok);
    }

    SECTION("empty rejected") {
        REQUIRE_FALSE(fromCharsUint16("").ok);
    }

    SECTION("default base is 10") {
        auto r = fromCharsUint16("FF");
        REQUIRE_FALSE(r.ok);
    }
}

// ====================================================================
// parseVersionSegment
// ====================================================================

TEST_CASE("parseVersionSegment", "[security][parser][safe]") {

    SECTION("valid") {
        auto r = parseVersionSegment("0");
        REQUIRE(r.ok);
        REQUIRE(r.value == 0);

        r = parseVersionSegment("123");
        REQUIRE(r.ok);
        REQUIRE(r.value == 123);
    }

    SECTION("empty rejected") {
        REQUIRE_FALSE(parseVersionSegment("").ok);
        REQUIRE_FALSE(parseVersionSegment("   ").ok);
    }

    SECTION("non-numeric rejected") {
        REQUIRE_FALSE(parseVersionSegment("abc").ok);
        REQUIRE_FALSE(parseVersionSegment("1a").ok);
        REQUIRE_FALSE(parseVersionSegment("1.2").ok);
    }

    SECTION("negative rejected") {
        REQUIRE_FALSE(parseVersionSegment("-1").ok);
    }

    SECTION("leading zeros ok") {
        auto r = parseVersionSegment("007");
        REQUIRE(r.ok);
        REQUIRE(r.value == 7);
    }
}

// ====================================================================
// trimView
// ====================================================================

TEST_CASE("trimView", "[security][parser][safe]") {

    SECTION("no whitespace") {
        auto r = trimView("hello");
        REQUIRE(r == "hello");
    }

    SECTION("leading whitespace") {
        auto r = trimView("  hello");
        REQUIRE(r == "hello");
    }

    SECTION("trailing whitespace") {
        auto r = trimView("hello  ");
        REQUIRE(r == "hello");
    }

    SECTION("both sides") {
        auto r = trimView("  hello  ");
        REQUIRE(r == "hello");
    }

    SECTION("empty") {
        auto r = trimView("");
        REQUIRE(r.empty());
    }

    SECTION("only whitespace") {
        auto r = trimView("   ");
        REQUIRE(r.empty());
    }

    SECTION("tabs and newlines") {
        auto r = trimView("\t\nhello\r\n");
        REQUIRE(r == "hello");
    }
}

// ====================================================================
// isWhitespace
// ====================================================================

TEST_CASE("isWhitespace", "[security][parser][safe]") {

    REQUIRE(isWhitespace(' '));
    REQUIRE(isWhitespace('\t'));
    REQUIRE(isWhitespace('\n'));
    REQUIRE(isWhitespace('\r'));

    REQUIRE_FALSE(isWhitespace('a'));
    REQUIRE_FALSE(isWhitespace('0'));
    REQUIRE_FALSE(isWhitespace('\0'));
    REQUIRE_FALSE(isWhitespace('.'));
    REQUIRE_FALSE(isWhitespace('-'));
    REQUIRE_FALSE(isWhitespace('+'));
}
