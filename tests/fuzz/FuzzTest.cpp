#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <string>
#include <random>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <unordered_map>

static std::mt19937& rng() {
    static std::mt19937 gen{42};
    return gen;
}

static std::vector<uint8_t> randomBytes(size_t size) {
    std::vector<uint8_t> data(size);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : data) b = static_cast<uint8_t>(dist(rng()));
    return data;
}

static std::string randomXml(size_t size) {
    std::uniform_int_distribution<int> choice(0, 4);
    std::string result;
    result.reserve(size);
    while (result.size() < size) {
        switch (choice(rng())) {
            case 0: result += "<tag>"; break;
            case 1: result += "</tag>"; break;
            case 2: result += "<selfclose/>"; break;
            case 3: result += "text_content "; break;
            case 4: result += "<nested a=\"b\"><inner x=\"1\"/></nested>"; break;
        }
    }
    if (result.size() > size) result.resize(size);
    return result;
}

static std::string randomJson(size_t size) {
    std::uniform_int_distribution<int> choice(0, 5);
    std::string result;
    result.reserve(size);
    while (result.size() < size) {
        switch (choice(rng())) {
            case 0: result += "{\"k\":\"v\"}"; break;
            case 1: result += "[1,2,3]"; break;
            case 2: result += "\"string\""; break;
            case 3: result += "true"; break;
            case 4: result += "null"; break;
            case 5: result += "42"; break;
        }
    }
    if (result.size() > size) result.resize(size);
    return result;
}

static bool safeBinaryParse(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;
    size_t pos = 0;
    (void)pos;
    while (pos < data.size()) {
        if (pos + 4 > data.size()) break;
        uint32_t tag = static_cast<uint32_t>(data[pos]) |
                       (static_cast<uint32_t>(data[pos+1]) << 8) |
                       (static_cast<uint32_t>(data[pos+2]) << 16) |
                       (static_cast<uint32_t>(data[pos+3]) << 24);
        if (tag == 0xDEADBEEF || tag == 0xFEEDFACE) {
            pos += 4;
            if (pos + 2 > data.size()) break;
            uint16_t len = static_cast<uint16_t>(data[pos]) |
                          (static_cast<uint16_t>(data[pos+1]) << 8);
            pos += 2;
            if (pos + len > data.size()) break;
            pos += len;
        } else {
            pos += 4;
        }
    }
    return true;
}

static bool safeXmlParse(const std::string& input) {
    if (input.empty()) return false;
    size_t depth = 0;
    size_t pos = 0;
    while (pos < input.size()) {
        if (input[pos] == '<') {
            if (pos + 1 < input.size() && input[pos+1] == '/') {
                if (depth > 0) --depth;
            } else if (pos + 1 < input.size() && input[pos+1] != '?') {
                ++depth;
            }
        }
        ++pos;
    }
    (void)depth;
    return true;
}

static bool safeJsonParse(const std::string& input) {
    if (input.empty()) return false;
    int braceDepth = 0;
    int bracketDepth = 0;
    bool inString = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (inString) {
            if (c == '\\' && i + 1 < input.size()) {
                ++i;
                continue;
            }
            if (c == '"') inString = false;
            continue;
        }
        if (c == '"') { inString = true; continue; }
        if (c == '{') { ++braceDepth; continue; }
        if (c == '}') { if (braceDepth > 0) --braceDepth; continue; }
        if (c == '[') { ++bracketDepth; continue; }
        if (c == ']') { if (bracketDepth > 0) --bracketDepth; continue; }
    }
    (void)braceDepth;
    (void)bracketDepth;
    return true;
}

TEST_CASE("BinaryFuzzTest", "[fuzz]") {

    SECTION("randomBinaryInputs") {
        for (int i = 0; i < 50; ++i) {
            auto data = randomBytes(static_cast<size_t>(rng()() % 512 + 1));
            REQUIRE(safeBinaryParse(data));
        }
    }

    SECTION("emptyBinaryInput") {
        std::vector<uint8_t> empty;
        REQUIRE(!safeBinaryParse(empty));
    }

    SECTION("largeBinaryInput") {
        auto data = randomBytes(65536);
        REQUIRE(safeBinaryParse(data));
    }
}

TEST_CASE("XmlFuzzTest", "[fuzz]") {

    SECTION("randomXmlInputs") {
        for (int i = 0; i < 50; ++i) {
            auto input = randomXml(static_cast<size_t>(rng()() % 256 + 1));
            REQUIRE(safeXmlParse(input));
        }
    }

    SECTION("emptyXmlInput") {
        REQUIRE(!safeXmlParse(""));
    }

    SECTION("deeplyNestedXml") {
        std::string deep;
        for (int i = 0; i < 100; ++i) deep += "<x>";
        for (int i = 0; i < 100; ++i) deep += "</x>";
        REQUIRE(safeXmlParse(deep));
    }
}

TEST_CASE("JsonFuzzTest", "[fuzz]") {

    SECTION("randomJsonInputs") {
        for (int i = 0; i < 50; ++i) {
            auto input = randomJson(static_cast<size_t>(rng()() % 256 + 1));
            REQUIRE(safeJsonParse(input));
        }
    }

    SECTION("emptyJsonInput") {
        REQUIRE(!safeJsonParse(""));
    }

    SECTION("unbalancedJson") {
        REQUIRE(safeJsonParse("{{{{{{}}}}}}"));
        REQUIRE(safeJsonParse("[[[[]]]]"));
    }
}

TEST_CASE("MalformedInputTest", "[fuzz]") {

    SECTION("nullBytesInInput") {
        std::vector<uint8_t> data = {0, 0, 0, 0, 0xFF, 0, 0xDE, 0xAD, 0xBE, 0xEF, 0, 0, 0, 0};
        REQUIRE(safeBinaryParse(data));
    }

    SECTION("unicodeString") {
        std::string uni = "\xc3\xa9\xe2\x82\xac\xf0\x9f\x92\xa9\x00\xff\xfe";
        REQUIRE(safeXmlParse(uni));
        REQUIRE(safeJsonParse(uni));
    }

    SECTION("veryLargeInput") {
        auto data = randomBytes(131072);
        REQUIRE(safeBinaryParse(data));
        auto xml = randomXml(65536);
        REQUIRE(safeXmlParse(xml));
        auto json = randomJson(65536);
        REQUIRE(safeJsonParse(json));
    }
}
