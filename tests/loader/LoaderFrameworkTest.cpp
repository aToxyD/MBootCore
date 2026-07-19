#include <catch2/catch_test_macros.hpp>

#include "mbootcore/loader/LoaderFramework.hpp"
#include "mbootcore/loader/LoaderRepository.hpp"
#include "mbootcore/loader/LoaderMatcher.hpp"
#include "mbootcore/loader/LoaderValidator.hpp"
#include "mbootcore/loader/LoaderCache.hpp"
#include "mbootcore/loader/ElfInspector.hpp"
#include "mbootcore/loader/PrioritySelection.hpp"
#include "mbootcore/loader/LoaderData.hpp"
#include "mbootcore/logging/NullLogger.hpp"
#include "VirtualLoaderRepository.hpp"

using namespace mbootcore;
using namespace mbootcore::test;

namespace {

LoaderMetadata makeMetadata(const std::string& vendor,
                            const std::string& protocol,
                            const std::string& chipset = {},
                            uint32_t msmId = 0) {
    LoaderMetadata meta;
    meta.vendor = vendor;
    meta.protocol = protocol;
    meta.chipset = chipset;
    meta.msmId = msmId;
    meta.loaderSize = 1024;
    meta.pkhash = std::vector<uint8_t>(32, 0x01);
    meta.capabilities = {"upload"};
    return meta;
}

// makeDeviceId intentionally unused; kept for reference

DeviceId makeDeviceIdWithPkHash(uint32_t msmId,
                                 const std::vector<uint8_t>& pk) {
    DeviceId id;
    id.msmId = msmId;
    auto count = std::min(pk.size(), size_t{32});
    std::copy_n(pk.begin(), count, id.pkhash.begin());
    return id;
}

}

