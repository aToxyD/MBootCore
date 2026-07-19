#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include "mbootcore/domain/Types.hpp"
#include "mbootcore/domain/Error.hpp"
#include "mbootcore/core/protocols/sahara/SaharaProtocol.hpp"

using namespace mbootcore;

TEST_CASE("TypesTest", "[domain]") {
    SECTION("testProtocolVersion") {
        ProtocolVersion v{2, 1};
        REQUIRE(v.major == 2u);
        REQUIRE(v.minor == 1u);
        REQUIRE(v == ProtocolVersion{2, 1});
        REQUIRE(v != ProtocolVersion{2, 2});
        REQUIRE(v < ProtocolVersion{2, 2});
        REQUIRE(!(ProtocolVersion{3, 0} < v));
    }

    SECTION("testSaharaNegotiatedVersion") {
        SaharaNegotiatedVersion v{2, 2};
        REQUIRE(v.isCompatible(2));
        REQUIRE(!v.isCompatible(1));
        REQUIRE(!v.isCompatible(3));
    }

    SECTION("testResultOk") {
        auto r = Result<int>(42);
        REQUIRE(r.isOk());
        REQUIRE(r.value() == 42);
    }

    SECTION("testResultError") {
        auto r = Result<int>(ErrorCode::TransportTimeout);
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::TransportTimeout);
    }

    SECTION("testResultVoid") {
        auto r = Result<void>{};
        REQUIRE(r.isOk());
    }

    SECTION("testErrorToString") {
        auto s = toString(ErrorCode::Success);
        REQUIRE(s[0] != '\0');
    }

    SECTION("testResultBoolConversion") {
        auto ok = Result<int>(10);
        REQUIRE(ok);
        auto err = Result<int>(ErrorCode::Unknown);
        REQUIRE(!err);
    }

    SECTION("testResultMoveSemantics") {
        Result<std::string> r1(std::string("hello"));
        REQUIRE(r1.isOk());
        REQUIRE(r1.value() == "hello");

        Result<std::string> r2(std::move(r1));
        REQUIRE(r2.isOk());
        REQUIRE(r2.value() == "hello");

        auto r3 = Result<std::string>(ErrorCode::Unknown);
        Result<std::string> r4(std::move(r3));
        REQUIRE(r4.isError());
        REQUIRE(r4.error() == ErrorCode::Unknown);
    }

    SECTION("testResultMoveAssignment") {
        Result<std::string> r1(std::string("hello"));
        Result<std::string> r2(std::string("world"));
        r2 = std::move(r1);
        REQUIRE(r2.isOk());
        REQUIRE(r2.value() == "hello");
    }

    SECTION("testResultMap") {
        auto r = Result<int>(42);
        auto mapped = r.map([](int x) { return x * 2; });
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value() == 84);
    }

    SECTION("testResultMapError") {
        auto r = Result<int>(ErrorCode::TransportTimeout);
        auto mapped = r.map([](int x) { return x * 2; });
        REQUIRE(mapped.isError());
    }

    SECTION("testResultAndThen") {
        auto r = Result<int>(5);
        auto chained = r.and_then([](int x) -> Result<int> {
            return Result<int>(x + 3);
        });
        REQUIRE(chained.isOk());
        REQUIRE(chained.value() == 8);
    }

    SECTION("testResultAndThenError") {
        auto r = Result<int>(ErrorCode::Unknown);
        auto chained = r.and_then([](int x) -> Result<int> {
            return Result<int>(x + 3);
        });
        REQUIRE(chained.isError());
    }

    SECTION("testResultOrElse") {
        bool handlerCalled = false;
        auto r = Result<int>(ErrorCode::TransportTimeout);
        auto result = r.or_else([&](const ErrorInfo&) {
            handlerCalled = true;
        });
        REQUIRE(handlerCalled);
        REQUIRE(result.isError());
        REQUIRE(result.error() == ErrorCode::TransportTimeout);
    }

    SECTION("testResultOrElseNotCalledOnOk") {
        bool handlerCalled = false;
        auto r = Result<int>(42);
        auto result = r.or_else([&](const ErrorInfo&) {
            handlerCalled = true;
        });
        REQUIRE(!handlerCalled);
        REQUIRE(result.isOk());
        REQUIRE(result.value() == 42);
    }

    SECTION("testResultValueOr") {
        auto ok = Result<int>(42);
        REQUIRE(ok.value_or(0) == 42);

        auto err = Result<int>(ErrorCode::Unknown);
        REQUIRE(err.value_or(99) == 99);
    }

    SECTION("testResultTakeValue") {
        auto r = Result<std::string>(std::string("movable"));
        auto v = std::move(r).takeValue();
        REQUIRE(v == "movable");
    }

    SECTION("testResultMatchOk") {
        auto r = Result<int>(42);
        auto result = r.match(
            [](int val) -> std::string { return "ok:" + std::to_string(val); },
            [](const ErrorInfo&) -> std::string { return "err"; }
        );
        REQUIRE(result == "ok:42");
    }

    SECTION("testResultMatchError") {
        auto r = Result<int>(ErrorCode::Unknown);
        auto result = r.match(
            [](int) -> std::string { return "ok"; },
            [](const ErrorInfo& e) -> std::string {
                return std::string("err:") + toString(e.code);
            }
        );
        REQUIRE(result.find("err:") == 0);
    }

    SECTION("testResultWithErrorInfo") {
        ErrorInfo info;
        info.code = ErrorCode::ProtocolError;
        info.context = "custom context";
        info.source = "test.cpp:42";
        auto r = Result<int>(info);
        REQUIRE(r.isError());
        REQUIRE(r.error() == ErrorCode::ProtocolError);
        REQUIRE(r.errorInfo().context == "custom context");
        REQUIRE(r.errorInfo().source == "test.cpp:42");
    }

    SECTION("testResultVoidMap") {
        auto r = Result<void>{};
        auto mapped = r.map([]() -> int { return 7; });
        REQUIRE(mapped.isOk());
        REQUIRE(mapped.value() == 7);
    }

    SECTION("testResultVoidAndThen") {
        auto r = Result<void>{};
        auto chained = r.and_then([]() -> Result<int> {
            return Result<int>(3);
        });
        REQUIRE(chained.isOk());
        REQUIRE(chained.value() == 3);
    }

    SECTION("testResultVoidMatchOk") {
        auto r = Result<void>{};
        auto result = r.match(
            []() -> std::string { return "ok"; },
            [](const ErrorInfo&) -> std::string { return "err"; }
        );
        REQUIRE(result == "ok");
    }

    SECTION("testResultStaticFactories") {
        auto ok = Result<int>::Ok(42);
        REQUIRE(ok.isOk());
        REQUIRE(ok.value() == 42);

        auto err = Result<int>::Error(ErrorCode::Unknown);
        REQUIRE(err.isError());
        REQUIRE(err.error() == ErrorCode::Unknown);
    }

    SECTION("testResultNonCopyableType") {
        auto r = Result<std::unique_ptr<int>>(std::make_unique<int>(42));
        REQUIRE(r.isOk());
        REQUIRE(*r.value() == 42);
        auto v = std::move(r).takeValue();
        REQUIRE(*v == 42);
    }
}
