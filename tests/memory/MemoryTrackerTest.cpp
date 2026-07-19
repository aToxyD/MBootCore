#include <catch2/catch_test_macros.hpp>
#include <mbootcore/memory/MemoryTracker.hpp>
#include <cstdlib>

using namespace mbootcore::memory;

TEST_CASE("MemoryTrackerTypesTest", "[memory]") {
    SECTION("allocationRecordDefaults") {
        AllocationRecord r;
        REQUIRE(r.address == nullptr);
        REQUIRE(r.size == size_t{0});
        REQUIRE(r.label.empty());
        REQUIRE(r.freed == false);
    }

    SECTION("memoryReportDefaults") {
        MemoryReport r;
        REQUIRE(r.currentUsage == size_t{0});
        REQUIRE(r.peakUsage == size_t{0});
        REQUIRE(r.totalAllocations == uint64_t{0});
        REQUIRE(r.totalDeallocations == uint64_t{0});
        REQUIRE(r.liveAllocations == size_t{0});
        REQUIRE(r.largeBufferThreshold == size_t{1024 * 1024});
        REQUIRE(r.liveObjects.empty());
        REQUIRE(r.largeBuffers.empty());
    }
}

TEST_CASE("MemoryTrackerTest", "[memory]") {
    SECTION("trackAllocation") {
        MemoryTracker mt;
        int data{};
        mt.trackAllocation(&data, sizeof(data), "int_data");

        REQUIRE(mt.currentUsage() == sizeof(int));
        REQUIRE(mt.totalAllocations() == uint64_t{1});
        REQUIRE(mt.liveAllocationCount() == size_t{1});
    }

    SECTION("trackDeallocation") {
        MemoryTracker mt;
        int data{};
        mt.trackAllocation(&data, sizeof(data), "int_data");
        mt.trackDeallocation(&data);

        REQUIRE(mt.currentUsage() == size_t{0});
        REQUIRE(mt.totalAllocations() == uint64_t{1});
        REQUIRE(mt.totalDeallocations() == uint64_t{1});
        REQUIRE(mt.liveAllocationCount() == size_t{0});
    }

    SECTION("peakUsageIsTracked") {
        MemoryTracker mt;
        int a{}, b{};
        mt.trackAllocation(&a, 100, "a");
        mt.trackAllocation(&b, 200, "b");
        REQUIRE(mt.peakUsage() == size_t{300});

        mt.trackDeallocation(&a);
        REQUIRE(mt.currentUsage() == size_t{200});
        REQUIRE(mt.peakUsage() == size_t{300});
    }

    SECTION("totalAllocationsCount") {
        MemoryTracker mt;
        int a{}, b{}, c{};
        mt.trackAllocation(&a, 1);
        mt.trackAllocation(&b, 2);
        mt.trackAllocation(&c, 3);
        REQUIRE(mt.totalAllocations() == uint64_t{3});
    }

    SECTION("totalDeallocationsCount") {
        MemoryTracker mt;
        int a{}, b{};
        mt.trackAllocation(&a, 1);
        mt.trackAllocation(&b, 2);
        mt.trackDeallocation(&a);
        mt.trackDeallocation(&b);
        REQUIRE(mt.totalDeallocations() == uint64_t{2});
    }

    SECTION("liveAllocationCount") {
        MemoryTracker mt;
        int a{}, b{}, c{};
        mt.trackAllocation(&a, 1);
        mt.trackAllocation(&b, 2);
        mt.trackAllocation(&c, 3);
        mt.trackDeallocation(&b);
        REQUIRE(mt.liveAllocationCount() == size_t{2});
    }

    SECTION("largeBufferDetection") {
        MemoryTracker mt;
        mt.setLargeBufferThreshold(10);

        std::vector<char> small(5);
        std::vector<char> large(20);

        mt.trackAllocation(small.data(), small.size(), "small");
        mt.trackAllocation(large.data(), large.size(), "large");

        const auto rep = mt.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().largeBuffers.size() == size_t{1});
        REQUIRE(rep.value().largeBuffers[0].label == std::string("large"));
    }

    SECTION("resetClearsState") {
        MemoryTracker mt;
        int data{};
        mt.trackAllocation(&data, 100);
        mt.reset();

        REQUIRE(mt.currentUsage() == size_t{0});
        REQUIRE(mt.peakUsage() == size_t{0});
        REQUIRE(mt.totalAllocations() == uint64_t{0});
        REQUIRE(mt.totalDeallocations() == uint64_t{0});
        REQUIRE(mt.liveAllocationCount() == size_t{0});
    }

    SECTION("nullAllocationIgnored") {
        MemoryTracker mt;
        mt.trackAllocation(nullptr, 100);
        REQUIRE(mt.totalAllocations() == uint64_t{0});
    }

    SECTION("nullDeallocationIgnored") {
        MemoryTracker mt;
        mt.trackDeallocation(nullptr);
        REQUIRE(mt.totalDeallocations() == uint64_t{0});
    }

    SECTION("unknownDeallocationIgnored") {
        MemoryTracker mt;
        int data{};
        mt.trackDeallocation(&data);
        REQUIRE(mt.totalDeallocations() == uint64_t{0});
    }

    SECTION("reportContainsLiveObjects") {
        MemoryTracker mt;
        int a{}, b{};
        mt.trackAllocation(&a, 10, "obj_a");
        mt.trackAllocation(&b, 20, "obj_b");

        const auto rep = mt.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().liveObjects.size() == size_t{2});
        REQUIRE(rep.value().liveAllocations == size_t{2});
    }

    SECTION("snapshotIsConsistent") {
        MemoryTracker mt;
        int data{};
        mt.trackAllocation(&data, 64, "snap");
        const auto snap = mt.snapshot();
        REQUIRE(snap.isOk());
        REQUIRE(snap.value().currentUsage == size_t{64});
        REQUIRE(snap.value().totalAllocations == uint64_t{1});
    }

    SECTION("exportReportFormat") {
        MemoryTracker mt;
        int data{};
        mt.trackAllocation(&data, 256, "export_test");

        const auto exp = mt.exportReport();
        REQUIRE(exp.isOk());
        const auto& s = exp.value();
        REQUIRE(s.find("Memory Report") != std::string::npos);
        REQUIRE(s.find("Current Usage:") != std::string::npos);
        REQUIRE(s.find("export_test") != std::string::npos);
        REQUIRE(s.find("256") != std::string::npos);
    }

    SECTION("largeBufferThresholdSetter") {
        MemoryTracker mt;
        mt.setLargeBufferThreshold(4096);
        const auto rep = mt.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().largeBufferThreshold == size_t{4096});
    }

    SECTION("multipleAllocationsDeallocations") {
        MemoryTracker mt;
        for (size_t i = 0; i < 10; ++i) {
            auto* p = static_cast<char*>(std::malloc(16));
            uintptr_t addr = reinterpret_cast<uintptr_t>(p);
            mt.trackAllocation(p, 16, "loop");
            mt.trackDeallocation(reinterpret_cast<void*>(addr));
            std::free(p);
        }
        REQUIRE(mt.totalAllocations() == uint64_t{10});
        REQUIRE(mt.totalDeallocations() == uint64_t{10});
        REQUIRE(mt.currentUsage() == size_t{0});
    }
}

