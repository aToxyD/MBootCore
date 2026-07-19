#include <catch2/catch_test_macros.hpp>
#include <mbootcore/faults/FaultTypes.hpp>
#include <mbootcore/faults/FaultInjector.hpp>
#include <mbootcore/domain/Error.hpp>

#include <memory>
#include <thread>
#include <chrono>

using namespace mbootcore;
using namespace mbootcore::faults;

TEST_CASE("FaultTest", "[faults]") {

    SECTION("testFaultTypeEnum") {
    }
    SECTION("testFaultScopeEnum") {
    }
    SECTION("testFaultStateEnum") {
    }
    SECTION("testFaultConfigDefaults") {
    }
    SECTION("testFaultInjectorConfigure") {
    }
    SECTION("testFaultInjectorShouldInjectOnce") {
    }
    SECTION("testFaultInjectorOnceSecondCallFalse") {
    }
    SECTION("testFaultInjectorInjectReturnsEvent") {
    }
    SECTION("testFaultInjectorReset") {
    }
    SECTION("testFaultManagerRegister") {
    }
    SECTION("testFaultManagerInjectFault") {
    }
    SECTION("testFaultManagerEventsCollection") {
    }
    SECTION("testFaultManagerClearEvents") {
    }
    SECTION("testFaultManagerResetAll") {
    }
    SECTION("testFaultManagerUnregister") {
    }
    SECTION("testFaultScopePersistent") {
    }
    SECTION("testFaultScopeRandom") {
    }
    SECTION("testFaultScopeTimed") {
    }
    SECTION("testFaultInjectionZeroProbability") {
    }
    SECTION("testFaultInjectionDisabled") {
    }
    SECTION("testFaultInjectionMaxTriggersExhausted") {
    }
    SECTION("testFaultInjectorStateTransitions") {
    }
    SECTION("testFaultManagerDuplicateRegistration") {
    }
    SECTION("testFaultManagerUnknownUnregister") {
    }
    SECTION("testFaultInjectorMoveSemantics") {
    }
}

void testFaultTypeEnum() {
    REQUIRE(static_cast<uint32_t>(FaultType::None) == 0u);
    REQUIRE(static_cast<uint32_t>(FaultType::TransportFailure) == 1u);
    REQUIRE(static_cast<uint32_t>(FaultType::MemoryFailure) == 2u);
    REQUIRE(static_cast<uint32_t>(FaultType::Timeout) == 3u);
    REQUIRE(static_cast<uint32_t>(FaultType::CRCError) == 4u);
    REQUIRE(static_cast<uint32_t>(FaultType::GPTCorruption) == 5u);
    REQUIRE(static_cast<uint32_t>(FaultType::DSPCorruption) == 6u);
    REQUIRE(static_cast<uint32_t>(FaultType::PluginFailure) == 7u);
    REQUIRE(static_cast<uint32_t>(FaultType::WorkflowFailure) == 8u);
    REQUIRE(static_cast<uint32_t>(FaultType::LoaderFailure) == 9u);
    REQUIRE(static_cast<uint32_t>(FaultType::USBDisconnect) == 10u);
    REQUIRE(static_cast<uint32_t>(FaultType::SerialDisconnect) == 11u);
    REQUIRE(static_cast<uint32_t>(FaultType::TCPDisconnect) == 12u);
    REQUIRE(static_cast<uint32_t>(FaultType::PartialWrite) == 13u);
    REQUIRE(static_cast<uint32_t>(FaultType::PartialRead) == 14u);
    REQUIRE(static_cast<uint32_t>(FaultType::PowerInterruption) == 15u);
}

void testFaultScopeEnum() {
    REQUIRE(static_cast<uint32_t>(FaultScope::Once) == 0u);
    REQUIRE(static_cast<uint32_t>(FaultScope::Random) == 1u);
    REQUIRE(static_cast<uint32_t>(FaultScope::Persistent) == 2u);
    REQUIRE(static_cast<uint32_t>(FaultScope::Timed) == 3u);
}

void testFaultStateEnum() {
    REQUIRE(static_cast<uint32_t>(FaultState::Inactive) == 0u);
    REQUIRE(static_cast<uint32_t>(FaultState::Active) == 1u);
    REQUIRE(static_cast<uint32_t>(FaultState::Triggered) == 2u);
    REQUIRE(static_cast<uint32_t>(FaultState::Expired) == 3u);
}

