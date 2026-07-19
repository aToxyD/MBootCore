#include <catch2/catch_test_macros.hpp>
#include <mbootcore/vendor/VendorTypes.hpp>
#include <mbootcore/vendor/VendorEvents.hpp>
#include <mbootcore/vendor/VendorContext.hpp>
#include <mbootcore/vendor/IVendor.hpp>
#include <mbootcore/vendor/IVendorPlugin.hpp>
#include <mbootcore/vendor/VendorRegistry.hpp>
#include <mbootcore/vendor/VendorFactory.hpp>
#include <mbootcore/vendor/CapabilityResolver.hpp>
#include <mbootcore/vendor/VendorRuntime.hpp>
#include <mbootcore/vendor/VendorSession.hpp>
#include <mbootcore/vendor/VendorPipeline.hpp>
#include <mbootcore/vendor/VendorMonitor.hpp>
#include <mbootcore/vendor/VendorPackageResolver.hpp>

#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/Types.hpp>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/pipeline/BootPipeline.hpp>
#include <mbootcore/generic/IFlashDevice.hpp>
#include <mbootcore/workflow/WorkflowEngine.hpp>
#include <mbootcore/firmware/FirmwarePackage.hpp>
#include <mbootcore/session/SessionTypes.hpp>

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

using namespace mbootcore;
using namespace mbootcore::vendor;

using namespace mbootcore::discovery;

namespace {

// ============================================================
// Mock Vendor implementation
// ============================================================

class MockVendor : public IVendor {
public:
    VendorInfo info;
    VendorCapability caps{VendorCapability::None};
    bool initialized{false};
    bool m_shutdown{false};
    std::string vendorName;

    MockVendor(const std::string& id, const std::string& name, VendorFamily family,
               VendorCapability cap = VendorCapability::BootROM | VendorCapability::MemoryRead,
               const std::vector<std::string>& protocols = {"Sahara"})
        : vendorName(name)
    {
        info.id = id;
        info.name = name;
        info.family = family;
        info.capabilities = cap;
        info.supportedProtocols = protocols;
        caps = cap;
    }

    Result<void> initialize(const VendorContext& context) override {
        if (initialized) return ErrorCode::InvalidState;
        initialized = true;
        return {};
    }

    Result<void> shutdown() noexcept override {
        if (!initialized && m_shutdown) return {};
        initialized = false;
        m_shutdown = true;
        return {};
    }

    VendorInfo vendorInfo() const override { return info; }
    VendorCapability capabilities() const override { return caps; }

    std::unique_ptr<discovery::IDeviceDetector> createDetector() override { return nullptr; }
    std::unique_ptr<discovery::IProtocolNegotiator> createNegotiator() override { return nullptr; }
    std::unique_ptr<pipeline::BootPipeline> createPipeline() override { return nullptr; }
    std::unique_ptr<IFlashDevice> createFlashDevice() override { return nullptr; }
    std::unique_ptr<workflow::WorkflowEngine> createWorkflow() override { return nullptr; }
    std::string_view name() const noexcept override { return vendorName; }
    std::unique_ptr<IVendor> clone() const override {
        return std::make_unique<MockVendor>(info.id, info.name, info.family, caps, info.supportedProtocols);
    }
};

// ============================================================
// Mock Vendor Plugin
// ============================================================

class MockVendorPlugin : public IVendorPlugin {
public:
    VendorInfo info;
    VendorCapability caps{VendorCapability::None};
    bool m_registered{false};
    std::string pluginName;

    MockVendorPlugin(const std::string& id, const std::string& name, VendorFamily family)
        : pluginName(name)
    {
        info.id = id;
        info.name = name;
        info.family = family;
        info.capabilities = VendorCapability::BootROM;
    }

    Result<void> registerVendor(const VendorContext& context) override {
        if (m_registered) return ErrorCode::AlreadyExists;
        m_registered = true;
        return {};
    }

    Result<void> unregisterVendor() noexcept override {
        if (!m_registered) return {};
        m_registered = false;
        return {};
    }

    VendorInfo vendorInfo() const override { return info; }
    VendorCapability capabilities() const override { return caps; }
    std::string_view name() const noexcept override { return pluginName; }
};

// ============================================================
// Helper: create a mock vendor
// ============================================================

std::unique_ptr<MockVendor> makeQualcommVendor() {
    return std::make_unique<MockVendor>(
        "qualcomm", "Qualcomm Technologies", VendorFamily::Qualcomm,
        VendorCapability::BootROM | VendorCapability::DownloadAgent |
        VendorCapability::LoaderUpload | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash |
        VendorCapability::GPT | VendorCapability::Partition |
        VendorCapability::Reset | VendorCapability::Reboot |
        VendorCapability::Verify,
        std::vector<std::string>{"Sahara", "Firehose"}
    );
}

std::unique_ptr<MockVendor> makeMediaTekVendor() {
    return std::make_unique<MockVendor>(
        "mediatek", "MediaTek Inc.", VendorFamily::MediaTek,
        VendorCapability::BootROM | VendorCapability::DownloadAgent |
        VendorCapability::LoaderUpload | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash |
        VendorCapability::GPT,
        std::vector<std::string>{"MediaTekBROM", "MediaTekDA"}
    );
}

std::unique_ptr<MockVendor> makeUnisocVendor() {
    return std::make_unique<MockVendor>(
        "unisoc", "UNISOC Communications", VendorFamily::UNISOC,
        VendorCapability::BootROM | VendorCapability::DownloadAgent |
        VendorCapability::LoaderUpload | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash |
        VendorCapability::Partition,
        std::vector<std::string>{"UNISOCBootROM", "UNISOCFDL"}
    );
}

std::unique_ptr<MockVendor> makeRockchipVendor() {
    return std::make_unique<MockVendor>(
        "rockchip", "Rockchip Electronics", VendorFamily::Rockchip,
        VendorCapability::BootROM | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash |
        VendorCapability::Reset,
        std::vector<std::string>{"USBStream"}
    );
}

std::unique_ptr<MockVendor> makeSamsungVendor() {
    return std::make_unique<MockVendor>(
        "samsung", "Samsung Electronics", VendorFamily::Samsung,
        VendorCapability::BootROM | VendorCapability::DownloadAgent |
        VendorCapability::LoaderUpload | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash |
        VendorCapability::SecureBoot | VendorCapability::Authentication |
        VendorCapability::GPT | VendorCapability::Partition,
        std::vector<std::string>{"USBStream", "Fastboot"}
    );
}

std::unique_ptr<MockVendor> makeAmlogicVendor() {
    return std::make_unique<MockVendor>(
        "amlogic", "Amlogic", VendorFamily::Amlogic,
        VendorCapability::BootROM | VendorCapability::MemoryRead |
        VendorCapability::MemoryWrite | VendorCapability::Flash,
        std::vector<std::string>{"USBStream"}
    );
}

// ============================================================
// Helper: create a registered registry with mock vendors
// ============================================================

void populateRegistry(VendorRegistry& reg) {
    reg.registerVendor(makeQualcommVendor());
    reg.registerVendor(makeMediaTekVendor());
    reg.registerVendor(makeUnisocVendor());
    reg.registerVendor(makeRockchipVendor());
    reg.registerVendor(makeSamsungVendor());
}

} // anonymous namespace

// ============================================================
// Test Class
// ============================================================


void testVendorFamilyEnumValues();
void testVendorCapabilityBitmaskOperations();
void testVendorInfoConstruction();
void testVendorStatisticsDefaults();
void testVendorEventCreation();
void testVendorEventTypeValues();
void testHasCapabilityHelper();
void testCapabilityOperatorOrAnd();
void testVendorInfoComparison();
void testVendorStatisticsUpdate();
void testRegisterVendor();
void testRegisterDuplicateVendorFails();
void testUnregisterVendor();
void testUnregisterNonexistentVendorFails();
void testResolveById();
void testResolveByFamily();
void testResolveByName();
void testResolveByCapability();
void testResolveByProtocol();
void testAllVendorslist();
void testVendorCount();
void testIsRegistered();
void testRegisterPlugin();
void testUnregisterPlugin();
void testMultipleVendorRegistration();
void testCreateVendorByFamily();
void testCreateVendorById();
void testCreatePlugin();
void testCreateFromDescriptor();
void testCreateFromDiscovery();
void testCreateFromSession();
void testFactoryUsesRegistryOnly();
void testNullVendorFallback();
void testFactoryMultipleCalls();
void testFactoryThreadSafety();
void testCanResolveCapability();
void testResolveCapabilitiesForVendor();
void testResolveVendorsByCapability();
void testCapabilitiesForUnknownVendorNone();
void testOptionalCapabilitiesEmpty();
void testFallbackCheck();
void testMultipleVendorsDifferentCapabilities();
void testSameCapabilityOnMultipleVendors();
void testAllCapabilitiesEnumerated();
void testCapabilityBitmaskCorrectness();
void testInitializeRuntime();
void testShutdownRuntime();
void testDoubleInitializeFails();
void testLoadVendor();
void testUnloadVendor();
void testActiveVendorTracking();
void testLoadPlugin();
void testUnloadPlugin();
void testCreateRuntimeContext();
void testDoubleShutdownOk();
void testOpenSession();
void testCloseSession();
void testReconnectSession();
void testResetSession();
void testOpenTwiceFails();
void testSessionStatistics();
void testSessionCapabilities();
void testSessionActiveVendorId();
void testSessionIsOpenState();
void testSessionMoveCopyRestrictions();
void testExecutePipeline();
void testCancelPipeline();
void testIsRunningState();
void testDoubleExecuteSequential();
void testProgressCallback();
void testCurrentVendorId();
void testPipelineWithWorkflow();
void testCancelNonRunning();
void testRecordEvent();
void testRecordSession();
void testRecordBoot();
void testRecordUpload();
void testRecordFlash();
void testRecordFailure();
void testMonitorStatisticsRetrieval();
void testRecentEvents();
void testClearVendor();
void testClearAll();
void testEventCallback();
void testTotalSessionsFailures();
void testResolvePackage();
void testResolveLoader();
void testResolveGptLayout();
void testResolvePartitionLayout();
void testResolveFlashingSequence();
void testPackageWithPath();
void testPackageNotFound();
void testPackageValidation();
void testVirtualQualcommVendorCreation();
void testVirtualMediaTekVendorCreation();
void testVirtualUnisocVendorCreation();
void testVirtualRockchipVendorCreation();
void testVirtualSamsungVendorCreation();
void testVirtualVendorCapabilitiesCorrect();
void testVirtualVendorFamilyCorrect();
void testVirtualVendorInfoFilled();
void testVirtualVendorPluginCreate();
void testVirtualVendorPluginRegister();
void testFrameworkRegisterAll();
void testFrameworkUnregisterAll();
void testFrameworkRegisterSingle();
void testResolveVirtualVendorByFamily();
void testCreateVirtualVendorDetector();
void testFullRegistrationResolutionCycle();
void testRuntimeSessionLifecycle();
void testMonitorEventsIntegration();
void testPipelineCancellationWithContext();
void testCapabilityRegistryIntegration();
void testVirtualVendorThroughRegistry();
void testSessionStatisticsAfterOperations();
void testMultipleConcurrentVendors();
void testVendorPluginRuntime();
void testEventCallbackFiring();
void testRuntimeContextSetup();
void testPipelineProgressCallback();
void testPackageResolveThenFlashSequence();
void testEndToEndVirtualVendorWorkflow();
void testMonitorRecordsCorrectly();
void testEmptyVendorId();
void testUnknownFamily();
void testNullVendorPointer();
void testEmptyRegistry();
void testZeroCapabilities();
void testMismatchedCase();
void testVeryLongVendorNames();
void testSpecialCharactersInIds();
void testMaximumVendors();
void testRapidRegisterUnregister();
void testStressRegistryCycles();
void testStressConcurrentVirtualVendors();
void testStressSessionOpenClose();
void testStressMultipleMonitors();
void testStressRegistryThreadSafety();
void testStressPipeline();
void testStressFactory();
void testStressMonitor();
void testStressCapabilityResolve();
void testStressMemoryAllocation();