TEST_CASE("ScopedMemoryTrackTest", "[memory]") {
    SECTION("scopedTrackAllocatesAndFrees") {
        MemoryTracker mt;
        {
            ScopedMemoryTrack st(mt, 64, "scoped_test");
            REQUIRE(st.address() != nullptr);
            REQUIRE(mt.currentUsage() == size_t{64});
            REQUIRE(mt.liveAllocationCount() == size_t{1});
        }
        REQUIRE(mt.currentUsage() == size_t{0});
        REQUIRE(mt.liveAllocationCount() == size_t{0});
    }

    SECTION("scopedTrackLabel") {
        MemoryTracker mt;
        {
            ScopedMemoryTrack st(mt, 128, "labeled");
            const auto rep = mt.report();
            REQUIRE(rep.isOk());
            REQUIRE(rep.value().liveObjects.size() == size_t{1});
            REQUIRE(rep.value().liveObjects[0].label == std::string("labeled"));
        }
    }

    SECTION("addressReturned") {
        MemoryTracker mt;
        ScopedMemoryTrack st(mt, 32, "addr");
        REQUIRE(st.address() != nullptr);
        const auto rep = mt.report();
        REQUIRE(rep.isOk());
        REQUIRE(rep.value().liveObjects[0].address == st.address());
    }
}
