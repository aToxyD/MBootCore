#pragma once

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <sstream>
#include <memory>

namespace mbootcore {
namespace testing {

struct GoldenVectorResult {
    std::string name;
    bool passed{false};
    std::string errorMessage;
    std::chrono::microseconds serializeTime{0};
    std::chrono::microseconds parseTime{0};
    std::chrono::microseconds totalTime{0};
    size_t inputBytes{0};
    size_t outputBytes{0};
};

struct GoldenVectorSummary {
    size_t totalVectors{0};
    size_t passedVectors{0};
    size_t failedVectors{0};
    std::vector<GoldenVectorResult> results;
    std::chrono::microseconds totalTime{0};
};

template <typename PacketType>
struct GoldenVector {
    std::string name;
    PacketType input;
    std::vector<uint8_t> expectedBytes;
    bool verifyBytes{false};
    std::string description;
};

template <typename SerializerType, typename ParserType, typename PacketType>
class GoldenVectorSuite {
public:
    using SerializeFunc = std::function<std::vector<uint8_t>(SerializerType&, const PacketType&)>;
    using ParseFunc = std::function<PacketType(ParserType&, const std::vector<uint8_t>&)>;
    using ValidateFunc = std::function<void(const PacketType&, const PacketType&)>;

    GoldenVectorSuite(std::string suiteName, SerializeFunc serializeFn, ParseFunc parseFn)
        : m_name(std::move(suiteName))
        , m_serialize(std::move(serializeFn))
        , m_parse(std::move(parseFn))
    {}

    void addVector(GoldenVector<PacketType> vec) {
        m_vectors.push_back(std::move(vec));
    }

    void setValidator(ValidateFunc fn) {
        m_validate = std::move(fn);
    }

    void addVectors(std::vector<GoldenVector<PacketType>> vectors) {
        for (auto& v : vectors) {
            m_vectors.push_back(std::move(v));
        }
    }

    GoldenVectorSummary runAll() const {
        GoldenVectorSummary summary;
        summary.totalVectors = m_vectors.size();
        summary.results.reserve(m_vectors.size());

        for (const auto& vec : m_vectors) {
            summary.results.push_back(runVector(vec));
        }

        summary.passedVectors = 0;
        summary.failedVectors = 0;
        summary.totalTime = std::chrono::microseconds::zero();

        for (const auto& r : summary.results) {
            if (r.passed) ++summary.passedVectors;
            else ++summary.failedVectors;
            summary.totalTime += r.totalTime;
        }

        return summary;
    }

    void runAndAssert() const {
        auto summary = runAll();
        for (const auto& r : summary.results) {
            INFO("Vector: " << r.name);
            REQUIRE(r.passed);
        }
    }

    void report(const GoldenVectorSummary& summary) const {
        std::ostringstream oss;
        oss << "\n=== Golden Vector Suite: " << m_name << " ===\n";
        oss << "Total: " << summary.totalVectors
            << " | Passed: " << summary.passedVectors
            << " | Failed: " << summary.failedVectors << "\n";
        oss << "Total time: " << summary.totalTime.count() << " us\n";
        for (const auto& r : summary.results) {
            oss << "  " << (r.passed ? "PASS" : "FAIL")
                << " " << r.name
                << " (" << r.totalTime.count() << " us"
                << ", " << r.inputBytes << " bytes in"
                << ", " << r.outputBytes << " bytes out";
            if (!r.errorMessage.empty()) {
                oss << ", " << r.errorMessage;
            }
            oss << ")\n";
        }
        UNSCOPED_INFO(oss.str());
    }

    const std::string& name() const { return m_name; }
    size_t vectorCount() const { return m_vectors.size(); }

private:
    GoldenVectorResult runVector(const GoldenVector<PacketType>& vec) const {
        GoldenVectorResult result;
        result.name = vec.name;
        result.inputBytes = vec.expectedBytes.size();

        SerializerType serializer;
        ParserType parser;

        try {
            auto serStart = std::chrono::steady_clock::now();
            auto bytes = m_serialize(serializer, vec.input);
            auto serEnd = std::chrono::steady_clock::now();
            result.serializeTime = std::chrono::duration_cast<std::chrono::microseconds>(serEnd - serStart);
            result.outputBytes = bytes.size();

            if (vec.verifyBytes && bytes != vec.expectedBytes) {
                result.errorMessage = "byte mismatch";
                return result;
            }

            auto parseStart = std::chrono::steady_clock::now();
            auto parsed = m_parse(parser, bytes);
            auto parseEnd = std::chrono::steady_clock::now();
            result.parseTime = std::chrono::duration_cast<std::chrono::microseconds>(parseEnd - parseStart);

            if (m_validate) {
                m_validate(vec.input, parsed);
            }

            result.totalTime = result.serializeTime + result.parseTime;
            result.passed = true;
        } catch (const std::exception& e) {
            result.errorMessage = e.what();
        }

        return result;
    }

    std::string m_name;
    SerializeFunc m_serialize;
    ParseFunc m_parse;
    ValidateFunc m_validate;
    std::vector<GoldenVector<PacketType>> m_vectors;
};

} // namespace testing
} // namespace mbootcore
