#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/protocols/sahara/SaharaStateMachine.hpp"

TEST_CASE("SaharaStateMachineTest", "[sahara]") {
    SECTION("testInitialState") {
        mbootcore::SaharaStateMachine fsm;
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::WaitingForHello));
    }

    SECTION("testHelloTransition") {
        mbootcore::SaharaStateMachine fsm;

        auto r = fsm.transition(
            static_cast<uint32_t>(mbootcore::SaharaEvent::HelloReceived));
        REQUIRE(r.isOk());
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::Handshake));
    }

    SECTION("testReset") {
        mbootcore::SaharaStateMachine fsm;
        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::HelloReceived));
        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::HelloResponseSent));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::Ready));

        fsm.reset();
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::WaitingForHello));
    }

    SECTION("testInvalidTransition") {
        mbootcore::SaharaStateMachine fsm;
        auto r = fsm.transition(999);
        REQUIRE(r.isError());
    }

    SECTION("testCanTransition") {
        mbootcore::SaharaStateMachine fsm;
        REQUIRE(fsm.canTransition(
            static_cast<uint32_t>(mbootcore::SaharaEvent::HelloReceived)));
        REQUIRE(!fsm.canTransition(
            static_cast<uint32_t>(mbootcore::SaharaEvent::ResetRequested)));
    }

    SECTION("testFullFlow") {
        mbootcore::SaharaStateMachine fsm;

        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::HelloReceived));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::Handshake));

        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::HelloResponseSent));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::Ready));

        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::ReadDataReceived));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::ReadingImage));

        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::DataSent));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::SendingData));

        (void)fsm.transition(0);
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::Ready));

        (void)fsm.transition(static_cast<uint32_t>(mbootcore::SaharaEvent::EndImageReceived));
        REQUIRE(fsm.currentState().id() ==
                 uint32_t(mbootcore::SaharaStateId::WaitingEndImage));
    }
}