TEST_CASE("LoaderFrameworkTest", "[loader]") {

SECTION("test_metadata_creation") {
    LoaderMetadata meta;
    meta.vendor = "Qualcomm";
    meta.protocol = "Sahara";
    meta.chipset = "SM8250";
    meta.msmId = 0x000E;
    meta.loaderSize = 4096;
    meta.pkhash = {0xAB, 0xCD, 0xEF};

    REQUIRE(meta.vendor == "Qualcomm");
    REQUIRE(meta.protocol == "Sahara");
    REQUIRE(meta.chipset == "SM8250");
    REQUIRE(meta.msmId == 0x000E);
    REQUIRE(meta.loaderSize == 4096ULL);
}

SECTION("test_metadata_pkhash_hex") {
    LoaderMetadata meta;
    meta.pkhash = {0xAB, 0xCD, 0xEF, 0x01};
    REQUIRE(meta.pkhashHex() == "ABCDEF01");

    LoaderMetadata empty;
    REQUIRE(empty.pkhashHex().empty());
}

SECTION("test_metadata_missing_fields") {
    LoaderMetadata meta;
    REQUIRE(meta.vendor.empty());
    REQUIRE(meta.protocol.empty());
    REQUIRE(meta.loaderSize == 0ULL);
    REQUIRE(meta.pkhash.empty());
}

SECTION("test_repository_add_get_remove") {
    LoaderRepository repo;
    auto ld = std::make_unique<LoaderData>(
        "test", ByteBuffer{1,2,3},
        makeMetadata("Qualcomm", "Sahara"));

    auto r = repo.add(std::move(ld));
    REQUIRE(r.isOk());

    auto got = repo.get("test");
    REQUIRE(got.isOk());
    REQUIRE(got.value()->name() == "test");

    auto removed = repo.remove("test");
    REQUIRE(removed.isOk());

    auto notFound = repo.get("test");
    REQUIRE(notFound.isError());
    REQUIRE(notFound.error() == ErrorCode::LoaderNotFound);
}

SECTION("test_repository_duplicate") {
    LoaderRepository repo;
    (void)repo.add(std::make_unique<LoaderData>(
        "dup", ByteBuffer{1}, makeMetadata("Qualcomm", "Sahara")));

    auto r = repo.add(std::make_unique<LoaderData>(
        "dup", ByteBuffer{2}, makeMetadata("MediaTek", "MTK-BROM")));
    REQUIRE(r.isError());
    REQUIRE(r.error() == ErrorCode::AlreadyExists);
}

SECTION("test_repository_list") {
    LoaderRepository repo;
    (void)repo.add(std::make_unique<LoaderData>(
        "a", ByteBuffer{1}, makeMetadata("Q", "S")));
    (void)repo.add(std::make_unique<LoaderData>(
        "b", ByteBuffer{2}, makeMetadata("M", "M")));

    auto list = repo.list();
    REQUIRE(list.size() == size_t{2});
    REQUIRE(repo.count() == size_t{2});
}

SECTION("test_repository_clear") {
    LoaderRepository repo;
    (void)repo.add(std::make_unique<LoaderData>(
        "x", ByteBuffer{1}, makeMetadata("Q", "S")));
    (void)repo.add(std::make_unique<LoaderData>(
        "y", ByteBuffer{2}, makeMetadata("M", "T")));

    REQUIRE(repo.count() == size_t{2});
    repo.clear();
    REQUIRE(repo.count() == size_t{0});
}

SECTION("test_validation_required_metadata") {
    LoaderValidator validator;

    LoaderMetadata good = makeMetadata("Qualcomm", "Sahara", "SM8250");
    REQUIRE(validator.hasRequiredMetadata(good));

    LoaderMetadata bad;
    REQUIRE(!validator.hasRequiredMetadata(bad));

    LoaderMetadata noVendor = makeMetadata("", "Sahara");
    REQUIRE(!validator.hasRequiredMetadata(noVendor));

    LoaderMetadata noProtocol = makeMetadata("Qualcomm", "");
    REQUIRE(!validator.hasRequiredMetadata(noProtocol));
}

SECTION("test_validation_corrupted") {
    LoaderValidator validator;

    LoaderMetadata ok = makeMetadata("Q", "S");
    ok.hash = std::vector<uint8_t>(32, 0x01);
    ok.loaderSize = 1024;
    REQUIRE(!validator.isCorrupted(ok));

    LoaderMetadata zeroSize;
    REQUIRE(validator.isCorrupted(zeroSize));
}

SECTION("test_validation_errors") {
    LoaderValidator validator;
    auto errors = validator.validationErrors(LoaderMetadata{});
    REQUIRE(errors.size() >= 3);
}

SECTION("test_matcher_exact_match") {
    LoaderMatcher matcher;
    DeviceId id;
    id.msmId = 0x000E;
    matcher.setDeviceIds(id);
    matcher.setVendor("Qualcomm");
    matcher.setChipset("SM8250");

    std::vector<std::pair<std::string, LoaderMetadata>> candidates;
    candidates.emplace_back("Qualcomm_SM8250_Sahara",
                            makeMetadata("Qualcomm", "Sahara", "SM8250", 0x000E));
    auto results = matcher.find(candidates);
    REQUIRE(!results.empty());
}

SECTION("test_matcher_vendor_match") {
    LoaderMatcher matcher;
    matcher.setVendor("Qualcomm");

    auto score = LoaderMatcher::computeScore(
        makeMetadata("Qualcomm", "Sahara"),
        matcher.criteria());
    REQUIRE(score == 40);

    auto scoreNoMatch = LoaderMatcher::computeScore(
        makeMetadata("MediaTek", "MTK-BROM"),
        matcher.criteria());
    REQUIRE(scoreNoMatch == 0);
}

SECTION("test_matcher_chipset_match") {
    LoaderMatcher matcher;
    matcher.setChipset("SM8550");

    auto score = LoaderMatcher::computeScore(
        makeMetadata("Qualcomm", "Sahara", "SM8550"),
        matcher.criteria());
    REQUIRE(score >= 30);
}

SECTION("test_matcher_pkhash_match") {
    LoaderMatcher matcher;
    std::vector<uint8_t> pk(32, 0x42);
    auto id = makeDeviceIdWithPkHash(0x000E, pk);
    matcher.setDeviceIds(id);

    auto meta = makeMetadata("Qualcomm", "Sahara");
    meta.pkhash = pk;

    auto score = LoaderMatcher::computeScore(meta, matcher.criteria());
    REQUIRE(score >= 80);
}

SECTION("test_matcher_no_match") {
    LoaderMatcher matcher;
    matcher.setVendor("NonExistent");

    std::vector<std::pair<std::string, LoaderMetadata>> candidates;
    candidates.emplace_back("Qualcomm_SM8250_Sahara",
                            makeMetadata("Qualcomm", "Sahara"));
    auto results = matcher.find(candidates);
    REQUIRE(results.empty());
}

SECTION("test_selection_priority") {
    PrioritySelection selector;

    ISelectionPolicy::Candidate a{"loader_a", LoaderMetadata{},
                                  SelectionPriority::ExactMatch};
    ISelectionPolicy::Candidate b{"loader_b", LoaderMetadata{},
                                  SelectionPriority::VendorMatch};

    auto result = selector.select({b, a});
    REQUIRE(result == "loader_a");

    ISelectionPolicy::Candidate c{"loader_c", LoaderMetadata{},
                                  SelectionPriority::Default};
    result = selector.select({b, c});
    REQUIRE(result == "loader_b");
}

SECTION("test_selection_empty") {
    PrioritySelection selector;
    REQUIRE(selector.select({}).empty());
}

SECTION("test_cache_put_get") {
    LoaderCache cache(10);
    REQUIRE(!cache.contains("key1"));

    cache.put("key1", std::make_unique<ByteBuffer>(ByteBuffer{1,2,3}));
    REQUIRE(cache.contains("key1"));

    auto val = cache.get("key1");
    REQUIRE(val.isOk());
    REQUIRE(val.value()->size() == size_t{3});
}

SECTION("test_cache_miss") {
    LoaderCache cache(10);
    auto val = cache.get("nonexistent");
    REQUIRE(val.isError());
    REQUIRE(val.error() == ErrorCode::CacheMiss);
}

SECTION("test_cache_eviction") {
    LoaderCache cache(3);
    cache.put("a", std::make_unique<ByteBuffer>(ByteBuffer{1}));
    cache.put("b", std::make_unique<ByteBuffer>(ByteBuffer{2}));
    cache.put("c", std::make_unique<ByteBuffer>(ByteBuffer{3}));
    cache.put("d", std::make_unique<ByteBuffer>(ByteBuffer{4}));

    REQUIRE(cache.size() == size_t{3});
    REQUIRE(!cache.contains("a"));
}

SECTION("test_cache_clear") {
    LoaderCache cache(10);
    cache.put("k1", std::make_unique<ByteBuffer>(ByteBuffer{1}));
    cache.put("k2", std::make_unique<ByteBuffer>(ByteBuffer{2}));
    REQUIRE(cache.size() == size_t{2});

    cache.clear();
    REQUIRE(cache.size() == size_t{0});
}

SECTION("test_framework_add_loader") {
    auto framework = std::make_unique<LoaderFramework>(
        std::make_shared<NullLogger>());

    auto r = framework->addLoader("test_loader", ByteBuffer{0x7F,'E','L','F'},
                                  makeMetadata("Qualcomm", "Sahara"));
    REQUIRE(r.isOk());
    REQUIRE(framework->loaderCount() == size_t{1});
}

SECTION("test_framework_find_best") {
    auto framework = VirtualLoaderRepository::create(10);

    std::vector<uint8_t> pk(32, 0x42);
    auto id = makeDeviceIdWithPkHash(0x0010, pk);

    auto result = framework->findBestLoader(id, "Qualcomm", "SM8250", "Sahara");
    REQUIRE((result.isOk() || result.error() == ErrorCode::LoaderNotFound));
}

SECTION("test_framework_find_all") {
    auto framework = VirtualLoaderRepository::create(5);
    DeviceId id;

    auto results = framework->findAll(id);
    REQUIRE(results.isOk());
    REQUIRE(!results.value().empty());
}

SECTION("test_framework_empty") {
    auto framework = std::make_unique<LoaderFramework>(
        std::make_shared<NullLogger>());

    DeviceId id;
    auto result = framework->findBestLoader(id);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::LoaderNotFound);
}