TEST_CASE("VendorFrameworkTest", "[vendor]") {


    // --- 1. Vendor Types Tests (~10) ---
    SECTION("testVendorFamilyEnumValues") {
        testVendorFamilyEnumValues();}
    SECTION("testVendorCapabilityBitmaskOperations") {
        testVendorCapabilityBitmaskOperations();}
    SECTION("testVendorInfoConstruction") {
        testVendorInfoConstruction();}
    SECTION("testVendorStatisticsDefaults") {
        testVendorStatisticsDefaults();}
    SECTION("testVendorEventCreation") {
        testVendorEventCreation();}
    SECTION("testVendorEventTypeValues") {
        testVendorEventTypeValues();}
    SECTION("testHasCapabilityHelper") {
        testHasCapabilityHelper();}
    SECTION("testCapabilityOperatorOrAnd") {
        testCapabilityOperatorOrAnd();}
    SECTION("testVendorInfoComparison") {
        testVendorInfoComparison();}
    SECTION("testVendorStatisticsUpdate") {
        testVendorStatisticsUpdate();
    }

    // --- 2. Vendor Registry Tests (~15) ---
    SECTION("testRegisterVendor") {
        testRegisterVendor();}
    SECTION("testRegisterDuplicateVendorFails") {
        testRegisterDuplicateVendorFails();}
    SECTION("testUnregisterVendor") {
        testUnregisterVendor();}
    SECTION("testUnregisterNonexistentVendorFails") {
        testUnregisterNonexistentVendorFails();}
    SECTION("testResolveById") {
        testResolveById();}
    SECTION("testResolveByFamily") {
        testResolveByFamily();}
    SECTION("testResolveByName") {
        testResolveByName();}
    SECTION("testResolveByCapability") {
        testResolveByCapability();}
    SECTION("testResolveByProtocol") {
        testResolveByProtocol();}
    SECTION("testAllVendorslist") {
        testAllVendorslist();}
    SECTION("testVendorCount") {
        testVendorCount();}
    SECTION("testIsRegistered") {
        testIsRegistered();}
    SECTION("testRegisterPlugin") {
        testRegisterPlugin();}
    SECTION("testUnregisterPlugin") {
        testUnregisterPlugin();}
    SECTION("testMultipleVendorRegistration") {
        testMultipleVendorRegistration();
    }

    // --- 3. Vendor Factory Tests (~10) ---
    SECTION("testCreateVendorByFamily") {
        testCreateVendorByFamily();}
    SECTION("testCreateVendorById") {
        testCreateVendorById();}
    SECTION("testCreatePlugin") {
        testCreatePlugin();}
    SECTION("testCreateFromDescriptor") {
        testCreateFromDescriptor();}
    SECTION("testCreateFromDiscovery") {
        testCreateFromDiscovery();}
    SECTION("testCreateFromSession") {
        testCreateFromSession();}
    SECTION("testFactoryUsesRegistryOnly") {
        testFactoryUsesRegistryOnly();}
    SECTION("testNullVendorFallback") {
        testNullVendorFallback();}
    SECTION("testFactoryMultipleCalls") {
        testFactoryMultipleCalls();}
    SECTION("testFactoryThreadSafety") {
        testFactoryThreadSafety();
    }

    // --- 4. Capability Resolver Tests (~10) ---
    SECTION("testCanResolveCapability") {
        testCanResolveCapability();}
    SECTION("testResolveCapabilitiesForVendor") {
        testResolveCapabilitiesForVendor();}
    SECTION("testResolveVendorsByCapability") {
        testResolveVendorsByCapability();}
    SECTION("testCapabilitiesForUnknownVendorNone") {
        testCapabilitiesForUnknownVendorNone();}
    SECTION("testOptionalCapabilitiesEmpty") {
        testOptionalCapabilitiesEmpty();}
    SECTION("testFallbackCheck") {
        testFallbackCheck();}
    SECTION("testMultipleVendorsDifferentCapabilities") {
        testMultipleVendorsDifferentCapabilities();}
    SECTION("testSameCapabilityOnMultipleVendors") {
        testSameCapabilityOnMultipleVendors();}
    SECTION("testAllCapabilitiesEnumerated") {
        testAllCapabilitiesEnumerated();}
    SECTION("testCapabilityBitmaskCorrectness") {
        testCapabilityBitmaskCorrectness();
    }

    // --- 5. Vendor Runtime Tests (~10) ---
    SECTION("testInitializeRuntime") {
        testInitializeRuntime();}
    SECTION("testShutdownRuntime") {
        testShutdownRuntime();}
    SECTION("testDoubleInitializeFails") {
        testDoubleInitializeFails();}
    SECTION("testLoadVendor") {
        testLoadVendor();}
    SECTION("testUnloadVendor") {
        testUnloadVendor();}
    SECTION("testActiveVendorTracking") {
        testActiveVendorTracking();}
    SECTION("testLoadPlugin") {
        testLoadPlugin();}
    SECTION("testUnloadPlugin") {
        testUnloadPlugin();}
    SECTION("testCreateRuntimeContext") {
        testCreateRuntimeContext();}
    SECTION("testDoubleShutdownOk") {
        testDoubleShutdownOk();
    }

    // --- 6. Vendor Session Tests (~10) ---
    SECTION("testOpenSession") {
        testOpenSession();}
    SECTION("testCloseSession") {
        testCloseSession();}
    SECTION("testReconnectSession") {
        testReconnectSession();}
    SECTION("testResetSession") {
        testResetSession();}
    SECTION("testOpenTwiceFails") {
        testOpenTwiceFails();}
    SECTION("testSessionStatistics") {
        testSessionStatistics();}
    SECTION("testSessionCapabilities") {
        testSessionCapabilities();}
    SECTION("testSessionActiveVendorId") {
        testSessionActiveVendorId();}
    SECTION("testSessionIsOpenState") {
        testSessionIsOpenState();}
    SECTION("testSessionMoveCopyRestrictions") {
        testSessionMoveCopyRestrictions();
    }

    // --- 7. Vendor Pipeline Tests (~8) ---
    SECTION("testExecutePipeline") {
        testExecutePipeline();}
    SECTION("testCancelPipeline") {
        testCancelPipeline();}
    SECTION("testIsRunningState") {
        testIsRunningState();}
    SECTION("testDoubleExecuteSequential") {
        testDoubleExecuteSequential();}
    SECTION("testProgressCallback") {
        testProgressCallback();}
    SECTION("testCurrentVendorId") {
        testCurrentVendorId();}
    SECTION("testPipelineWithWorkflow") {
        testPipelineWithWorkflow();}
    SECTION("testCancelNonRunning") {
        testCancelNonRunning();
    }

    // --- 8. Vendor Monitor Tests (~10) ---
    SECTION("testRecordEvent") {
        testRecordEvent();}
    SECTION("testRecordSession") {
        testRecordSession();}
    SECTION("testRecordBoot") {
        testRecordBoot();}
    SECTION("testRecordUpload") {
        testRecordUpload();}
    SECTION("testRecordFlash") {
        testRecordFlash();}
    SECTION("testRecordFailure") {
        testRecordFailure();}
    SECTION("testMonitorStatisticsRetrieval") {
        testMonitorStatisticsRetrieval();}
    SECTION("testRecentEvents") {
        testRecentEvents();}
    SECTION("testClearVendor") {
        testClearVendor();}
    SECTION("testClearAll") {
        testClearAll();}
    SECTION("testEventCallback") {
        testEventCallback();}
    SECTION("testTotalSessionsFailures") {
        testTotalSessionsFailures();
    }

    // --- 9. Vendor Package Resolver Tests (~8) ---
    SECTION("testResolvePackage") {
        testResolvePackage();}
    SECTION("testResolveLoader") {
        testResolveLoader();}
    SECTION("testResolveGptLayout") {
        testResolveGptLayout();}
    SECTION("testResolvePartitionLayout") {
        testResolvePartitionLayout();}
    SECTION("testResolveFlashingSequence") {
        testResolveFlashingSequence();}
    SECTION("testPackageWithPath") {
        testPackageWithPath();}
    SECTION("testPackageNotFound") {
        testPackageNotFound();}
    SECTION("testPackageValidation") {
        testPackageValidation();
    }

    // --- 10. Virtual Vendor Tests (~15) ---
    SECTION("testVirtualQualcommVendorCreation") {
        testVirtualQualcommVendorCreation();}
    SECTION("testVirtualMediaTekVendorCreation") {
        testVirtualMediaTekVendorCreation();}
    SECTION("testVirtualUnisocVendorCreation") {
        testVirtualUnisocVendorCreation();}
    SECTION("testVirtualRockchipVendorCreation") {
        testVirtualRockchipVendorCreation();}
    SECTION("testVirtualSamsungVendorCreation") {
        testVirtualSamsungVendorCreation();}
    SECTION("testVirtualVendorCapabilitiesCorrect") {
        testVirtualVendorCapabilitiesCorrect();}
    SECTION("testVirtualVendorFamilyCorrect") {
        testVirtualVendorFamilyCorrect();}
    SECTION("testVirtualVendorInfoFilled") {
        testVirtualVendorInfoFilled();}
    SECTION("testVirtualVendorPluginCreate") {
        testVirtualVendorPluginCreate();}
    SECTION("testVirtualVendorPluginRegister") {
        testVirtualVendorPluginRegister();}
    SECTION("testFrameworkRegisterAll") {
        testFrameworkRegisterAll();}
    SECTION("testFrameworkUnregisterAll") {
        testFrameworkUnregisterAll();}
    SECTION("testFrameworkRegisterSingle") {
        testFrameworkRegisterSingle();}
    SECTION("testResolveVirtualVendorByFamily") {
        testResolveVirtualVendorByFamily();}
    SECTION("testCreateVirtualVendorDetector") {
        testCreateVirtualVendorDetector();
    }

    // --- 11. Integration Tests (~15) ---
    SECTION("testFullRegistrationResolutionCycle") {
        testFullRegistrationResolutionCycle();}
    SECTION("testRuntimeSessionLifecycle") {
        testRuntimeSessionLifecycle();}
    SECTION("testMonitorEventsIntegration") {
        testMonitorEventsIntegration();}
    SECTION("testPipelineCancellationWithContext") {
        testPipelineCancellationWithContext();}
    SECTION("testCapabilityRegistryIntegration") {
        testCapabilityRegistryIntegration();}
    SECTION("testVirtualVendorThroughRegistry") {
        testVirtualVendorThroughRegistry();}
    SECTION("testSessionStatisticsAfterOperations") {
        testSessionStatisticsAfterOperations();}
    SECTION("testMultipleConcurrentVendors") {
        testMultipleConcurrentVendors();}
    SECTION("testVendorPluginRuntime") {
        testVendorPluginRuntime();}
    SECTION("testEventCallbackFiring") {
        testEventCallbackFiring();}
    SECTION("testRuntimeContextSetup") {
        testRuntimeContextSetup();}
    SECTION("testPipelineProgressCallback") {
        testPipelineProgressCallback();}
    SECTION("testPackageResolveThenFlashSequence") {
        testPackageResolveThenFlashSequence();}
    SECTION("testEndToEndVirtualVendorWorkflow") {
        testEndToEndVirtualVendorWorkflow();}
    SECTION("testMonitorRecordsCorrectly") {
        testMonitorRecordsCorrectly();
    }

    // --- 12. Edge Case Tests (~10) ---
    SECTION("testEmptyVendorId") {
        testEmptyVendorId();}
    SECTION("testUnknownFamily") {
        testUnknownFamily();}
    SECTION("testNullVendorPointer") {
        testNullVendorPointer();}
    SECTION("testEmptyRegistry") {
        testEmptyRegistry();}
    SECTION("testZeroCapabilities") {
        testZeroCapabilities();}
    SECTION("testMismatchedCase") {
        testMismatchedCase();}
    SECTION("testVeryLongVendorNames") {
        testVeryLongVendorNames();}
    SECTION("testSpecialCharactersInIds") {
        testSpecialCharactersInIds();}
    SECTION("testMaximumVendors") {
        testMaximumVendors();}
    SECTION("testRapidRegisterUnregister") {
        testRapidRegisterUnregister();
    }

    // --- 13. Stress Tests (~10) ---
    SECTION("testStressRegistryCycles") {
        testStressRegistryCycles();}
    SECTION("testStressConcurrentVirtualVendors") {
        testStressConcurrentVirtualVendors();}
    SECTION("testStressSessionOpenClose") {
        testStressSessionOpenClose();}
    SECTION("testStressMultipleMonitors") {
        testStressMultipleMonitors();}
    SECTION("testStressRegistryThreadSafety") {
        testStressRegistryThreadSafety();}
    SECTION("testStressPipeline") {
        testStressPipeline();}
    SECTION("testStressFactory") {
        testStressFactory();}
    SECTION("testStressMonitor") {
        testStressMonitor();}
    SECTION("testStressCapabilityResolve") {
        testStressCapabilityResolve();}
    SECTION("testStressMemoryAllocation") {
        testStressMemoryAllocation();}
}

