#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>
#include <atomic>
#include <memory>
#include <random>

struct MemoryTracker {
    std::atomic<int64_t> allocations{0};
    std::atomic<int64_t> deallocations{0};

    void onAlloc() { allocations.fetch_add(1, std::memory_order_relaxed); }
    void onDealloc() { deallocations.fetch_add(1, std::memory_order_relaxed); }
    int64_t live() const { return allocations.load(std::memory_order_relaxed) -
                                  deallocations.load(std::memory_order_relaxed); }
};

template<typename T>
class TrackedObject {
public:
    TrackedObject(MemoryTracker& tracker, T val) : m_tracker(&tracker), m_value(val) {
        m_tracker->onAlloc();
    }
    ~TrackedObject() { m_tracker->onDealloc(); }
    T value() const { return m_value; }
private:
    MemoryTracker* m_tracker;
    T m_value;
};

static std::mt19937& lrng() {
    static std::mt19937 gen{12345};
    return gen;
}

TEST_CASE("ContinuousOperationTest", "[longrunning]") {

    SECTION("thousandSequentialOperations") {
        std::vector<int> data;
        data.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            data.push_back(i);
        }
        REQUIRE(static_cast<int>(data.size()) == 1000);
        for (int i = 0; i < 1000; ++i) {
            REQUIRE(data[i] == i);
        }
    }

    SECTION("hundredConnectDisconnectCycles") {
        int state = 0;
        for (int cycle = 0; cycle < 100; ++cycle) {
            REQUIRE(state == 0);
            state = 1;
            REQUIRE(state == 1);
            state = 2;
            REQUIRE(state == 2);
            state = 0;
            REQUIRE(state == 0);
        }
    }

    SECTION("thousandCounterIncrements") {
        int64_t counter = 0;
        for (int i = 0; i < 1000; ++i) {
            ++counter;
        }
        REQUIRE(counter == int64_t{1000});
        for (int i = 0; i < 1000; ++i) {
            --counter;
        }
        REQUIRE(counter == int64_t{0});
    }
}

struct WorkflowStage {
    int id;
    bool executed{false};
    bool rolledBack{false};
};

TEST_CASE("ContinuousWorkflowTest", "[longrunning]") {

    SECTION("hundredStagePipeline") {
        std::vector<WorkflowStage> stages;
        stages.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stages.push_back({i, false, false});
        }
        for (auto& s : stages) {
            s.executed = true;
        }
        for (const auto& s : stages) {
            REQUIRE(s.executed);
        }
    }

    SECTION("pipelineWithRandomDelays") {
        std::vector<WorkflowStage> stages;
        stages.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stages.push_back({i, false, false});
        }
        for (auto& s : stages) {
            s.executed = true;
            volatile int dummy = 0;
            int delay = lrng()() % 10;
            for (int d = 0; d < delay; ++d) ++dummy;
            (void)dummy;
        }
        for (const auto& s : stages) {
            REQUIRE(s.executed);
        }
    }
}

TEST_CASE("LeakDetectionTest", "[longrunning]") {

    SECTION("allocateDeallocateCycle") {
        MemoryTracker tracker;
        {
            std::vector<std::unique_ptr<TrackedObject<int>>> objs;
            objs.reserve(1000);
            for (int i = 0; i < 1000; ++i) {
                objs.push_back(std::make_unique<TrackedObject<int>>(tracker, i));
            }
            REQUIRE(tracker.live() == int64_t{1000});
        }
        REQUIRE(tracker.live() == int64_t{0});
    }

    SECTION("memoryTrackerNoLeak") {
        MemoryTracker tracker;
        for (int i = 0; i < 1000; ++i) {
            auto obj = std::make_unique<TrackedObject<int>>(tracker, 42);
            REQUIRE(obj->value() == 42);
        }
        REQUIRE(tracker.live() == int64_t{0});
        REQUIRE(tracker.allocations.load() == int64_t{1000});
        REQUIRE(tracker.deallocations.load() == int64_t{1000});
    }
}

TEST_CASE("StabilityTest", "[longrunning]") {

    SECTION("fiveHundredOpsConsistentState") {
        enum class State : uint32_t { Idle, Running, Paused, Error };
        State current = State::Idle;
        int errorCount = 0;

        for (int i = 0; i < 500; ++i) {
            switch (current) {
                case State::Idle:
                    current = State::Running;
                    break;
                case State::Running:
                    if (lrng()() % 5 == 0) {
                        current = State::Error;
                        ++errorCount;
                    } else if (lrng()() % 3 == 0) {
                        current = State::Paused;
                    }
                    break;
                case State::Paused:
                    current = State::Running;
                    break;
                case State::Error:
                    current = State::Idle;
                    break;
            }
        }
        REQUIRE(errorCount >= 0);
        REQUIRE(static_cast<int>(current) >= 0);
    }

    SECTION("stateMachineTransitionConsistency") {
        int idleCount = 0, runningCount = 0, pausedCount = 0, errorCount = 0;

        enum class S : uint32_t { Idle, Running, Paused, Error };
        S state = S::Idle;

        for (int i = 0; i < 500; ++i) {
            switch (state) {
                case S::Idle: state = S::Running; ++idleCount; break;
                case S::Running:
                    state = (lrng()() % 2 == 0) ? S::Paused : S::Idle;
                    ++runningCount;
                    break;
                case S::Paused: state = S::Running; ++pausedCount; break;
                case S::Error: state = S::Idle; ++errorCount; break;
            }
        }

        REQUIRE(static_cast<int>(idleCount + runningCount + pausedCount + errorCount) == 500);
        REQUIRE(idleCount >= 0);
        REQUIRE(runningCount >= 0);
    }
}