void testFaultConfigDefaults() {
    FaultConfig cfg;
    REQUIRE(cfg.type == FaultType::None);
    REQUIRE(cfg.scope == FaultScope::Once);
    REQUIRE(cfg.probability == 100u);
    REQUIRE(cfg.maxTriggers == 1u);
    REQUIRE(cfg.duration.count() == 0LL);
    REQUIRE(cfg.enabled == false);
}

void testFaultInjectorConfigure() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::Timeout;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    auto result = injector.configure(cfg);
    REQUIRE(result.isOk());

    auto retrieved = injector.config();
    REQUIRE(retrieved.isOk());
    REQUIRE(retrieved.value().type == FaultType::Timeout);
    REQUIRE(retrieved.value().enabled == true);
}

void testFaultInjectorShouldInjectOnce() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::USBDisconnect;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(should.value());
}

void testFaultInjectorOnceSecondCallFalse() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::USBDisconnect;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    auto result = injector.inject("test");
    REQUIRE(result.isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

void testFaultInjectorInjectReturnsEvent() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::CRCError;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    auto result = injector.inject("CRC mismatch");
    REQUIRE(result.isOk());
    REQUIRE(result.value().type == FaultType::CRCError);
    REQUIRE(result.value().description == "CRC mismatch");
    REQUIRE(!result.value().handled);
}

void testFaultInjectorReset() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::PowerInterruption;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());
    REQUIRE(injector.inject("test").isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());

    REQUIRE(injector.reset().isOk());

    should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(should.value());
}

void testFaultManagerRegister() {
    FaultManager manager;
    auto injector = std::make_unique<FaultInjector>();

    auto result = manager.registerInjector("timeout_fault", std::move(injector));
    REQUIRE(result.isOk());
}

void testFaultManagerInjectFault() {
    FaultManager manager;
    auto injector = std::make_unique<FaultInjector>();
    FaultConfig cfg;
    cfg.type = FaultType::Timeout;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;
    REQUIRE(injector->configure(cfg).isOk());

    REQUIRE(manager.registerInjector("timeout_fault", std::move(injector)).isOk());

    auto result = manager.injectFault(FaultType::Timeout, "timeout occurred");
    REQUIRE(result.isOk());
}

void testFaultManagerEventsCollection() {
    FaultManager manager;
    auto injector = std::make_unique<FaultInjector>();
    FaultConfig cfg;
    cfg.type = FaultType::USBDisconnect;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;
    REQUIRE(injector->configure(cfg).isOk());

    REQUIRE(manager.registerInjector("usb_fault", std::move(injector)).isOk());
    REQUIRE(manager.injectFault(FaultType::USBDisconnect, "USB disconnected").isOk());

    auto events = manager.events();
    REQUIRE(events.isOk());
    REQUIRE(events.value().size() == 1u);
    REQUIRE(events.value()[0].type == FaultType::USBDisconnect);
}

void testFaultManagerClearEvents() {
    FaultManager manager;
    auto injector = std::make_unique<FaultInjector>();
    FaultConfig cfg;
    cfg.type = FaultType::Timeout;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;
    REQUIRE(injector->configure(cfg).isOk());

    REQUIRE(manager.registerInjector("timeout_fault", std::move(injector)).isOk());
    REQUIRE(manager.injectFault(FaultType::Timeout).isOk());

    auto events = manager.events();
    REQUIRE(events.isOk());
    REQUIRE(events.value().size() == 1u);

    REQUIRE(manager.clearEvents().isOk());

    events = manager.events();
    REQUIRE(events.isOk());
    REQUIRE(events.value().size() == 0u);
}

void testFaultManagerResetAll() {
    FaultManager manager;

    auto injector1 = std::make_unique<FaultInjector>();
    FaultConfig cfg1;
    cfg1.type = FaultType::SerialDisconnect;
    cfg1.scope = FaultScope::Once;
    cfg1.probability = 100;
    cfg1.enabled = true;
    REQUIRE(injector1->configure(cfg1).isOk());
    REQUIRE(manager.registerInjector("serial_fault", std::move(injector1)).isOk());

    auto injector2 = std::make_unique<FaultInjector>();
    FaultConfig cfg2;
    cfg2.type = FaultType::TCPDisconnect;
    cfg2.scope = FaultScope::Once;
    cfg2.probability = 100;
    cfg2.enabled = true;
    REQUIRE(injector2->configure(cfg2).isOk());
    REQUIRE(manager.registerInjector("tcp_fault", std::move(injector2)).isOk());

    REQUIRE(manager.injectFault(FaultType::SerialDisconnect).isOk());
    REQUIRE(manager.injectFault(FaultType::TCPDisconnect).isOk());

    REQUIRE(manager.resetAll().isOk());

    auto events = manager.events();
    REQUIRE(events.isOk());
    REQUIRE(events.value().size() == 0u);
}