// ============================================================
// 1. Vendor Types Tests
// ============================================================

void testVendorFamilyEnumValues() {
    REQUIRE(static_cast<uint32_t>(VendorFamily::Unknown) == 0u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::Qualcomm) == 1u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::MediaTek) == 2u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::UNISOC) == 3u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::Rockchip) == 4u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::Samsung) == 5u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::Amlogic) == 6u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::NXP) == 7u);
    REQUIRE(static_cast<uint32_t>(VendorFamily::Custom) == 0xFFu);
}

void testVendorCapabilityBitmaskOperations() {
    uint32_t boot = static_cast<uint32_t>(VendorCapability::BootROM);
    uint32_t mem = static_cast<uint32_t>(VendorCapability::MemoryRead);
    uint32_t flash = static_cast<uint32_t>(VendorCapability::Flash);
    REQUIRE(boot == 1u << 0);
    REQUIRE(mem == 1u << 3);
    REQUIRE(flash == 1u << 5);
    REQUIRE(static_cast<uint32_t>(VendorCapability::Logging) == 1u << 14);
}

void testVendorInfoConstruction() {
    VendorInfo info;
    REQUIRE(info.id == std::string());
    REQUIRE(info.name == std::string());
    REQUIRE(info.family == VendorFamily::Unknown);
    REQUIRE(info.capabilities == VendorCapability::None);
    REQUIRE(info.supportedProtocols.empty());
}

void testVendorStatisticsDefaults() {
    VendorStatistics stats;
    REQUIRE(stats.devicesDetected == uint64_t(0));
    REQUIRE(stats.successfulSessions == uint64_t(0));
    REQUIRE(stats.failedSessions == uint64_t(0));
    REQUIRE(stats.uploads == uint64_t(0));
    REQUIRE(stats.flashes == uint64_t(0));
    REQUIRE(stats.averageBootTime.count() == int64_t(0));
}

void testVendorEventCreation() {
    auto now = std::chrono::steady_clock::now();
    VendorEvent ev;
    ev.type = VendorEventType::VendorLoaded;
    ev.vendorId = "test";
    ev.message = "loaded";
    ev.success = true;
    ev.timestamp = now;
    REQUIRE(ev.type == VendorEventType::VendorLoaded);
    REQUIRE(ev.vendorId == std::string("test"));
    REQUIRE(ev.message == std::string("loaded"));
    REQUIRE(ev.success);
    REQUIRE(ev.timestamp == now);
}

void testVendorEventTypeValues() {
    REQUIRE(static_cast<uint32_t>(VendorEventType::VendorLoaded) == 0u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::VendorUnloaded) == 1u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::SessionOpened) == 2u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::SessionClosed) == 3u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::CapabilityChanged) == 4u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::PluginLoaded) == 5u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::PluginFailed) == 6u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::DiscoveryCompleted) == 7u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::WorkflowStarted) == 8u);
    REQUIRE(static_cast<uint32_t>(VendorEventType::WorkflowFinished) == 9u);
}

void testHasCapabilityHelper() {
    auto caps = VendorCapability::BootROM | VendorCapability::MemoryRead | VendorCapability::Flash;
    REQUIRE(hasCapability(caps, VendorCapability::BootROM));
    REQUIRE(hasCapability(caps, VendorCapability::MemoryRead));
    REQUIRE(hasCapability(caps, VendorCapability::Flash));
    REQUIRE(!hasCapability(caps, VendorCapability::MemoryWrite));
    REQUIRE(!hasCapability(caps, VendorCapability::GPT));
    REQUIRE(!hasCapability(caps, VendorCapability::None));
}

void testCapabilityOperatorOrAnd() {
    auto combined = VendorCapability::BootROM | VendorCapability::Flash;
    REQUIRE(hasCapability(combined, VendorCapability::BootROM));
    REQUIRE(hasCapability(combined, VendorCapability::Flash));
    REQUIRE(!hasCapability(combined, VendorCapability::MemoryRead));
    auto masked = combined & VendorCapability::BootROM;
    REQUIRE(hasCapability(masked, VendorCapability::BootROM));
    REQUIRE(!hasCapability(masked, VendorCapability::Flash));
}

