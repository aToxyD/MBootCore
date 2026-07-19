#include <catch2/catch_test_macros.hpp>

#include <mbootcore/domain/Error.hpp>
#include "platform/DynamicLibrary.hpp"

using namespace mbootcore;
using namespace mbootcore::platform;

TEST_CASE("DynamicLibraryTest", "[platform]") {

    SECTION("loadNonExistent_returnsError") {
        auto result = DynamicLibrary::load("/nonexistent/library.so");
        REQUIRE(result.isError());
    }

    SECTION("loadNonExistentPath_returnsError") {
        auto result = DynamicLibrary::load("/tmp/__nonexistent_lib__XXXX__");
        REQUIRE(result.isError());
    }

    SECTION("moveConstructor_works") {
        auto result = DynamicLibrary::load("/tmp/nonexistent.so");
        REQUIRE(result.isError());

        auto result2 = DynamicLibrary::load("/tmp/nonexistent2.so");
        REQUIRE(result2.isError());
    }
}