void testFaultManagerUnregister() {
    FaultManager manager;
    auto injector = std::make_unique<FaultInjector>();
    FaultConfig cfg;
    cfg.type = FaultType::MemoryFailure;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;
    REQUIRE(injector->configure(cfg).isOk());

    REQUIRE(manager.registerInjector("mem_fault", std::move(injector)).isOk());
    REQUIRE(manager.unregisterInjector("mem_fault").isOk());
}

void testFaultScopePersistent() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::PartialWrite;
    cfg.scope = FaultScope::Persistent;
    cfg.probability = 100;
    cfg.maxTriggers = 5;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    for (uint32_t i = 0; i < 5; ++i) {
        auto should = injector.shouldInject();
        REQUIRE(should.isOk());
        REQUIRE(should.value());
        auto result = injector.inject("persistent fault " + std::to_string(i));
        REQUIRE(result.isOk());
    }

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

void testFaultScopeRandom() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::LoaderFailure;
    cfg.scope = FaultScope::Random;
    cfg.probability = 50;
    cfg.maxTriggers = 10;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    uint32_t injectedCount = 0;
    for (uint32_t i = 0; i < 20; ++i) {
        auto should = injector.shouldInject();
        if (should.isOk() && should.value()) {
            auto result = injector.inject("random fault");
            if (result.isOk()) ++injectedCount;
        }
    }

    REQUIRE(injectedCount > 0);
}

void testFaultScopeTimed() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::DSPCorruption;
    cfg.scope = FaultScope::Timed;
    cfg.probability = 100;
    cfg.duration = std::chrono::milliseconds(50);
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(should.value());

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

void testFaultInjectionZeroProbability() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::GPTCorruption;
    cfg.scope = FaultScope::Once;
    cfg.probability = 0;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

void testFaultInjectionDisabled() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::PluginFailure;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = false;

    REQUIRE(injector.configure(cfg).isOk());

    auto should = injector.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

void testFaultInjectionMaxTriggersExhausted() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::WorkflowFailure;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.maxTriggers = 1;
    cfg.enabled = true;

    REQUIRE(injector.configure(cfg).isOk());
    REQUIRE(injector.inject("first trigger").isOk());

    auto result = injector.inject("second trigger");
    REQUIRE(result.isError());
}

void testFaultInjectorStateTransitions() {
    FaultInjector injector;
    FaultConfig cfg;
    cfg.type = FaultType::TransportFailure;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;

    REQUIRE(injector.state() == FaultState::Inactive);

    REQUIRE(injector.configure(cfg).isOk());
    REQUIRE(injector.state() == FaultState::Active);

    REQUIRE(injector.inject("test").isOk());
    REQUIRE(injector.state() == FaultState::Expired);

    REQUIRE(injector.reset().isOk());
    REQUIRE(injector.state() == FaultState::Active);
}

void testFaultManagerDuplicateRegistration() {
    FaultManager manager;
    auto injector1 = std::make_unique<FaultInjector>();
    auto injector2 = std::make_unique<FaultInjector>();

    REQUIRE(manager.registerInjector("dup", std::move(injector1)).isOk());

    auto result = manager.registerInjector("dup", std::move(injector2));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::AlreadyExists);
}

void testFaultManagerUnknownUnregister() {
    FaultManager manager;
    auto result = manager.unregisterInjector("nonexistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testFaultInjectorMoveSemantics() {
    FaultInjector injector1;
    FaultConfig cfg;
    cfg.type = FaultType::PartialRead;
    cfg.scope = FaultScope::Once;
    cfg.probability = 100;
    cfg.enabled = true;
    REQUIRE(injector1.configure(cfg).isOk());
    REQUIRE(injector1.inject("pre-move").isOk());

    FaultInjector injector2(std::move(injector1));
    auto should = injector2.shouldInject();
    REQUIRE(should.isOk());
    REQUIRE(!should.value());
}