void testVendorInfoComparison() {
    VendorInfo a{"qcom", "Qualcomm", "1.0", "author", "desc", VendorFamily::Qualcomm, VendorMaturity::Production, {"Sahara"}, VendorCapability::BootROM};
    VendorInfo b{"qcom", "Qualcomm", "1.0", "author", "desc", VendorFamily::Qualcomm, VendorMaturity::Production, {"Sahara"}, VendorCapability::BootROM};
    VendorInfo c{"mtk", "MediaTek", "2.0", "auth", "desc2", VendorFamily::MediaTek, VendorMaturity::Scaffold, {"BROM"}, VendorCapability::None};
    REQUIRE(a.id == b.id);
    REQUIRE(a.name == b.name);
    REQUIRE(a.family == b.family);
    REQUIRE(a.id != c.id);
    REQUIRE(a.family != c.family);
}

void testVendorStatisticsUpdate() {
    VendorStatistics stats;
    stats.devicesDetected = 5;
    stats.successfulSessions = 10;
    stats.failedSessions = 2;
    stats.uploads = 3;
    stats.flashes = 7;
    stats.averageBootTime = std::chrono::milliseconds(1500);
    REQUIRE(stats.devicesDetected == uint64_t(5));
    REQUIRE(stats.successfulSessions == uint64_t(10));
    REQUIRE(stats.failedSessions == uint64_t(2));
    REQUIRE(stats.uploads == uint64_t(3));
    REQUIRE(stats.flashes == uint64_t(7));
    REQUIRE(stats.averageBootTime.count() == int64_t(1500));
}

// ============================================================
// 2. Vendor Registry Tests
// ============================================================

void testRegisterVendor() {
    VendorRegistry reg;
    auto result = reg.registerVendor(makeQualcommVendor());
    REQUIRE(result.isOk());
    REQUIRE(reg.vendorCount() == std::size_t(1));
}

void testRegisterDuplicateVendorFails() {
    VendorRegistry reg;
    reg.registerVendor(makeQualcommVendor());
    auto result = reg.registerVendor(makeQualcommVendor());
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::AlreadyExists);
}

void testUnregisterVendor() {
    VendorRegistry reg;
    reg.registerVendor(makeQualcommVendor());
    REQUIRE(reg.vendorCount() == std::size_t(1));
    auto result = reg.unregisterVendor("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(reg.vendorCount() == std::size_t(0));
}

void testUnregisterNonexistentVendorFails() {
    VendorRegistry reg;
    auto result = reg.unregisterVendor("nonexistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testResolveById() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto* vendor = reg.resolveById("qualcomm");
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().family == VendorFamily::Qualcomm);
    auto* notFound = reg.resolveById("nonexistent");
    REQUIRE(notFound == nullptr);
}

void testResolveByFamily() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto* vendor = reg.resolveByFamily(VendorFamily::Qualcomm);
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("qualcomm"));
    auto* notFound = reg.resolveByFamily(VendorFamily::Custom);
    REQUIRE(notFound == nullptr);
}

void testResolveByName() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto* vendor = reg.resolveByName("MediaTek Inc.");
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().family == VendorFamily::MediaTek);
    auto* notFound = reg.resolveByName("Nonexistent Corp");
    REQUIRE(notFound == nullptr);
}

void testResolveByCapability() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto withFlash = reg.resolveByCapability(VendorCapability::Flash);
    REQUIRE(withFlash.size() >= 4);
    auto withSecureBoot = reg.resolveByCapability(VendorCapability::SecureBoot);
    REQUIRE(withSecureBoot.size() == std::size_t(1));
}

void testResolveByProtocol() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto sahara = reg.resolveByProtocol("Sahara");
    REQUIRE(sahara.size() == std::size_t(1));
    auto brom = reg.resolveByProtocol("MediaTekBROM");
    REQUIRE(brom.size() == std::size_t(1));
    auto none = reg.resolveByProtocol("NonexistentProtocol");
    REQUIRE(none.empty());
}

void testAllVendorslist() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto all = reg.allVendors();
    REQUIRE(all.size() == std::size_t(5));
}

void testVendorCount() {
    VendorRegistry reg;
    REQUIRE(reg.vendorCount() == std::size_t(0));
    reg.registerVendor(makeQualcommVendor());
    REQUIRE(reg.vendorCount() == std::size_t(1));
    reg.registerVendor(makeMediaTekVendor());
    REQUIRE(reg.vendorCount() == std::size_t(2));
}

void testIsRegistered() {
    VendorRegistry reg;
    populateRegistry(reg);
    REQUIRE(reg.isRegistered("qualcomm"));
    REQUIRE(reg.isRegistered("mediatek"));
    REQUIRE(!reg.isRegistered("nonexistent"));
}

void testRegisterPlugin() {
    VendorRegistry reg;
    auto plugin = std::make_unique<MockVendorPlugin>("plug1", "TestPlugin", VendorFamily::Custom);
    auto result = reg.registerPlugin(std::move(plugin));
    REQUIRE(result.isOk());
    REQUIRE(reg.pluginCount() == std::size_t(1));
}

void testUnregisterPlugin() {
    VendorRegistry reg;
    auto plugin = std::make_unique<MockVendorPlugin>("plug1", "TestPlugin", VendorFamily::Custom);
    reg.registerPlugin(std::move(plugin));
    auto result = reg.unregisterPlugin("TestPlugin");
    REQUIRE(result.isOk());
    REQUIRE(reg.pluginCount() == std::size_t(0));
}

void testMultipleVendorRegistration() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());
    REQUIRE(reg.registerVendor(makeMediaTekVendor()).isOk());
    REQUIRE(reg.registerVendor(makeUnisocVendor()).isOk());
    REQUIRE(reg.registerVendor(makeRockchipVendor()).isOk());
    REQUIRE(reg.registerVendor(makeSamsungVendor()).isOk());
    REQUIRE(reg.vendorCount() == std::size_t(5));
    REQUIRE(reg.registerVendor(makeAmlogicVendor()).isOk());
    REQUIRE(reg.vendorCount() == std::size_t(6));
}

// ============================================================
// 3. Vendor Factory Tests
// ============================================================

void testCreateVendorByFamily() {
    VendorFactory::registerVendor(makeQualcommVendor());
    auto vendor = VendorFactory::createVendor(VendorFamily::Qualcomm);
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().family == VendorFamily::Qualcomm);
    VendorFactory::clearRegistry();
}

void testCreateVendorById() {
    VendorFactory::registerVendor(makeQualcommVendor());
    auto vendor = VendorFactory::createVendor("qualcomm");
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("qualcomm"));
    VendorFactory::clearRegistry();
}

void testCreatePlugin() {
    auto plugin = VendorFactory::createPlugin("test_plugin");
    REQUIRE(plugin == nullptr);
}

void testCreateFromDescriptor() {
    VendorFactory::registerVendor(makeQualcommVendor());
    DeviceDescriptor desc;
    desc.vendor = Vendor::Qualcomm;
    desc.protocolHint = ProtocolType::Sahara;
    desc.connectionPath = "usb:test";
    auto vendor = VendorFactory::createFromDescriptor(desc);
    REQUIRE(vendor != nullptr);
    VendorFactory::clearRegistry();
}

void testCreateFromDiscovery() {
    VendorFactory::registerVendor(makeMediaTekVendor());
    DiscoveryResult dr;
    DeviceDescriptor desc;
    desc.vendor = Vendor::MediaTek;
    desc.protocolHint = ProtocolType::MediaTekBROM;
    desc.connectionPath = "usb:mtk";
    dr.devices.push_back(desc);
    auto vendor = VendorFactory::createFromDiscovery(dr);
    REQUIRE(vendor != nullptr);
    VendorFactory::clearRegistry();
}

void testCreateFromSession() {
    session::SessionConfig config;
    auto vendor = VendorFactory::createFromSession(config);
    REQUIRE(vendor == nullptr);
}

void testFactoryUsesRegistryOnly() {
    VendorFactory::registerVendor(makeQualcommVendor());
    auto vendor = VendorFactory::createVendor(VendorFamily::Qualcomm);
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("qualcomm"));
    VendorFactory::clearRegistry();
}

void testNullVendorFallback() {
    auto vendor = VendorFactory::createVendor(VendorFamily::Unknown);
    REQUIRE(vendor == nullptr);
}

void testFactoryMultipleCalls() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorFactory::registerVendor(makeMediaTekVendor());
    VendorFactory::registerVendor(makeUnisocVendor());
    auto v1 = VendorFactory::createVendor(VendorFamily::Qualcomm);
    auto v2 = VendorFactory::createVendor(VendorFamily::MediaTek);
    auto v3 = VendorFactory::createVendor(VendorFamily::UNISOC);
    REQUIRE(v1 != nullptr);
    REQUIRE(v2 != nullptr);
    REQUIRE(v3 != nullptr);
    REQUIRE(v1->vendorInfo().family == VendorFamily::Qualcomm);
    REQUIRE(v2->vendorInfo().family == VendorFamily::MediaTek);
    REQUIRE(v3->vendorInfo().family == VendorFamily::UNISOC);
    VendorFactory::clearRegistry();
}

void testFactoryThreadSafety() {
    VendorFactory::registerVendor(makeQualcommVendor());
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto v = VendorFactory::createVendor(VendorFamily::Qualcomm);
            if (v) ++successCount;
        });
    }
    for (auto& t : threads) t.join();
    REQUIRE(successCount.load() == 10);
    VendorFactory::clearRegistry();
}

// ============================================================
// 4. Capability Resolver Tests
// ============================================================

void testCanResolveCapability() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    REQUIRE(resolver.canResolve("qualcomm", VendorCapability::BootROM));
    REQUIRE(resolver.canResolve("qualcomm", VendorCapability::Flash));
    REQUIRE(!resolver.canResolve("qualcomm", VendorCapability::Streaming));
    REQUIRE(!resolver.canResolve("nonexistent", VendorCapability::BootROM));
}

