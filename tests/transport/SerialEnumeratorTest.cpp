#include <catch2/catch_test_macros.hpp>
#include <mbootcore/transport/SerialEnumerator.hpp>

namespace mt = mbootcore::transport;

TEST_CASE("SerialEnumeratorTest", "[serial][enumerator]") {

    SECTION("enumerate_returnsVector") {
        auto ports = mt::SerialEnumerator::enumerate();
        REQUIRE(ports.size() <= 100);
    }

    SECTION("portCount_matchesEnumerateSize") {
        auto ports = mt::SerialEnumerator::enumerate();
        size_t count = mt::SerialEnumerator::portCount();
        REQUIRE(count == ports.size());
    }

    SECTION("findFirst_returnsValidOrEmpty") {
        auto first = mt::SerialEnumerator::findFirst();
        auto ports = mt::SerialEnumerator::enumerate();
        if (!ports.empty()) {
            REQUIRE(first.portName == ports.front().portName);
        } else {
            REQUIRE(first.portName.empty());
        }
    }

    SECTION("findByDescription_returnsSubset") {
        auto ports = mt::SerialEnumerator::enumerate();
        if (!ports.empty()) {
            const auto& firstDesc = ports.front().description;
            if (!firstDesc.empty()) {
                auto found = mt::SerialEnumerator::findByDescription(firstDesc.substr(0, 4));
                REQUIRE(!found.empty());
            }
        }
    }

    SECTION("findFirst_afterEnumerate_consistent") {
        auto first = mt::SerialEnumerator::findFirst();
        auto ports = mt::SerialEnumerator::enumerate();
        if (!ports.empty()) {
            REQUIRE(first.portName == ports.front().portName);
        }
    }

    SECTION("portCount_nonNegative") {
        size_t count = mt::SerialEnumerator::portCount();
        REQUIRE(count <= 100);
        auto first = mt::SerialEnumerator::enumerate();
        auto second = mt::SerialEnumerator::enumerate();
        REQUIRE(first.size() == second.size());
        for (size_t i = 0; i < first.size() && i < second.size(); ++i) {
            REQUIRE(first[i].portName == second[i].portName);
        }
    }
}
