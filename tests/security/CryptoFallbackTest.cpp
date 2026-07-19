#include <catch2/catch_test_macros.hpp>

#include <mbootcore/firmware/FirmwareValidator.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/firmware/FirmwareTypes.hpp>
#include <mbootcore/firmware/VirtualFirmware.hpp>
#include <mbootcore/logging/NullLogger.hpp>
#include "MockLogger.hpp"

using namespace mbootcore;
using namespace mbootcore::firmware;

TEST_CASE("FirmwareValidator — XOR fallback warning", "[security][crypto]") {

    FirmwareValidator validator;
    MockLogger logger;
    validator.setLogger(&logger);

    VirtualPackageGenerator gen;
    auto pkg = gen.generateGoodPackage();

#ifndef MBOOTCORE_ENABLE_CRYPTO

    // The crypto-disabled warning uses a function-local static atomic flag
    // inside FirmwareValidator::validateIntegrity().  Once emitted, it is
    // suppressed for the remainder of the process.  Catch2 SECTIONs share
    // the same process, so the flag is never reset between sections.
    // All warning-emission assertions are therefore consolidated into a
    // single SECTION to test the once-per-process guarantee correctly.
    SECTION("crypto-disabled warning is emitted exactly once per process") {
        logger.clear();

        auto r1 = validator.validateIntegrity(*pkg);
        REQUIRE(r1.valid);

        auto r2 = validator.validateIntegrity(*pkg);
        REQUIRE(r2.valid);

        auto r3 = validator.validateIntegrity(*pkg);
        REQUIRE(r3.valid);

        // Exactly one warning across all three calls.
        int warningCount = 0;
        for (const auto& rec : logger.records()) {
            if (rec.level == LogLevel::Warning && rec.tag == "FirmwareValidator") {
                ++warningCount;
            }
        }
        REQUIRE(warningCount == 1);

        // Verify the warning text matches the documented behaviour.
        bool found = false;
        for (const auto& rec : logger.records()) {
            if (rec.level == LogLevel::Warning && rec.tag == "FirmwareValidator") {
                REQUIRE(rec.message.find("XOR checksum") != std::string::npos);
                REQUIRE(rec.message.find("NO protection") != std::string::npos);
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    SECTION("subsequent calls still complete successfully") {
        logger.clear();
        auto r1 = validator.validateIntegrity(*pkg);
        REQUIRE(r1.valid);

        auto r2 = validator.validateIntegrity(*pkg);
        REQUIRE(r2.valid);

        auto r3 = validator.validateIntegrity(*pkg);
        REQUIRE(r3.valid);
    }

    SECTION("validate() at Full level completes successfully") {
        logger.clear();
        auto r = validator.validate(*pkg, ValidationLevel::Full);
        REQUIRE(r.valid);
    }

    SECTION("warning absent when logger is not set") {
        FirmwareValidator noLoggerValidator;
        auto r = noLoggerValidator.validateIntegrity(*pkg);
        REQUIRE(r.valid);
    }

#else

    SECTION("no warning emitted when crypto is enabled") {
        logger.clear();
        auto r = validator.validateIntegrity(*pkg);
        REQUIRE(r.valid);

        for (const auto& rec : logger.records()) {
            REQUIRE_FALSE(rec.level == LogLevel::Warning);
        }
    }

    SECTION("validate() at Full level emits no crypto warning") {
        logger.clear();
        auto r = validator.validate(*pkg, ValidationLevel::Full);
        REQUIRE(r.valid);

        for (const auto& rec : logger.records()) {
            if (rec.tag == "FirmwareValidator") {
                REQUIRE(rec.message.find("XOR checksum") == std::string::npos);
            }
        }
    }

#endif
}