void testResolveCapabilitiesForVendor() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto caps = resolver.resolveForVendor("qualcomm");
    REQUIRE(!caps.empty());
    bool hasBoot = false, hasFlash = false;
    for (auto c : caps) {
        if (c == VendorCapability::BootROM) hasBoot = true;
        if (c == VendorCapability::Flash) hasFlash = true;
    }
    REQUIRE(hasBoot);
    REQUIRE(hasFlash);
}

void testResolveVendorsByCapability() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto vendors = resolver.resolveVendorsByCapability(VendorCapability::Flash);
    REQUIRE(vendors.size() >= 4);
    auto secure = resolver.resolveVendorsByCapability(VendorCapability::SecureBoot);
    REQUIRE(secure.size() == std::size_t(1));
}

void testCapabilitiesForUnknownVendorNone() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto caps = resolver.resolveCapabilities("nonexistent");
    REQUIRE(caps == VendorCapability::None);
}

void testOptionalCapabilitiesEmpty() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto opt = resolver.resolveOptional("qualcomm");
    REQUIRE(opt.empty());
}

void testFallbackCheck() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    REQUIRE(!resolver.hasFallback("qualcomm", VendorCapability::BootROM));
    REQUIRE(!resolver.hasFallback("nonexistent", VendorCapability::BootROM));
}

void testMultipleVendorsDifferentCapabilities() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    REQUIRE(resolver.canResolve("samsung", VendorCapability::SecureBoot));
    REQUIRE(resolver.canResolve("samsung", VendorCapability::Authentication));
    REQUIRE(!resolver.canResolve("rockchip", VendorCapability::Authentication));
    REQUIRE(!resolver.canResolve("rockchip", VendorCapability::SecureBoot));
}

void testSameCapabilityOnMultipleVendors() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto vendors = resolver.resolveVendorsByCapability(VendorCapability::BootROM);
    REQUIRE(vendors.size() == std::size_t(5));
}

void testAllCapabilitiesEnumerated() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto caps = resolver.resolveForVendor("samsung");
    bool hasSecureBoot = false, hasAuth = false, hasGPT = false;
    for (auto c : caps) {
        if (c == VendorCapability::SecureBoot) hasSecureBoot = true;
        if (c == VendorCapability::Authentication) hasAuth = true;
        if (c == VendorCapability::GPT) hasGPT = true;
    }
    REQUIRE(hasSecureBoot);
    REQUIRE(hasAuth);
    REQUIRE(hasGPT);
}

void testCapabilityBitmaskCorrectness() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    auto qcCaps = resolver.resolveCapabilities("qualcomm");
    auto mtkCaps = resolver.resolveCapabilities("mediatek");
    REQUIRE(hasCapability(qcCaps, VendorCapability::Flash));
    REQUIRE(hasCapability(qcCaps, VendorCapability::Verify));
    REQUIRE(hasCapability(mtkCaps, VendorCapability::BootROM));
    REQUIRE(!hasCapability(mtkCaps, VendorCapability::SecureBoot));
}

// ============================================================
// 5. Vendor Runtime Tests
// ============================================================

void testInitializeRuntime() {
    VendorRuntime runtime;
    VendorContext ctx;
    ctx.vendorId = "test";
    auto result = runtime.initialize(ctx);
    REQUIRE(result.isOk());
    REQUIRE(runtime.isInitialized());
}

void testShutdownRuntime() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    REQUIRE(runtime.isInitialized());
    auto result = runtime.shutdown();
    REQUIRE(result.isOk());
    REQUIRE(!runtime.isInitialized());
}

void testDoubleInitializeFails() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto result = runtime.initialize(ctx);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidState);
}

void testLoadVendor() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto result = runtime.loadVendor("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(runtime.activeVendor() != nullptr);
    VendorFactory::clearRegistry();
}

void testUnloadVendor() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    runtime.loadVendor("qualcomm");
    auto result = runtime.unloadVendor("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(runtime.activeVendor() == nullptr);
    VendorFactory::clearRegistry();
}

void testActiveVendorTracking() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    REQUIRE(runtime.activeVendor() == nullptr);
    runtime.loadVendor("qualcomm");
    REQUIRE(runtime.activeVendor() != nullptr);
    REQUIRE(runtime.activeVendor()->vendorInfo().id == std::string("qualcomm"));
    runtime.unloadVendor("qualcomm");
    REQUIRE(runtime.activeVendor() == nullptr);
    VendorFactory::clearRegistry();
}

void testLoadPlugin() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto plugin = std::make_unique<MockVendorPlugin>("test", "TestPlugin", VendorFamily::Custom);
    auto result = runtime.loadPlugin(std::move(plugin));
    REQUIRE(result.isOk());
}

void testUnloadPlugin() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto plugin = std::make_unique<MockVendorPlugin>("test", "TestPlugin", VendorFamily::Custom);
    runtime.loadPlugin(std::move(plugin));
    auto result = runtime.unloadPlugin("TestPlugin");
    REQUIRE(result.isOk());
}

void testCreateRuntimeContext() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto result = runtime.createRuntimeContext("qualcomm");
    REQUIRE(result.isOk());
}

void testDoubleShutdownOk() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    REQUIRE(runtime.shutdown().isOk());
    REQUIRE(!runtime.isInitialized());
    auto result = runtime.shutdown();
    REQUIRE(result.isOk());
    REQUIRE(!runtime.isInitialized());
}

// ============================================================
// 6. Vendor Session Tests
// ============================================================

void testOpenSession() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorSession session;
    auto result = session.open("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(session.isOpen());
    VendorFactory::clearRegistry();
}

void testCloseSession() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorSession session;
    session.open("qualcomm");
    REQUIRE(session.isOpen());
    auto result = session.close();
    REQUIRE(result.isOk());
    REQUIRE(!session.isOpen());
    VendorFactory::clearRegistry();
}

void testReconnectSession() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorSession session;
    session.open("qualcomm");
    REQUIRE(session.isOpen());
    auto result = session.reconnect();
    REQUIRE(result.isOk());
    REQUIRE(session.isOpen());
    VendorFactory::clearRegistry();
}

void testResetSession() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorSession session;
    session.open("qualcomm");
    auto result = session.reset();
    REQUIRE(result.isOk());
    REQUIRE(session.isOpen());
    VendorFactory::clearRegistry();
}

void testOpenTwiceFails() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorSession session;
    session.open("qualcomm");
    auto result = session.open("mediatek");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::TransportAlreadyOpen);
    VendorFactory::clearRegistry();
}

void testSessionStatistics() {
    VendorSession session;
    auto stats = session.statistics();
    REQUIRE(stats.devicesDetected == uint64_t(0));
    REQUIRE(stats.successfulSessions == uint64_t(0));
    REQUIRE(stats.failedSessions == uint64_t(0));
}

void testSessionCapabilities() {
    VendorSession session;
    auto caps = session.capabilities();
    REQUIRE(caps == VendorCapability::None);
    VendorFactory::registerVendor(makeQualcommVendor());
    session.open("qualcomm");
    caps = session.capabilities();
    REQUIRE(caps != VendorCapability::None);
    REQUIRE(hasCapability(caps, VendorCapability::BootROM));
    VendorFactory::clearRegistry();
}

void testSessionActiveVendorId() {
    VendorSession session;
    REQUIRE(session.activeVendorId().empty());
    VendorFactory::registerVendor(makeQualcommVendor());
    session.open("qualcomm");
    REQUIRE(session.activeVendorId() == std::string("qualcomm"));
    VendorFactory::clearRegistry();
}

void testSessionIsOpenState() {
    VendorSession session;
    REQUIRE(!session.isOpen());
    VendorFactory::registerVendor(makeQualcommVendor());
    session.open("qualcomm");
    REQUIRE(session.isOpen());
    session.close();
    REQUIRE(!session.isOpen());
    VendorFactory::clearRegistry();
}

void testSessionMoveCopyRestrictions() {
    VendorSession s1;
    REQUIRE(!s1.isOpen());
    VendorSession s2;
    REQUIRE(!s2.isOpen());
}

// ============================================================
// 7. Vendor Pipeline Tests
// ============================================================

void testExecutePipeline() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    auto result = pipeline.execute("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(!pipeline.isRunning());
}

void testCancelPipeline() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    pipeline.execute("qualcomm");
    auto result = pipeline.cancel();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidState);
}

void testIsRunningState() {
    VendorContext ctx;
    VendorPipeline pipeline(ctx);
    REQUIRE(!pipeline.isRunning());
}

void testDoubleExecuteSequential() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    auto r1 = pipeline.execute("qualcomm");
    REQUIRE(r1.isOk());
    auto r2 = pipeline.execute("qualcomm");
    REQUIRE(r2.isOk());
    REQUIRE(pipeline.currentVendorId() == std::string("qualcomm"));
}

void testProgressCallback() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    std::string lastStage;
    int lastPercent = -1;
    pipeline.setProgressCallback([&](const std::string& stage, int percent) {
        lastStage = stage;
        lastPercent = percent;
    });
    auto result = pipeline.execute("qualcomm");
    REQUIRE(result.isOk());
}

void testCurrentVendorId() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    REQUIRE(pipeline.currentVendorId().empty());
    pipeline.execute("qualcomm");
    REQUIRE(pipeline.currentVendorId() == std::string("qualcomm"));
}

void testPipelineWithWorkflow() {
    VendorContext ctx;
    ctx.vendorId = "mediatek";
    VendorPipeline pipeline(ctx);
    auto result = pipeline.executeWithWorkflow("mediatek");
    REQUIRE(result.isOk());
    REQUIRE(pipeline.currentVendorId() == std::string("mediatek"));
}

void testCancelNonRunning() {
    VendorContext ctx;
    VendorPipeline pipeline(ctx);
    auto result = pipeline.cancel();
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidState);
}

// ============================================================
// 8. Vendor Monitor Tests
// ============================================================

