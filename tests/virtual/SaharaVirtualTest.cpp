#include <catch2/catch_test_macros.hpp>

#include "VirtualSaharaDevice.hpp"

namespace {
mbootcore::MockTransport m_transport;
mbootcore::MockLogger m_logger;
}

TEST_CASE("SaharaVirtualTest", "[virtual]") {

SECTION("testScenarioHandshakeV2Success") {
    auto result = mbootcore::SaharaScenario::handshakeV2Success(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(!result.trace.isEmpty());
}

SECTION("testScenarioHandshakeV3Success") {
    auto result = mbootcore::SaharaScenario::handshakeV3Success(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
}

SECTION("testScenarioHandshakeVersionMismatch") {
    auto result = mbootcore::SaharaScenario::handshakeVersionMismatch(m_transport, m_logger);
    REQUIRE(!result.handshakeOk);
    REQUIRE(result.error == mbootcore::ErrorCode::ProtocolMismatch);
}

SECTION("testScenarioUploadSuccess") {
    auto result = mbootcore::SaharaScenario::uploadSuccess(m_transport, m_logger, 8192, 4096);
    REQUIRE(result.handshakeOk);
    REQUIRE(result.uploadOk);
    REQUIRE(result.bytesUploaded == size_t(8192));
}

SECTION("testScenarioUploadNak") {
    auto result = mbootcore::SaharaScenario::uploadNak(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(!result.uploadOk);
    REQUIRE(result.error == mbootcore::ErrorCode::SaharaNakReadDataError);
}

SECTION("testScenarioResetSuccess") {
    auto result = mbootcore::SaharaScenario::resetSuccess(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(result.resetOk);
}

SECTION("testScenarioDeviceDisconnect") {
    auto result = mbootcore::SaharaScenario::deviceDisconnect(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(!result.resetOk);
    REQUIRE(result.error == mbootcore::ErrorCode::TransportDisconnected);
}

SECTION("testScenarioUploadTimeout") {
    auto result = mbootcore::SaharaScenario::uploadTimeout(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(!result.uploadOk);
}

SECTION("testScenarioUploadUnexpectedPacket") {
    auto result = mbootcore::SaharaScenario::uploadUnexpectedPacket(m_transport, m_logger);
    REQUIRE(result.handshakeOk);
    REQUIRE(!result.uploadOk);
}

SECTION("testScenarioHandshakeTimeout") {
    auto result = mbootcore::SaharaScenario::handshakeTimeout(m_transport, m_logger);
    REQUIRE(!result.handshakeOk);
}

}