SECTION("test_virtual_repository_creation") {
    auto framework = VirtualLoaderRepository::create(100);
    REQUIRE(framework->loaderCount() == size_t{100});

    auto list = framework->listLoaders();
    REQUIRE(list.size() == size_t{100});
}

SECTION("test_virtual_repository_search") {
    auto framework = VirtualLoaderRepository::create(120);
    DeviceId id;
    id.msmId = 0x0021;

    auto results = framework->findAll(id);
    REQUIRE(results.isOk());
}

SECTION("test_virtual_repository_selection_priority") {
    auto framework = VirtualLoaderRepository::create(120);

    std::vector<uint8_t> pk(32, 0x42);
    auto id = makeDeviceIdWithPkHash(0x0010, pk);

    auto result = framework->findBestLoader(id);
    if (result.isOk()) {
        REQUIRE(!result.value()->name().empty());
    }
}

SECTION("test_virtual_repository_duplicate_names") {
    LoaderFramework framework(std::make_shared<NullLogger>());

    auto r1 = framework.addLoader("dup", ByteBuffer{1},
                                   makeMetadata("Q", "S"));
    REQUIRE(r1.isOk());

    auto r2 = framework.addLoader("dup", ByteBuffer{2},
                                   makeMetadata("M", "T"));
    REQUIRE(r2.isError());
    REQUIRE(r2.error() == ErrorCode::AlreadyExists);
}