void testRecordEvent() {
    VendorMonitor monitor;
    VendorEvent ev;
    ev.type = VendorEventType::VendorLoaded;
    ev.vendorId = "qualcomm";
    ev.message = "loaded ok";
    ev.success = true;
    monitor.recordEvent(ev);
    auto events = monitor.recentEvents("qualcomm");
    REQUIRE(events.size() == std::size_t(1));
    REQUIRE(events[0].type == VendorEventType::VendorLoaded);
    REQUIRE(events[0].message == std::string("loaded ok"));
}

void testRecordSession() {
    VendorMonitor monitor;
    monitor.recordSession("qualcomm", true, std::chrono::milliseconds(100));
    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.successfulSessions == uint64_t(1));
    REQUIRE(stats.failedSessions == uint64_t(0));
}

void testRecordBoot() {
    VendorMonitor monitor;
    monitor.recordBoot("qualcomm", std::chrono::milliseconds(500));
    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.averageBootTime.count() == int64_t(500));
}

void testRecordUpload() {
    VendorMonitor monitor;
    monitor.recordUpload("qualcomm");
    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.uploads == uint64_t(1));
}

void testRecordFlash() {
    VendorMonitor monitor;
    monitor.recordFlash("qualcomm");
    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.flashes == uint64_t(1));
}

void testRecordFailure() {
    VendorMonitor monitor;
    monitor.recordFailure("qualcomm", ErrorCode::ProtocolError);
    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.failedSessions == uint64_t(1));
}

void testMonitorStatisticsRetrieval() {
    VendorMonitor monitor;
    monitor.recordSession("qcom", true, std::chrono::milliseconds(200));
    monitor.recordUpload("qcom");
    monitor.recordFlash("qcom");
    monitor.recordBoot("qcom", std::chrono::milliseconds(300));
    auto stats = monitor.statistics("qcom");
    REQUIRE(stats.successfulSessions == uint64_t(1));
    REQUIRE(stats.uploads == uint64_t(1));
    REQUIRE(stats.flashes == uint64_t(1));
    REQUIRE(stats.averageBootTime.count() == int64_t(250));
}

void testRecentEvents() {
    VendorMonitor monitor;
    for (int i = 0; i < 10; ++i) {
        VendorEvent ev;
        ev.type = VendorEventType::VendorLoaded;
        ev.vendorId = "qcom";
        ev.message = "event " + std::to_string(i);
        ev.success = true;
        monitor.recordEvent(ev);
    }
    auto events = monitor.recentEvents("qcom", 5);
    REQUIRE(events.size() == std::size_t(5));
    REQUIRE(events[0].message == std::string("event 9"));
}

void testClearVendor() {
    VendorMonitor monitor;
    monitor.recordSession("qcom", true, std::chrono::milliseconds(100));
    monitor.recordSession("mtk", true, std::chrono::milliseconds(200));
    monitor.clear("qcom");
    auto stats = monitor.statistics("qcom");
    REQUIRE(stats.successfulSessions == uint64_t(0));
    auto mtkStats = monitor.statistics("mtk");
    REQUIRE(mtkStats.successfulSessions == uint64_t(1));
}

void testClearAll() {
    VendorMonitor monitor;
    monitor.recordSession("qcom", true, std::chrono::milliseconds(100));
    monitor.recordSession("mtk", true, std::chrono::milliseconds(200));
    monitor.clearAll();
    REQUIRE(monitor.totalSessions() == std::size_t(0));
}

void testEventCallback() {
    VendorMonitor monitor;
    int callbackCount = 0;
    VendorEvent captured;
    monitor.setEventCallback([&](const VendorEvent& e) {
        ++callbackCount;
        captured = e;
    });
    VendorEvent ev;
    ev.type = VendorEventType::SessionOpened;
    ev.vendorId = "qcom";
    ev.message = "opened";
    ev.success = true;
    monitor.recordEvent(ev);
    REQUIRE(callbackCount == 1);
    REQUIRE(captured.type == VendorEventType::SessionOpened);
    REQUIRE(captured.vendorId == std::string("qcom"));
}

void testTotalSessionsFailures() {
    VendorMonitor monitor;
    monitor.recordSession("a", true, std::chrono::milliseconds(10));
    monitor.recordSession("a", false, std::chrono::milliseconds(10));
    monitor.recordSession("b", true, std::chrono::milliseconds(10));
    monitor.recordFailure("c", ErrorCode::Unknown);
    REQUIRE(monitor.totalSessions() == std::size_t(4));
    REQUIRE(monitor.totalFailures() == std::size_t(2));
}

// ============================================================
// 9. Vendor Package Resolver Tests
// ============================================================

void testResolvePackage() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolvePackage("test/path");
    REQUIRE(result.isError());
}

void testResolveLoader() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolveLoader("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(result.value() == std::string("loader.elf"));
}

void testResolveGptLayout() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolveGptLayout("qualcomm");
    REQUIRE(result.isOk());
}

void testResolvePartitionLayout() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolvePartitionLayout("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(result.value().empty());
}

void testResolveFlashingSequence() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolveFlashingSequence("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(result.value().size() == std::size_t(4));
    REQUIRE(result.value()[0] == std::string("programmer"));
}

void testPackageWithPath() {
    VendorContext ctx;
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolvePackage("/some/firmware/dir");
    REQUIRE(result.isError());
}

void testPackageNotFound() {
    VendorContext ctx;
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolvePackage("");
    REQUIRE(result.isError());
}

void testPackageValidation() {
    VendorContext ctx;
    VendorPackageResolver resolver(ctx);
    auto result = resolver.resolveLoader("nonexistent");
    REQUIRE(result.isOk());
}

// ============================================================
// 10. Virtual Vendor Tests
// ============================================================

void testVirtualQualcommVendorCreation() {
    auto vendor = makeQualcommVendor();
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("qualcomm"));
}

void testVirtualMediaTekVendorCreation() {
    auto vendor = makeMediaTekVendor();
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("mediatek"));
}

void testVirtualUnisocVendorCreation() {
    auto vendor = makeUnisocVendor();
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("unisoc"));
}

void testVirtualRockchipVendorCreation() {
    auto vendor = makeRockchipVendor();
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("rockchip"));
}

void testVirtualSamsungVendorCreation() {
    auto vendor = makeSamsungVendor();
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("samsung"));
}

void testVirtualVendorCapabilitiesCorrect() {
    auto qc = makeQualcommVendor();
    REQUIRE(hasCapability(qc->capabilities(), VendorCapability::BootROM));
    REQUIRE(hasCapability(qc->capabilities(), VendorCapability::Flash));
    REQUIRE(hasCapability(qc->capabilities(), VendorCapability::Verify));
    auto rk = makeRockchipVendor();
    REQUIRE(hasCapability(rk->capabilities(), VendorCapability::Reset));
    REQUIRE(!hasCapability(rk->capabilities(), VendorCapability::GPT));
}

void testVirtualVendorFamilyCorrect() {
    REQUIRE(makeQualcommVendor()->vendorInfo().family == VendorFamily::Qualcomm);
    REQUIRE(makeMediaTekVendor()->vendorInfo().family == VendorFamily::MediaTek);
    REQUIRE(makeUnisocVendor()->vendorInfo().family == VendorFamily::UNISOC);
    REQUIRE(makeRockchipVendor()->vendorInfo().family == VendorFamily::Rockchip);
    REQUIRE(makeSamsungVendor()->vendorInfo().family == VendorFamily::Samsung);
}

void testVirtualVendorInfoFilled() {
    auto vendor = makeQualcommVendor();
    REQUIRE(!vendor->vendorInfo().id.empty());
    REQUIRE(!vendor->vendorInfo().name.empty());
    REQUIRE(!vendor->vendorInfo().supportedProtocols.empty());
    REQUIRE(vendor->vendorInfo().supportedProtocols.size() == std::size_t(2));
}

void testVirtualVendorPluginCreate() {
    auto plugin = std::make_unique<MockVendorPlugin>("qcom_plugin", "QualcommPlugin", VendorFamily::Qualcomm);
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->vendorInfo().family == VendorFamily::Qualcomm);
}

void testVirtualVendorPluginRegister() {
    VendorRegistry reg;
    auto plugin = std::make_unique<MockVendorPlugin>("p1", "Plugin1", VendorFamily::Qualcomm);
    auto result = reg.registerPlugin(std::move(plugin));
    REQUIRE(result.isOk());
    REQUIRE(reg.pluginCount() == std::size_t(1));
}

void testFrameworkRegisterAll() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());
    REQUIRE(reg.registerVendor(makeMediaTekVendor()).isOk());
    REQUIRE(reg.registerVendor(makeUnisocVendor()).isOk());
    REQUIRE(reg.registerVendor(makeRockchipVendor()).isOk());
    REQUIRE(reg.registerVendor(makeSamsungVendor()).isOk());
    REQUIRE(reg.vendorCount() == std::size_t(5));
}

void testFrameworkUnregisterAll() {
    VendorRegistry reg;
    reg.registerVendor(makeQualcommVendor());
    reg.registerVendor(makeMediaTekVendor());
    reg.registerVendor(makeUnisocVendor());
    REQUIRE(reg.vendorCount() == std::size_t(3));
    REQUIRE(reg.unregisterVendor("qualcomm").isOk());
    REQUIRE(reg.unregisterVendor("mediatek").isOk());
    REQUIRE(reg.unregisterVendor("unisoc").isOk());
    REQUIRE(reg.vendorCount() == std::size_t(0));
}

void testFrameworkRegisterSingle() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());
    REQUIRE(reg.vendorCount() == std::size_t(1));
    REQUIRE(reg.isRegistered("qualcomm"));
}

void testResolveVirtualVendorByFamily() {
    VendorRegistry reg;
    populateRegistry(reg);
    auto* vendor = reg.resolveByFamily(VendorFamily::Samsung);
    REQUIRE(vendor != nullptr);
    REQUIRE(vendor->vendorInfo().id == std::string("samsung"));
}

