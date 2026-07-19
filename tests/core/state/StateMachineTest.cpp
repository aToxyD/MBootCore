#include <catch2/catch_test_macros.hpp>
#include "mbootcore/core/state/StateMachine.hpp"

TEST_CASE("StateMachineTest", "[state]") {
    SECTION("testGenericStateMachine") {
        mbootcore::GenericStateMachine fsm;
        fsm.addState(0, "Idle");
        fsm.addState(1, "Active");
        fsm.addState(2, "Error");
        fsm.setInitialState(0);

        fsm.addTransition(0, 1, 1, "Idle → Active");
        fsm.addTransition(1, 2, 2, "Active → Error");

        REQUIRE(fsm.currentState().id() == uint32_t(0));

        auto r1 = fsm.transition(1);
        REQUIRE(r1.isOk());
        REQUIRE(fsm.currentState().id() == uint32_t(1));

        auto r2 = fsm.transition(2);
        REQUIRE(r2.isOk());
        REQUIRE(fsm.currentState().id() == uint32_t(2));
    }

    SECTION("testInvalidTransition") {
        mbootcore::GenericStateMachine fsm;
        fsm.addState(0, "Idle");
        fsm.addState(1, "Active");
        fsm.setInitialState(0);
        fsm.addTransition(0, 1, 1, "Idle → Active");

        auto r = fsm.transition(999);
        REQUIRE(r.isError());
        REQUIRE(static_cast<uint32_t>(r.error()) == static_cast<uint32_t>(mbootcore::ErrorCode::InvalidState));
    }

    SECTION("testCanTransition") {
        mbootcore::GenericStateMachine fsm;
        fsm.addState(0, "Idle");
        fsm.addState(1, "Active");
        fsm.setInitialState(0);
        fsm.addTransition(0, 1, 1, "Idle → Active");

        REQUIRE(fsm.canTransition(1));
        REQUIRE(!fsm.canTransition(999));
    }

    SECTION("testReset") {
        mbootcore::GenericStateMachine fsm;
        fsm.addState(0, "Idle");
        fsm.addState(1, "Active");
        fsm.setInitialState(0);
        fsm.addTransition(0, 1, 1, "Idle → Active");

        (void)fsm.transition(1);
        REQUIRE(fsm.currentState().id() == uint32_t(1));

        fsm.reset();
        REQUIRE(fsm.currentState().id() == uint32_t(0));
    }
}