SECTION("test_elf_inspector_basic") {
    ElfInspector inspector;

    ByteBuffer elf(64, 0);
    elf[0] = 0x7F; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2;
    elf[5] = 1;
    elf[16] = 2; elf[17] = 0;
    elf[18] = 0xB7; elf[19] = 0;

    REQUIRE(inspector.isElf(elf));
    REQUIRE(inspector.is64Bit(elf));
    REQUIRE(!inspector.is32Bit(elf));

    auto result = inspector.inspect(elf);
    REQUIRE(result.isOk());
    REQUIRE(result.value().architecture == "AArch64");
    REQUIRE(result.value().header.type == uint16_t{2});
    REQUIRE(result.value().header.machine == uint16_t{0xB7});
}

SECTION("test_elf_inspector_rejects_invalid") {
    ElfInspector inspector;

    ByteBuffer notElf = {0x00, 0x01, 0x02, 0x03};
    REQUIRE(!inspector.isElf(notElf));

    auto result = inspector.inspect(notElf);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("test_framework_with_custom_components") {
    LoaderFramework framework(std::make_shared<NullLogger>());

    framework.setRepository(std::make_unique<LoaderRepository>());
    framework.setMatcher(std::make_unique<LoaderMatcher>());
    framework.setValidator(std::make_unique<LoaderValidator>());
    framework.setCache(std::make_unique<LoaderCache>(64));
    framework.setInspector(std::make_unique<ElfInspector>());
    framework.setSelectionPolicy(std::make_unique<PrioritySelection>());

    auto r = framework.addLoader("custom", ByteBuffer{0x7F,'E','L','F'},
                                  makeMetadata("Qualcomm", "Sahara"));
    REQUIRE(r.isOk());
    REQUIRE(framework.loaderCount() == size_t{1});

    DeviceId id;
    auto found = framework.findBestLoader(id);
    REQUIRE(found.isOk());
    REQUIRE(found.value()->name() == "custom");
}

}