void testCreateVirtualVendorDetector() {
    auto vendor = makeQualcommVendor();
    auto detector = vendor->createDetector();
    REQUIRE(detector == nullptr);
}

// ============================================================
// 11. Integration Tests
// ============================================================

void testFullRegistrationResolutionCycle() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());
    REQUIRE(reg.registerVendor(makeMediaTekVendor()).isOk());
    REQUIRE(reg.registerVendor(makeSamsungVendor()).isOk());

    auto* qc = reg.resolveById("qualcomm");
    REQUIRE(qc != nullptr);
    REQUIRE(qc->vendorInfo().family == VendorFamily::Qualcomm);
    REQUIRE(qc->vendorInfo().supportedProtocols.size() == std::size_t(2));

    auto* mtk = reg.resolveByFamily(VendorFamily::MediaTek);
    REQUIRE(mtk != nullptr);
    REQUIRE(mtk->vendorInfo().id == std::string("mediatek"));

    auto flashVendors = reg.resolveByCapability(VendorCapability::Flash);
    REQUIRE(flashVendors.size() == std::size_t(3));

    auto saharaVendors = reg.resolveByProtocol("Sahara");
    REQUIRE(saharaVendors.size() == std::size_t(1));

    REQUIRE(reg.vendorCount() == std::size_t(3));
    REQUIRE(reg.isRegistered("qualcomm"));
    REQUIRE(!reg.isRegistered("nonexistent"));
}

void testRuntimeSessionLifecycle() {
    VendorFactory::registerVendor(makeQualcommVendor());
    VendorRuntime runtime;
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    REQUIRE(runtime.initialize(ctx).isOk());
    REQUIRE(runtime.isInitialized());

    REQUIRE(runtime.loadVendor("qualcomm").isOk());
    REQUIRE(runtime.activeVendor() != nullptr);
    REQUIRE(runtime.activeVendor()->vendorInfo().id == std::string("qualcomm"));

    REQUIRE(runtime.unloadVendor("qualcomm").isOk());
    REQUIRE(runtime.activeVendor() == nullptr);

    REQUIRE(runtime.shutdown().isOk());
    REQUIRE(!runtime.isInitialized());
    VendorFactory::clearRegistry();
}

void testMonitorEventsIntegration() {
    VendorMonitor monitor;
    int eventsRecorded = 0;
    monitor.setEventCallback([&](const VendorEvent&) { ++eventsRecorded; });

    VendorEvent ev1;
    ev1.type = VendorEventType::VendorLoaded;
    ev1.vendorId = "qualcomm";
    ev1.message = "Loaded";
    monitor.recordEvent(ev1);

    monitor.recordSession("qualcomm", true, std::chrono::milliseconds(500));
    monitor.recordBoot("qualcomm", std::chrono::milliseconds(450));
    monitor.recordUpload("qualcomm");
    monitor.recordFlash("qualcomm");

    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.successfulSessions == uint64_t(1));
    REQUIRE(stats.uploads == uint64_t(1));
    REQUIRE(stats.flashes == uint64_t(1));
    REQUIRE(stats.averageBootTime.count() == int64_t(475));

    auto events = monitor.recentEvents("qualcomm");
    REQUIRE(events.size() == std::size_t(1));

    REQUIRE(eventsRecorded == 1);
    REQUIRE(monitor.totalSessions() == std::size_t(1));
}

void testPipelineCancellationWithContext() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPipeline pipeline(ctx);
    auto result = pipeline.execute("qualcomm");
    REQUIRE(result.isOk());
    REQUIRE(!pipeline.isRunning());
}

void testCapabilityRegistryIntegration() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);

    auto* qc = reg.resolveById("qualcomm");
    REQUIRE(qc != nullptr);

    auto qcCaps = resolver.resolveCapabilities("qualcomm");
    REQUIRE(hasCapability(qcCaps, VendorCapability::BootROM));
    REQUIRE(hasCapability(qcCaps, VendorCapability::Flash));
    REQUIRE(hasCapability(qcCaps, VendorCapability::Verify));

    auto flashVendors = resolver.resolveVendorsByCapability(VendorCapability::Flash);
    REQUIRE(flashVendors.size() >= 4);

    REQUIRE(resolver.canResolve("qualcomm", VendorCapability::BootROM));
    REQUIRE(!resolver.canResolve("qualcomm", VendorCapability::Streaming));
}

void testVirtualVendorThroughRegistry() {
    VendorRegistry reg;
    reg.registerVendor(makeQualcommVendor());

    auto* resolved = reg.resolveById("qualcomm");
    REQUIRE(resolved != nullptr);
    REQUIRE(resolved->vendorInfo().family == VendorFamily::Qualcomm);
    REQUIRE(resolved->name() == std::string_view("Qualcomm Technologies"));
}

void testSessionStatisticsAfterOperations() {
    VendorSession session;
    auto stats = session.statistics();
    REQUIRE(stats.devicesDetected == uint64_t(0));
    REQUIRE(stats.successfulSessions == uint64_t(0));
    REQUIRE(stats.failedSessions == uint64_t(0));

    // open without a registry will fail → failedSessions increments
    auto r = session.open("nonexistent");
    REQUIRE(r.isError());
    stats = session.statistics();
    REQUIRE(stats.failedSessions == uint64_t(1));
    REQUIRE(stats.successfulSessions == uint64_t(0));
}

void testMultipleConcurrentVendors() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());
    REQUIRE(reg.registerVendor(makeMediaTekVendor()).isOk());
    REQUIRE(reg.registerVendor(makeUnisocVendor()).isOk());
    REQUIRE(reg.registerVendor(makeRockchipVendor()).isOk());
    REQUIRE(reg.registerVendor(makeSamsungVendor()).isOk());
    REQUIRE(reg.registerVendor(makeAmlogicVendor()).isOk());

    auto all = reg.allVendors();
    REQUIRE(all.size() == std::size_t(6));

    auto byBoot = reg.resolveByCapability(VendorCapability::BootROM);
    REQUIRE(byBoot.size() == std::size_t(6));
}

void testVendorPluginRuntime() {
    VendorRuntime runtime;
    VendorContext ctx;
    runtime.initialize(ctx);
    auto plugin = std::make_unique<MockVendorPlugin>("test", "TestPlugin", VendorFamily::Custom);
    auto result = runtime.loadPlugin(std::move(plugin));
    REQUIRE(result.isOk());
    auto unloadResult = runtime.unloadPlugin("TestPlugin");
    REQUIRE(unloadResult.isOk());
}

void testEventCallbackFiring() {
    VendorMonitor monitor;
    std::vector<VendorEvent> received;
    monitor.setEventCallback([&](const VendorEvent& e) { received.push_back(e); });

    VendorEvent e1;
    e1.type = VendorEventType::VendorLoaded;
    e1.vendorId = "q1";
    e1.message = "v1";
    monitor.recordEvent(e1);

    VendorEvent e2;
    e2.type = VendorEventType::SessionOpened;
    e2.vendorId = "q2";
    e2.message = "v2";
    monitor.recordEvent(e2);

    REQUIRE(received.size() == std::size_t(2));
    REQUIRE(received[0].vendorId == std::string("q1"));
    REQUIRE(received[1].vendorId == std::string("q2"));
}

void testRuntimeContextSetup() {
    VendorRuntime runtime;
    VendorContext ctx;
    ctx.vendorId = "test";
    ctx.properties["key1"] = "value1";
    ctx.properties["key2"] = "value2";
    REQUIRE(runtime.initialize(ctx).isOk());
    REQUIRE(runtime.context().vendorId == std::string("test"));
    REQUIRE(runtime.context().properties.at("key1") == std::string("value1"));
}

void testPipelineProgressCallback() {
    VendorContext ctx;
    VendorPipeline pipeline(ctx);
    std::vector<std::pair<std::string, int>> progress;
    pipeline.setProgressCallback([&](const std::string& s, int p) {
        progress.push_back({s, p});
    });
    auto result = pipeline.execute("test");
    REQUIRE(result.isOk());
}

void testPackageResolveThenFlashSequence() {
    VendorContext ctx;
    ctx.vendorId = "qualcomm";
    VendorPackageResolver resolver(ctx);
    auto loader = resolver.resolveLoader("qualcomm");
    REQUIRE(loader.isOk());
    auto seq = resolver.resolveFlashingSequence("qualcomm");
    REQUIRE(seq.isOk());
    REQUIRE(seq.value().size() == std::size_t(4));
}

void testEndToEndVirtualVendorWorkflow() {
    VendorRegistry reg;
    REQUIRE(reg.registerVendor(makeQualcommVendor()).isOk());

    auto* vendor = reg.resolveById("qualcomm");
    REQUIRE(vendor != nullptr);

    VendorContext ctx;
    ctx.vendorId = "qualcomm";

    VendorRuntime runtime;
    REQUIRE(runtime.initialize(ctx).isOk());

    VendorMonitor monitor;
    monitor.recordEvent({VendorEventType::VendorLoaded, "qualcomm", "loaded", true, {}});
    monitor.recordSession("qualcomm", true, std::chrono::milliseconds(100));

    auto stats = monitor.statistics("qualcomm");
    REQUIRE(stats.successfulSessions == uint64_t(1));

    REQUIRE(reg.vendorCount() == std::size_t(1));
    REQUIRE(reg.isRegistered("qualcomm"));
}

void testMonitorRecordsCorrectly() {
    VendorMonitor monitor;
    monitor.recordSession("v1", true, std::chrono::milliseconds(100));
    monitor.recordSession("v1", true, std::chrono::milliseconds(200));
    monitor.recordSession("v1", false, std::chrono::milliseconds(50));
    monitor.recordUpload("v1");
    monitor.recordFlash("v1");
    monitor.recordFlash("v1");

    auto stats = monitor.statistics("v1");
    REQUIRE(stats.successfulSessions == uint64_t(2));
    REQUIRE(stats.failedSessions == uint64_t(1));
    REQUIRE(stats.uploads == uint64_t(1));
    REQUIRE(stats.flashes == uint64_t(2));

    monitor.recordSession("v2", true, std::chrono::milliseconds(300));
    REQUIRE(monitor.totalSessions() == std::size_t(4));
    REQUIRE(monitor.totalFailures() == std::size_t(1));

    monitor.clear("v1");
    auto cleared = monitor.statistics("v1");
    REQUIRE(cleared.successfulSessions == uint64_t(0));
}

// ============================================================
// 12. Edge Case Tests
// ============================================================

void testEmptyVendorId() {
    VendorRegistry reg;
    auto vendor = std::make_unique<MockVendor>("", "Empty", VendorFamily::Unknown);
    auto result = reg.registerVendor(std::move(vendor));
    REQUIRE(result.isOk());
    auto* resolved = reg.resolveById("");
    REQUIRE(resolved != nullptr);
}

void testUnknownFamily() {
    auto vendor = std::make_unique<MockVendor>("unknown", "Unknown", VendorFamily::Unknown);
    REQUIRE(vendor->vendorInfo().family == VendorFamily::Unknown);
}

void testNullVendorPointer() {
    VendorRegistry reg;
    auto result = reg.registerVendor(nullptr);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

void testEmptyRegistry() {
    VendorRegistry reg;
    REQUIRE(reg.vendorCount() == std::size_t(0));
    REQUIRE(reg.allVendors().empty());
    REQUIRE(reg.allPlugins().empty());
    REQUIRE(reg.resolveById("anything") == nullptr);
    REQUIRE(reg.resolveByFamily(VendorFamily::Qualcomm) == nullptr);
    REQUIRE(reg.resolveByName("anything") == nullptr);
    REQUIRE(reg.resolveByCapability(VendorCapability::BootROM).empty());
    REQUIRE(reg.resolveByProtocol("Sahara").empty());
}

void testZeroCapabilities() {
    auto vendor = std::make_unique<MockVendor>("nocaps", "NoCapabilities", VendorFamily::Custom, VendorCapability::None);
    REQUIRE(vendor->capabilities() == VendorCapability::None);
    REQUIRE(!hasCapability(vendor->capabilities(), VendorCapability::BootROM));
}

void testMismatchedCase() {
    VendorRegistry reg;
    reg.registerVendor(makeQualcommVendor());
    auto* lower = reg.resolveById("qualcomm");
    REQUIRE(lower != nullptr);
    auto* upper = reg.resolveById("QUALCOMM");
    REQUIRE(upper == nullptr);
}

void testVeryLongVendorNames() {
    std::string longId(256, 'x');
    std::string longName(512, 'Y');
    auto vendor = std::make_unique<MockVendor>(longId, longName, VendorFamily::Custom);
    VendorRegistry reg;
    auto result = reg.registerVendor(std::move(vendor));
    REQUIRE(result.isOk());
    auto* resolved = reg.resolveById(longId);
    REQUIRE(resolved != nullptr);
    REQUIRE(resolved->vendorInfo().name == longName);
}

void testSpecialCharactersInIds() {
    std::string specialId = "ven-dor_1.0@test#special!chars";
    auto vendor = std::make_unique<MockVendor>(specialId, "Special", VendorFamily::Custom);
    VendorRegistry reg;
    auto result = reg.registerVendor(std::move(vendor));
    REQUIRE(result.isOk());
    auto* resolved = reg.resolveById(specialId);
    REQUIRE(resolved != nullptr);
}

void testMaximumVendors() {
    VendorRegistry reg;
    for (int i = 0; i < 100; ++i) {
        auto id = "vendor_" + std::to_string(i);
        auto vendor = std::make_unique<MockVendor>(id, "Vendor" + std::to_string(i), VendorFamily::Custom);
        auto result = reg.registerVendor(std::move(vendor));
        REQUIRE(result.isOk());
    }
    REQUIRE(reg.vendorCount() == std::size_t(100));
    REQUIRE(reg.allVendors().size() == std::size_t(100));
}

void testRapidRegisterUnregister() {
    VendorRegistry reg;
    for (int i = 0; i < 50; ++i) {
        auto id = "vendor_" + std::to_string(i);
        auto vendor = std::make_unique<MockVendor>(id, "V" + std::to_string(i), VendorFamily::Custom);
        REQUIRE(reg.registerVendor(std::move(vendor)).isOk());
        REQUIRE(reg.unregisterVendor(id).isOk());
    }
    REQUIRE(reg.vendorCount() == std::size_t(0));
}

// ============================================================
// 13. Stress Tests
// ============================================================

void testStressRegistryCycles() {
    VendorRegistry reg;
    for (int i = 0; i < 1000; ++i) {
        auto id = "stress_" + std::to_string(i);
        REQUIRE(reg.registerVendor(std::make_unique<MockVendor>(id, "S" + std::to_string(i), VendorFamily::Custom)).isOk());
    }
    REQUIRE(reg.vendorCount() == std::size_t(1000));
    for (int i = 0; i < 1000; ++i) {
        auto id = "stress_" + std::to_string(i);
        auto* v = reg.resolveById(id);
        REQUIRE(v != nullptr);
    }
    for (int i = 0; i < 1000; ++i) {
        auto id = "stress_" + std::to_string(i);
        REQUIRE(reg.unregisterVendor(id).isOk());
    }
    REQUIRE(reg.vendorCount() == std::size_t(0));
}

void testStressConcurrentVirtualVendors() {
    VendorRegistry reg;
    for (int i = 0; i < 100; ++i) {
        auto id = "concurrent_" + std::to_string(i);
        VendorFamily family = static_cast<VendorFamily>(static_cast<uint32_t>(VendorFamily::Custom));
        auto vendor = std::make_unique<MockVendor>(id, "CV" + std::to_string(i), family);
        REQUIRE(reg.registerVendor(std::move(vendor)).isOk());
    }
    REQUIRE(reg.vendorCount() == std::size_t(100));
    auto all = reg.allVendors();
    REQUIRE(all.size() == std::size_t(100));
    auto byBoot = reg.resolveByCapability(VendorCapability::BootROM);
    REQUIRE(byBoot.size() <= 100);
}

void testStressSessionOpenClose() {
    VendorFactory::registerVendor(makeQualcommVendor());
    for (int i = 0; i < 20; ++i) {
        VendorSession session;
        auto result = session.open("qualcomm");
        REQUIRE(result.isOk());
        REQUIRE(session.isOpen());
        REQUIRE(session.close().isOk());
        REQUIRE(!session.isOpen());
    }
    VendorFactory::clearRegistry();
}

void testStressMultipleMonitors() {
    std::vector<std::unique_ptr<VendorMonitor>> monitors;
    for (int i = 0; i < 50; ++i) {
        monitors.push_back(std::make_unique<VendorMonitor>());
        monitors.back()->recordSession("v", true, std::chrono::milliseconds(i));
    }
    for (auto& m : monitors) {
        auto stats = m->statistics("v");
        REQUIRE(stats.successfulSessions == uint64_t(1));
    }
}

void testStressRegistryThreadSafety() {
    VendorRegistry reg;
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i) {
                auto id = "thread_" + std::to_string(t) + "_" + std::to_string(i);
                auto vendor = std::make_unique<MockVendor>(id, "T" + std::to_string(t) + "V" + std::to_string(i), VendorFamily::Custom);
                auto result = reg.registerVendor(std::move(vendor));
                if (result.isOk()) ++success;
            }
        });
    }
    for (auto& t : threads) t.join();
    REQUIRE(static_cast<size_t>(success.load()) == reg.vendorCount());
    REQUIRE(reg.vendorCount() <= 200);
}

void testStressPipeline() {
    VendorContext ctx;
    ctx.vendorId = "stress";
    VendorPipeline pipeline(ctx);
    for (int i = 0; i < 10; ++i) {
        auto result = pipeline.execute("stress_" + std::to_string(i));
        if (i == 0) {
            REQUIRE(result.isOk());
        } else {
            if (result.isError()) {
                REQUIRE(result.error() == ErrorCode::TransportBusy);
            }
        }
    }
}

void testStressFactory() {
    VendorFactory::registerVendor(makeQualcommVendor());
    for (int i = 0; i < 50; ++i) {
        auto v = VendorFactory::createVendor(VendorFamily::Qualcomm);
        REQUIRE(v != nullptr);
    }
    VendorFactory::clearRegistry();
}

void testStressMonitor() {
    VendorMonitor monitor;
    for (int i = 0; i < 100; ++i) {
        monitor.recordSession("stress", true, std::chrono::milliseconds(i));
        monitor.recordUpload("stress");
        monitor.recordFlash("stress");
    }
    auto stats = monitor.statistics("stress");
    REQUIRE(stats.successfulSessions == uint64_t(100));
    REQUIRE(stats.uploads == uint64_t(100));
    REQUIRE(stats.flashes == uint64_t(100));
}

void testStressCapabilityResolve() {
    VendorRegistry reg;
    populateRegistry(reg);
    CapabilityResolver resolver(reg);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(resolver.canResolve("qualcomm", VendorCapability::BootROM));
        REQUIRE(!resolver.canResolve("qualcomm", VendorCapability::Streaming));
        auto vendors = resolver.resolveVendorsByCapability(VendorCapability::Flash);
        REQUIRE(vendors.size() >= 4);
        auto caps = resolver.resolveForVendor("samsung");
        REQUIRE(!caps.empty());
    }
}

void testStressMemoryAllocation() {
    std::vector<std::unique_ptr<IVendor>> vendors;
    for (int i = 0; i < 200; ++i) {
        auto id = "mem_" + std::to_string(i);
        vendors.push_back(std::make_unique<MockVendor>(id, "MemVendor" + std::to_string(i), VendorFamily::Custom));
    }
    REQUIRE(vendors.size() == std::size_t(200));
    vendors.clear();
    REQUIRE(vendors.empty());
}

