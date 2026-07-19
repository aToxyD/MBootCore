#include <catch2/catch_test_macros.hpp>
#include <mbootcore/plugin/PluginTypes.hpp>
#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/IProtocolPlugin.hpp>
#include <mbootcore/plugin/PluginManager.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/discovery/IProtocolFactory.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>
#include <mbootcore/session/DeviceManager.hpp>

#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::plugin;
namespace disc = mbootcore::discovery;

// ============================================================
// Virtual Plugins for Testing
// ============================================================

class VirtualPluginBase : public virtual IPlugin {
public:
    explicit VirtualPluginBase(PluginMetadata meta)
        : m_meta(std::move(meta)) {}

    ~VirtualPluginBase() override = default;

    PluginMetadata metadata() const noexcept override { return m_meta; }
    PluginState state() const noexcept override { return m_state; }
    void setEnabled(bool enabled) noexcept override { m_enabled = enabled; }
    bool isEnabled() const noexcept override { return m_enabled; }

    int initCount() const noexcept { return m_initCount; }
    int shutdownCount() const noexcept { return m_shutdownCount; }
    int regCount() const noexcept { return m_regCount; }
    int unregCount() const noexcept { return m_unregCount; }

    static PluginMetadata makeMetadata(const std::string& name,
                                       PluginCapability caps = PluginCapability::None,
                                       std::vector<PluginDependency> deps = {},
                                       uint32_t compatVersion = 1) {
        PluginMetadata meta;
        meta.name = name;
        meta.version = "1";
        meta.author = "test";
        meta.license = "MIT";
        meta.description = "Virtual plugin for testing: " + name;
        meta.capabilities = caps;
        meta.priority = 100;
        meta.uuid = name + "-uuid";
        meta.compatibilityVersion = compatVersion;
        meta.dependencies = std::move(deps);
        return meta;
    }

protected:
    PluginMetadata m_meta;
    PluginState m_state{PluginState::Unloaded};
    bool m_enabled{true};
    int m_initCount{0};
    int m_shutdownCount{0};
    int m_regCount{0};
    int m_unregCount{0};
};

class VirtualPlugin : public VirtualPluginBase {
public:
    explicit VirtualPlugin(const std::string& name,
                           PluginCapability caps = PluginCapability::None,
                           std::vector<PluginDependency> deps = {},
                           uint32_t compatVersion = 1)
        : VirtualPluginBase(makeMetadata(name, caps, std::move(deps), compatVersion)) {}

    Result<void> initialize(PluginContext&) override {
        m_initCount++;
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_shutdownCount++;
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext&) override {
        m_regCount++;
        return {};
    }

    Result<void> unregisterComponents(PluginContext&) override {
        m_unregCount++;
        return {};
    }
};

class FailingPlugin : public VirtualPlugin {
public:
    using VirtualPlugin::VirtualPlugin;

    Result<void> initialize(PluginContext&) override {
        m_state = PluginState::Error;
        return ErrorCode::PluginInitFailed;
    }

    Result<void> shutdown() noexcept override {
        return ErrorCode::PluginShutdownFailed;
    }

    Result<void> registerComponents(PluginContext&) override {
        return ErrorCode::PluginRegistrationFailed;
    }

    Result<void> unregisterComponents(PluginContext&) override {
        return ErrorCode::PluginUnregistrationFailed;
    }
};

class InitFailingPlugin : public VirtualPlugin {
public:
    using VirtualPlugin::VirtualPlugin;

    Result<void> initialize(PluginContext&) override {
        m_state = PluginState::Error;
        return ErrorCode::PluginInitFailed;
    }

    Result<void> shutdown() noexcept override {
        return {};
    }

    Result<void> registerComponents(PluginContext&) override {
        return {};
    }

    Result<void> unregisterComponents(PluginContext&) override {
        return {};
    }
};

class VirtualProtocolPlugin : public VirtualPluginBase, public IProtocolPlugin {
public:
    VirtualProtocolPlugin(const std::string& name, disc::ProtocolType proto,
                          std::vector<disc::ProtocolType> supported = {})
        : VirtualPluginBase(makeMetadata(name, PluginCapability::Protocol))
        , m_proto(proto)
        , m_supported(supported.empty() ? std::vector<disc::ProtocolType>{proto} : std::move(supported)) {}

    disc::ProtocolType protocolType() const noexcept override { return m_proto; }
    std::vector<disc::ProtocolType> supportedProtocols() const noexcept override { return m_supported; }

    Result<void> initialize(PluginContext&) override {
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext& ctx) override {
        m_regCount++;
        return {};
    }

    Result<void> unregisterComponents(PluginContext& ctx) override {
        m_unregCount++;
        return {};
    }

private:
    disc::ProtocolType m_proto;
    std::vector<disc::ProtocolType> m_supported;
};

class VirtualVendorPlugin : public VirtualPluginBase {
public:
    VirtualVendorPlugin(const std::string& name, disc::Vendor vendor)
        : VirtualPluginBase(makeMetadata(name, PluginCapability::Vendor))
        , m_vendor(vendor) {}

    disc::Vendor vendor() const noexcept override { return m_vendor; }
    std::vector<VidPidEntry> vidPidTable() const override { return m_vidPid; }
    std::vector<ChipsetInfo> knownChipsets() const override { return m_chipsets; }
    std::vector<disc::BootMode> bootModes() const noexcept override { return m_modes; }

    void addVidPid(uint16_t vid, uint16_t pid, const std::string& desc = "") {
        m_vidPid.push_back({vid, pid, desc});
    }

    void addChipset(const std::string& name, std::vector<uint32_t> ids) {
        m_chipsets.push_back({name, std::move(ids)});
    }

    void addBootMode(disc::BootMode mode) { m_modes.push_back(mode); }

    Result<void> initialize(PluginContext&) override {
        m_state = PluginState::Initialized;
        return {};
    }

    Result<void> shutdown() noexcept override {
        m_state = PluginState::Loaded;
        return {};
    }

    Result<void> registerComponents(PluginContext&) override { m_regCount++; return {}; }
    Result<void> unregisterComponents(PluginContext&) override { m_unregCount++; return {}; }

private:
    disc::Vendor m_vendor;
    std::vector<VidPidEntry> m_vidPid;
    std::vector<ChipsetInfo> m_chipsets;
    std::vector<disc::BootMode> m_modes;
};

// ============================================================
// Null Logger for Testing
// ============================================================

class NullTestLogger : public ILogger {
public:
    void log(LogLevel, std::string_view, const std::string&) override {}
    void setLevel(LogLevel) noexcept override {}
    LogLevel level() const noexcept override { return LogLevel::Debug; }
};

// ============================================================
// Test Fixture
// ============================================================

void testPluginStateEnumValues();
void testPluginCapabilityFlags();
void testPluginCapabilityHasCap();
void testPluginCapabilityCombine();
void testPluginMetadataDefaults();
void testPluginMetadataCustom();
void testPluginDependencyStruct();
void testPluginConfigDefaults();
void testPluginConfigCustom();
void testVirtualPluginCreate();
void testVirtualPluginMetadata();
void testVirtualPluginState();
void testPluginLoad();
void testPluginUnload();
void testPluginInitialize();
void testPluginShutdown();
void testPluginFullLifecycle();
void testPluginLoadDuplicate();
void testPluginLoadNull();
void testPluginInitializeTwice();
void testPluginShutdownBeforeInit();
void testPluginUnloadBeforeShutdown();
void testPluginEnableDisable();
void testPluginEnableWhileInitialized();
void testPluginDisableWhileInitialized();
void testDependencyResolveValid();
void testDependencyMissingOptional();
void testDependencyMissingRequired();
void testDependencyVersionMismatch();
void testDependencyCircular();
void testDependencyChain();
void testLoadAll();
void testInitializeAll();
void testShutdownAll();
void testShutdownAllEmpty();
void testFindPlugin();
void testFindPluginNotFound();
void testFindProtocolPlugin();
void testFindProtocolPluginNotFound();
void testFindVendorPlugin();
void testFindVendorPluginNotFound();
void testPluginState();
void testPluginStateUnloaded();
void testPluginInfo();
void testPluginInfoNotFound();
void testListPlugins();
void testListPluginsByState();
void testListEnabledPlugins();
void testPluginCount();
void testPluginCountEmpty();
void testPluginReload();
void testPluginReloadNotFound();
void testProtocolPluginCreate();
void testProtocolPluginProtocolType();
void testProtocolPluginSupportedProtocols();
void testVendorPluginCreate();
void testVendorPluginVendor();
void testVendorPluginVidPid();
void testVendorPluginChipsets();
void testVendorPluginBootModes();
void testCompatibilityVersionValid();
void testCompatibilityVersionInvalid();
void testPluginAlreadyLoaded();
void testDoubleLoad();
void testMultipleUnloads();
void testUnloadNonExistent();
void testInitializeNonExistent();
void testShutdownNonExistent();
void testEnableNonExistent();
void testDisableNonExistent();
void testPluginInitFailure();
void testPluginRegistrationFailure();
void testStressManyPlugins();
void testStressRepeatedLoadUnload();
void testStressRandomLoadOrder();
TEST_CASE("PluginTest", "[plugin]") {

    SECTION("testPluginStateEnumValues") {
        testPluginStateEnumValues();
    }
    SECTION("testPluginCapabilityFlags") {
        testPluginCapabilityFlags();
    }
    SECTION("testPluginCapabilityHasCap") {
        testPluginCapabilityHasCap();
    }
    SECTION("testPluginCapabilityCombine") {
        testPluginCapabilityCombine();
    }
    SECTION("testPluginMetadataDefaults") {
        testPluginMetadataDefaults();
    }
    SECTION("testPluginMetadataCustom") {
        testPluginMetadataCustom();
    }
    SECTION("testPluginDependencyStruct") {
        testPluginDependencyStruct();
    }
    SECTION("testPluginConfigDefaults") {
        testPluginConfigDefaults();
    }
    SECTION("testPluginConfigCustom") {
        testPluginConfigCustom();
    }
    SECTION("testVirtualPluginCreate") {
        testVirtualPluginCreate();
    }
    SECTION("testVirtualPluginMetadata") {
        testVirtualPluginMetadata();
    }
    SECTION("testVirtualPluginState") {
        testVirtualPluginState();
    }
    SECTION("testPluginLoad") {
        testPluginLoad();
    }
    SECTION("testPluginUnload") {
        testPluginUnload();
    }
    SECTION("testPluginInitialize") {
        testPluginInitialize();
    }
    SECTION("testPluginShutdown") {
        testPluginShutdown();
    }
    SECTION("testPluginFullLifecycle") {
        testPluginFullLifecycle();
    }
    SECTION("testPluginLoadDuplicate") {
        testPluginLoadDuplicate();
    }
    SECTION("testPluginLoadNull") {
        testPluginLoadNull();
    }
    SECTION("testPluginInitializeTwice") {
        testPluginInitializeTwice();
    }
    SECTION("testPluginShutdownBeforeInit") {
        testPluginShutdownBeforeInit();
    }
    SECTION("testPluginUnloadBeforeShutdown") {
        testPluginUnloadBeforeShutdown();
    }
    SECTION("testPluginEnableDisable") {
        testPluginEnableDisable();
    }
    SECTION("testPluginEnableWhileInitialized") {
        testPluginEnableWhileInitialized();
    }
    SECTION("testPluginDisableWhileInitialized") {
        testPluginDisableWhileInitialized();
    }
    SECTION("testDependencyResolveValid") {
        testDependencyResolveValid();
    }
    SECTION("testDependencyMissingOptional") {
        testDependencyMissingOptional();
    }
    SECTION("testDependencyMissingRequired") {
        testDependencyMissingRequired();
    }
    SECTION("testDependencyVersionMismatch") {
        testDependencyVersionMismatch();
    }
    SECTION("testDependencyCircular") {
        testDependencyCircular();
    }
    SECTION("testDependencyChain") {
        testDependencyChain();
    }
    SECTION("testLoadAll") {
        testLoadAll();
    }
    SECTION("testInitializeAll") {
        testInitializeAll();
    }
    SECTION("testShutdownAll") {
        testShutdownAll();
    }
    SECTION("testShutdownAllEmpty") {
        testShutdownAllEmpty();
    }
    SECTION("testFindPlugin") {
        testFindPlugin();
    }
    SECTION("testFindPluginNotFound") {
        testFindPluginNotFound();
    }
    SECTION("testFindProtocolPlugin") {
        testFindProtocolPlugin();
    }
    SECTION("testFindProtocolPluginNotFound") {
        testFindProtocolPluginNotFound();
    }
    SECTION("testFindVendorPlugin") {
        testFindVendorPlugin();
    }
    SECTION("testFindVendorPluginNotFound") {
        testFindVendorPluginNotFound();
    }
    SECTION("testPluginState") {
        testPluginState();
    }
    SECTION("testPluginStateUnloaded") {
        testPluginStateUnloaded();
    }
    SECTION("testPluginInfo") {
        testPluginInfo();
    }
    SECTION("testPluginInfoNotFound") {
        testPluginInfoNotFound();
    }
    SECTION("testListPlugins") {
        testListPlugins();
    }
    SECTION("testListPluginsByState") {
        testListPluginsByState();
    }
    SECTION("testListEnabledPlugins") {
        testListEnabledPlugins();
    }
    SECTION("testPluginCount") {
        testPluginCount();
    }
    SECTION("testPluginCountEmpty") {
        testPluginCountEmpty();
    }
    SECTION("testPluginReload") {
        testPluginReload();
    }
    SECTION("testPluginReloadNotFound") {
        testPluginReloadNotFound();
    }
    SECTION("testProtocolPluginCreate") {
        testProtocolPluginCreate();
    }
    SECTION("testProtocolPluginProtocolType") {
        testProtocolPluginProtocolType();
    }
    SECTION("testProtocolPluginSupportedProtocols") {
        testProtocolPluginSupportedProtocols();
    }
    SECTION("testVendorPluginCreate") {
        testVendorPluginCreate();
    }
    SECTION("testVendorPluginVendor") {
        testVendorPluginVendor();
    }
    SECTION("testVendorPluginVidPid") {
        testVendorPluginVidPid();
    }
    SECTION("testVendorPluginChipsets") {
        testVendorPluginChipsets();
    }
    SECTION("testVendorPluginBootModes") {
        testVendorPluginBootModes();
    }
    SECTION("testCompatibilityVersionValid") {
        testCompatibilityVersionValid();
    }
    SECTION("testCompatibilityVersionInvalid") {
        testCompatibilityVersionInvalid();
    }
    SECTION("testPluginAlreadyLoaded") {
        testPluginAlreadyLoaded();
    }
    SECTION("testDoubleLoad") {
        testDoubleLoad();
    }
    SECTION("testMultipleUnloads") {
        testMultipleUnloads();
    }
    SECTION("testUnloadNonExistent") {
        testUnloadNonExistent();
    }
    SECTION("testInitializeNonExistent") {
        testInitializeNonExistent();
    }
    SECTION("testShutdownNonExistent") {
        testShutdownNonExistent();
    }
    SECTION("testEnableNonExistent") {
        testEnableNonExistent();
    }
    SECTION("testDisableNonExistent") {
        testDisableNonExistent();
    }
    SECTION("testPluginInitFailure") {
        testPluginInitFailure();
    }
    SECTION("testPluginRegistrationFailure") {
        testPluginRegistrationFailure();
    }
    SECTION("testStressManyPlugins") {
        testStressManyPlugins();
    }
    SECTION("testStressRepeatedLoadUnload") {
        testStressRepeatedLoadUnload();
    }
    SECTION("testStressRandomLoadOrder") {
        testStressRandomLoadOrder();
    }
}

// ============================================================
// PluginTypes Tests
// ============================================================

void testPluginStateEnumValues() {
    REQUIRE(static_cast<uint32_t>(PluginState::Unloaded) == 0u);
    REQUIRE(static_cast<uint32_t>(PluginState::Loaded) == 1u);
    REQUIRE(static_cast<uint32_t>(PluginState::Initialized) == 2u);
    REQUIRE(static_cast<uint32_t>(PluginState::Enabled) == 3u);
    REQUIRE(static_cast<uint32_t>(PluginState::Disabled) == 4u);
    REQUIRE(static_cast<uint32_t>(PluginState::Error) == 5u);
}

void testPluginCapabilityFlags() {
    REQUIRE(static_cast<uint32_t>(PluginCapability::None) == 0u);
    REQUIRE(static_cast<uint32_t>(PluginCapability::Protocol) & (1u << 0));
    REQUIRE(static_cast<uint32_t>(PluginCapability::Vendor) & (1u << 1));
    REQUIRE(static_cast<uint32_t>(PluginCapability::Discovery) & (1u << 2));
}

void testPluginCapabilityHasCap() {
    auto caps = PluginCapability::Protocol | PluginCapability::Vendor;
    REQUIRE(hasCapability(caps, PluginCapability::Protocol));
    REQUIRE(hasCapability(caps, PluginCapability::Vendor));
    REQUIRE(!hasCapability(caps, PluginCapability::Discovery));
    REQUIRE(!hasCapability(PluginCapability::None, PluginCapability::Protocol));
}

void testPluginCapabilityCombine() {
    auto caps = PluginCapability::Protocol | PluginCapability::Vendor;
    REQUIRE(hasCapability(caps, PluginCapability::Protocol));
    REQUIRE(hasCapability(caps, PluginCapability::Vendor));
}

void testPluginMetadataDefaults() {
    PluginMetadata meta;
    REQUIRE(meta.name.empty());
    REQUIRE(meta.version.empty());
    REQUIRE(static_cast<uint32_t>(meta.vendor) == 0u);
    REQUIRE(meta.capabilities == PluginCapability::None);
    REQUIRE(meta.priority == 100u);
    REQUIRE(meta.compatibilityVersion == 1u);
    REQUIRE(meta.dependencies.empty());
}

void testPluginMetadataCustom() {
    PluginDependency dep;
    dep.pluginName = "BasePlugin";
    dep.minVersion = 1;

    PluginMetadata meta;
    meta.name = "TestPlugin";
    meta.version = "2.0";
    meta.vendor = disc::Vendor::Qualcomm;
    meta.author = "test";
    meta.capabilities = PluginCapability::Protocol;
    meta.priority = 50;
    meta.uuid = "abc-123";
    meta.compatibilityVersion = 1;
    meta.supportedProtocols = {disc::ProtocolType::Sahara};
    meta.dependencies = {dep};

    REQUIRE(meta.name == std::string("TestPlugin"));
    REQUIRE(static_cast<uint32_t>(meta.vendor) == 1u);
    REQUIRE(meta.capabilities == PluginCapability::Protocol);
    REQUIRE(meta.priority == 50u);
    REQUIRE(meta.dependencies.size() == std::size_t(1));
    REQUIRE(meta.supportedProtocols.size() == std::size_t(1));
    REQUIRE(meta.supportedProtocols[0] == disc::ProtocolType::Sahara);
}

void testPluginDependencyStruct() {
    PluginDependency dep;
    REQUIRE(dep.pluginName.empty());
    REQUIRE(dep.minVersion == 1u);
    REQUIRE(dep.maxVersion == UINT32_MAX);
    REQUIRE(!dep.optional);

    dep.pluginName = "Base";
    dep.minVersion = 2;
    dep.maxVersion = 5;
    dep.optional = true;
    REQUIRE(dep.pluginName == std::string("Base"));
    REQUIRE(dep.minVersion == 2u);
    REQUIRE(dep.maxVersion == 5u);
    REQUIRE(dep.optional);
}

void testPluginConfigDefaults() {
    PluginConfig config;
    REQUIRE(config.enabled);
    REQUIRE(config.priority == 100u);
    REQUIRE(config.autoLoad);
    REQUIRE(!config.compatibilityMode);
    REQUIRE(config.vendorSettings.empty());
}

void testPluginConfigCustom() {
    PluginConfig config;
    config.enabled = false;
    config.priority = 10;
    config.autoLoad = false;
    config.compatibilityMode = true;
    config.vendorSettings["key"] = "value";

    REQUIRE(!config.enabled);
    REQUIRE(config.priority == 10u);
    REQUIRE(!config.autoLoad);
    REQUIRE(config.compatibilityMode);
    REQUIRE(config.vendorSettings["key"] == std::string("value"));
}

// ============================================================
// VirtualPlugin Tests
// ============================================================

void testVirtualPluginCreate() {
    auto plugin = std::make_unique<VirtualPlugin>("Test");
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->metadata().name == std::string("Test"));
    REQUIRE(plugin->state() == PluginState::Unloaded);
}

void testVirtualPluginMetadata() {
    auto plugin = std::make_unique<VirtualPlugin>("MetaTest", PluginCapability::Protocol);
    auto meta = plugin->metadata();
    REQUIRE(meta.name == std::string("MetaTest"));
    REQUIRE(meta.capabilities == PluginCapability::Protocol);
    REQUIRE(meta.compatibilityVersion == 1u);
}

void testVirtualPluginState() {
    auto plugin = std::make_unique<VirtualPlugin>("StateTest");
    REQUIRE(plugin->state() == PluginState::Unloaded);
    plugin->setEnabled(false);
    REQUIRE(!plugin->isEnabled());
    plugin->setEnabled(true);
    REQUIRE(plugin->isEnabled());
}

// ============================================================
// Lifecycle Tests
// ============================================================

void testPluginLoad() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("LoadTest");
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(1));
    REQUIRE(mgr.pluginState("LoadTest") == PluginState::Loaded);
}

void testPluginUnload() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("UnloadTest");
    mgr.load(std::move(plugin));
    auto result = mgr.unload("UnloadTest");
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(0));
}

void testPluginInitialize() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("InitTest");
    mgr.load(std::move(plugin));
    auto result = mgr.initialize("InitTest");
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginState("InitTest") == PluginState::Enabled);
}

void testPluginShutdown() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("ShutdownTest");
    mgr.load(std::move(plugin));
    mgr.initialize("ShutdownTest");
    auto result = mgr.shutdown("ShutdownTest");
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginState("ShutdownTest") == PluginState::Loaded);
}

void testPluginFullLifecycle() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("FullLifecycle");
    auto* rawPtr = plugin.get();
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isOk());
    REQUIRE(rawPtr->initCount() == 0);
    REQUIRE(rawPtr->regCount() == 0);

    mgr.initialize("FullLifecycle");
    REQUIRE(rawPtr->initCount() == 1);
    REQUIRE(rawPtr->regCount() == 1);

    mgr.shutdown("FullLifecycle");
    REQUIRE(rawPtr->shutdownCount() == 1);
    REQUIRE(rawPtr->unregCount() == 1);

    mgr.unload("FullLifecycle");
    REQUIRE(mgr.pluginCount() == std::size_t(0));
}

void testPluginLoadDuplicate() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto p1 = std::make_unique<VirtualPlugin>("DupTest");
    REQUIRE(mgr.load(std::move(p1)).isOk());

    auto p2 = std::make_unique<VirtualPlugin>("DupTest");
    auto result = mgr.load(std::move(p2));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginDuplicate);
}

void testPluginLoadNull() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::unique_ptr<IPlugin> nullPtr;
    auto result = mgr.load(std::move(nullPtr));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidArgument);
}

void testPluginInitializeTwice() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("InitTwice");
    mgr.load(std::move(plugin));
    mgr.initialize("InitTwice");
    auto result = mgr.initialize("InitTwice");
    REQUIRE(result.isOk());
}

void testPluginShutdownBeforeInit() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("ShutdownBeforeInit");
    mgr.load(std::move(plugin));
    auto result = mgr.shutdown("ShutdownBeforeInit");
    REQUIRE(result.isOk());
}

void testPluginUnloadBeforeShutdown() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("UnloadBeforeShutdown");
    mgr.load(std::move(plugin));
    mgr.initialize("UnloadBeforeShutdown");

    REQUIRE(mgr.pluginState("UnloadBeforeShutdown") == PluginState::Enabled);

    auto result = mgr.unload("UnloadBeforeShutdown");
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(0));
}

void testPluginEnableDisable() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("EnableDisable");
    mgr.load(std::move(plugin));
    mgr.initialize("EnableDisable");

    mgr.disable("EnableDisable");
    REQUIRE(mgr.pluginState("EnableDisable") == PluginState::Disabled);

    mgr.enable("EnableDisable");
    REQUIRE(mgr.pluginState("EnableDisable") == PluginState::Enabled);
}

void testPluginEnableWhileInitialized() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("EnableInit");
    mgr.load(std::move(plugin));
    mgr.initialize("EnableInit");
    REQUIRE(mgr.pluginState("EnableInit") == PluginState::Enabled);
}

void testPluginDisableWhileInitialized() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("DisableInit");
    mgr.load(std::move(plugin));
    mgr.initialize("DisableInit");
    mgr.disable("DisableInit");
    REQUIRE(mgr.pluginState("DisableInit") == PluginState::Disabled);
}

// ============================================================
// Dependency Resolution Tests
// ============================================================

void testDependencyResolveValid() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto base = std::make_unique<VirtualPlugin>("Base");
    mgr.load(std::move(base));

    PluginDependency dep;
    dep.pluginName = "Base";
    dep.minVersion = 1;
    dep.maxVersion = 1;
    auto derived = std::make_unique<VirtualPlugin>("Derived", PluginCapability::None,
        std::vector<PluginDependency>{dep});
    auto result = mgr.load(std::move(derived));
    REQUIRE(result.isOk());
}

void testDependencyMissingOptional() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    PluginDependency dep;
    dep.pluginName = "OptionalBase";
    dep.optional = true;
    auto plugin = std::make_unique<VirtualPlugin>("WithOptional", PluginCapability::None,
        std::vector<PluginDependency>{dep});
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isOk());
}

void testDependencyMissingRequired() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    PluginDependency dep;
    dep.pluginName = "MissingBase";
    auto plugin = std::make_unique<VirtualPlugin>("WithMissing", PluginCapability::None,
        std::vector<PluginDependency>{dep});
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginDependencyMissing);
}

void testDependencyVersionMismatch() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto base = std::make_unique<VirtualPlugin>("VersionBase");
    mgr.load(std::move(base));

    PluginDependency dep;
    dep.pluginName = "VersionBase";
    dep.minVersion = 5;
    dep.maxVersion = 10;
    auto derived = std::make_unique<VirtualPlugin>("VersionDerived", PluginCapability::None,
        std::vector<PluginDependency>{dep});
    auto result = mgr.load(std::move(derived));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginVersionMismatch);
}

void testDependencyCircular() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    PluginDependency depA; depA.pluginName = "PluginB"; depA.optional = true;
    PluginDependency depB; depB.pluginName = "PluginA"; depB.optional = true;

    auto pluginA = std::make_unique<VirtualPlugin>("PluginA", PluginCapability::None,
        std::vector<PluginDependency>{depA});
    auto pluginB = std::make_unique<VirtualPlugin>("PluginB", PluginCapability::None,
        std::vector<PluginDependency>{depB});

    auto resultA = mgr.load(std::move(pluginA));
    REQUIRE(resultA.isOk());

    auto result = mgr.load(std::move(pluginB));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginCircularDependency);
}

void testDependencyChain() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    PluginDependency depB; depB.pluginName = "PluginB";
    PluginDependency depC; depC.pluginName = "PluginC";

    auto pluginC = std::make_unique<VirtualPlugin>("PluginC");
    auto pluginB = std::make_unique<VirtualPlugin>("PluginB", PluginCapability::None,
        std::vector<PluginDependency>{depC});
    auto pluginA = std::make_unique<VirtualPlugin>("PluginA", PluginCapability::None,
        std::vector<PluginDependency>{depB});

    REQUIRE(mgr.load(std::move(pluginC)).isOk());
    REQUIRE(mgr.load(std::move(pluginB)).isOk());
    REQUIRE(mgr.load(std::move(pluginA)).isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(3));
}

// ============================================================
// Batch Operations
// ============================================================

void testLoadAll() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("Batch1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("Batch2"));
    plugins.push_back(std::make_unique<VirtualPlugin>("Batch3"));

    auto result = mgr.loadAll(std::move(plugins));
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(3));
}

void testInitializeAll() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("InitAll1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("InitAll2"));
    mgr.loadAll(std::move(plugins));

    auto result = mgr.initializeAll();
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginState("InitAll1") == PluginState::Enabled);
    REQUIRE(mgr.pluginState("InitAll2") == PluginState::Enabled);
}

void testShutdownAll() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("ShutdownAll1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("ShutdownAll2"));
    mgr.loadAll(std::move(plugins));
    mgr.initializeAll();

    auto result = mgr.shutdownAll();
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginState("ShutdownAll1") == PluginState::Loaded);
    REQUIRE(mgr.pluginState("ShutdownAll2") == PluginState::Loaded);
}

void testShutdownAllEmpty() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto result = mgr.shutdownAll();
    REQUIRE(result.isOk());
}

// ============================================================
// Query Tests
// ============================================================

void testFindPlugin() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("FindMe");
    mgr.load(std::move(plugin));

    auto* found = mgr.findPlugin("FindMe");
    REQUIRE(found != nullptr);
    REQUIRE(found->metadata().name == std::string("FindMe"));
}

void testFindPluginNotFound() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto* found = mgr.findPlugin("NonExistent");
    REQUIRE(found == nullptr);
}

void testFindProtocolPlugin() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::unique_ptr<IPlugin> plugin = std::make_unique<VirtualProtocolPlugin>("ProtoPlugin", disc::ProtocolType::Sahara);
    mgr.load(std::move(plugin));
    mgr.initialize("ProtoPlugin");

    auto* found = mgr.findProtocolPlugin(disc::ProtocolType::Sahara);
    REQUIRE(found != nullptr);
    REQUIRE(found->protocolType() == disc::ProtocolType::Sahara);
}

void testFindProtocolPluginNotFound() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto* found = mgr.findProtocolPlugin(disc::ProtocolType::Firehose);
    REQUIRE(found == nullptr);
}

void testFindVendorPlugin() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::unique_ptr<IPlugin> plugin = std::make_unique<VirtualVendorPlugin>("VendorPlugin", disc::Vendor::Qualcomm);
    mgr.load(std::move(plugin));

    auto* found = mgr.findVendorPlugin(disc::Vendor::Qualcomm);
    REQUIRE(found != nullptr);
    REQUIRE(found->vendor() == disc::Vendor::Qualcomm);
}

void testFindVendorPluginNotFound() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto* found = mgr.findVendorPlugin(disc::Vendor::MediaTek);
    REQUIRE(found == nullptr);
}

void testPluginState() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("StateQuery");
    mgr.load(std::move(plugin));
    REQUIRE(mgr.pluginState("StateQuery") == PluginState::Loaded);

    mgr.initialize("StateQuery");
    REQUIRE(mgr.pluginState("StateQuery") == PluginState::Enabled);

    mgr.disable("StateQuery");
    REQUIRE(mgr.pluginState("StateQuery") == PluginState::Disabled);
}

void testPluginStateUnloaded() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    REQUIRE(mgr.pluginState("NonExistent") == PluginState::Unloaded);
}

void testPluginInfo() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("InfoTest");
    mgr.load(std::move(plugin));

    auto* info = mgr.pluginInfo("InfoTest");
    REQUIRE(info != nullptr);
    REQUIRE(info->state == PluginState::Loaded);
    REQUIRE(info->config.enabled);
}

void testPluginInfoNotFound() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto* info = mgr.pluginInfo("NonExistent");
    REQUIRE(info == nullptr);
}

void testListPlugins() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("List1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("List2"));
    plugins.push_back(std::make_unique<VirtualPlugin>("List3"));
    mgr.loadAll(std::move(plugins));

    auto names = mgr.listPlugins();
    REQUIRE(names.size() == std::size_t(3));
    REQUIRE(std::find(names.begin(), names.end(), "List1") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "List2") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "List3") != names.end());
}

void testListPluginsByState() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> p1;
    p1.push_back(std::make_unique<VirtualPlugin>("StateA"));
    p1.push_back(std::make_unique<VirtualPlugin>("StateB"));
    mgr.loadAll(std::move(p1));
    mgr.initialize("StateA");

    auto loaded = mgr.listPluginsByState(PluginState::Loaded);
    auto enabled = mgr.listPluginsByState(PluginState::Enabled);

    REQUIRE(loaded.size() == std::size_t(1));
    REQUIRE(enabled.size() == std::size_t(1));
    REQUIRE(enabled[0] == std::string("StateA"));
}

void testListEnabledPlugins() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("Enabled1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("Enabled2"));
    mgr.loadAll(std::move(plugins));
    mgr.initializeAll();
    mgr.disable("Enabled2");

    auto enabled = mgr.listEnabledPlugins();
    REQUIRE(enabled.size() == std::size_t(1));
    REQUIRE(enabled[0] == std::string("Enabled1"));
}

void testPluginCount() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    std::vector<std::unique_ptr<IPlugin>> plugins;
    plugins.push_back(std::make_unique<VirtualPlugin>("Count1"));
    plugins.push_back(std::make_unique<VirtualPlugin>("Count2"));
    mgr.loadAll(std::move(plugins));
    REQUIRE(mgr.pluginCount() == std::size_t(2));
}

void testPluginCountEmpty() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    REQUIRE(mgr.pluginCount() == std::size_t(0));
}

// ============================================================
// Reload Tests
// ============================================================

void testPluginReload() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("ReloadTest");
    auto* rawPtr = plugin.get();
    mgr.load(std::move(plugin));
    mgr.initialize("ReloadTest");

    REQUIRE(rawPtr->initCount() == 1);

    auto result = mgr.reload("ReloadTest");
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(1));
    REQUIRE(mgr.pluginState("ReloadTest") == PluginState::Enabled);
}

void testPluginReloadNotFound() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto result = mgr.reload("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

// ============================================================
// ProtocolPlugin Tests
// ============================================================

void testProtocolPluginCreate() {
    auto plugin = std::make_unique<VirtualProtocolPlugin>("Proto", disc::ProtocolType::Sahara);
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->protocolType() == disc::ProtocolType::Sahara);
}

void testProtocolPluginProtocolType() {
    auto plugin = std::make_unique<VirtualProtocolPlugin>("TypeTest", disc::ProtocolType::Firehose);
    REQUIRE(plugin->protocolType() == disc::ProtocolType::Firehose);
    REQUIRE(plugin->metadata().capabilities == PluginCapability::Protocol);
}

void testProtocolPluginSupportedProtocols() {
    auto plugin = std::make_unique<VirtualProtocolPlugin>(
        "MultiProto", disc::ProtocolType::Sahara,
        std::vector<disc::ProtocolType>{disc::ProtocolType::Sahara, disc::ProtocolType::Firehose});

    auto supported = plugin->supportedProtocols();
    REQUIRE(supported.size() == std::size_t(2));
    REQUIRE(supported[0] == disc::ProtocolType::Sahara);
    REQUIRE(supported[1] == disc::ProtocolType::Firehose);
}

// ============================================================
// VendorPlugin Tests
// ============================================================

void testVendorPluginCreate() {
    auto plugin = std::make_unique<VirtualVendorPlugin>("Vendor", disc::Vendor::Qualcomm);
    REQUIRE(plugin != nullptr);
    REQUIRE(plugin->vendor() == disc::Vendor::Qualcomm);
}

void testVendorPluginVendor() {
    auto plugin = std::make_unique<VirtualVendorPlugin>("VendorTest", disc::Vendor::MediaTek);
    REQUIRE(plugin->vendor() == disc::Vendor::MediaTek);
}

void testVendorPluginVidPid() {
    auto plugin = std::make_unique<VirtualVendorPlugin>("VidPidTest", disc::Vendor::Qualcomm);
    plugin->addVidPid(0x05C6, 0x9008, "QC EDL");
    plugin->addVidPid(0x05C6, 0x900B, "QC Firehose");

    auto table = plugin->vidPidTable();
    REQUIRE(table.size() == std::size_t(2));
    REQUIRE(table[0].vid == 0x05C6);
    REQUIRE(table[0].pid == 0x9008);
}

void testVendorPluginChipsets() {
    auto plugin = std::make_unique<VirtualVendorPlugin>("ChipsetTest", disc::Vendor::Qualcomm);
    plugin->addChipset("SM8450", {8450, 8451});
    plugin->addChipset("SM8550", {8550});

    auto chipsets = plugin->knownChipsets();
    REQUIRE(chipsets.size() == std::size_t(2));
    REQUIRE(chipsets[0].name == std::string("SM8450"));
    REQUIRE(chipsets[0].chipsetIds.size() == std::size_t(2));
}

void testVendorPluginBootModes() {
    auto plugin = std::make_unique<VirtualVendorPlugin>("ModeTest", disc::Vendor::Qualcomm);
    plugin->addBootMode(disc::BootMode::EDL);
    plugin->addBootMode(disc::BootMode::Firehose);

    auto modes = plugin->bootModes();
    REQUIRE(modes.size() == std::size_t(2));
    REQUIRE(modes[0] == disc::BootMode::EDL);
}

// ============================================================
// Compatibility Tests
// ============================================================

void testCompatibilityVersionValid() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("CompV1", PluginCapability::None,
        std::vector<PluginDependency>{}, 1);
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isOk());
}

void testCompatibilityVersionInvalid() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("CompBad", PluginCapability::None,
        std::vector<PluginDependency>{}, 99);
    auto result = mgr.load(std::move(plugin));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginIncompatible);
}

// ============================================================
// Edge Case Tests
// ============================================================

void testPluginAlreadyLoaded() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto p1 = std::make_unique<VirtualPlugin>("AlreadyLoaded");
    mgr.load(std::move(p1));
    auto p2 = std::make_unique<VirtualPlugin>("AlreadyLoaded");
    auto result = mgr.load(std::move(p2));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginDuplicate);
}

void testDoubleLoad() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto p1 = std::make_unique<VirtualPlugin>("DoubleLoad");
    REQUIRE(mgr.load(std::move(p1)).isOk());
    auto p2 = std::make_unique<VirtualPlugin>("DoubleLoad");
    auto result = mgr.load(std::move(p2));
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginDuplicate);
}

void testMultipleUnloads() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<VirtualPlugin>("MultiUnload");
    mgr.load(std::move(plugin));
    REQUIRE(mgr.unload("MultiUnload").isOk());
    auto result = mgr.unload("MultiUnload");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testUnloadNonExistent() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    auto result = mgr.unload("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testInitializeNonExistent() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    auto result = mgr.initialize("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testShutdownNonExistent() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    auto result = mgr.shutdown("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testEnableNonExistent() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    auto result = mgr.enable("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

void testDisableNonExistent() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);
    auto result = mgr.disable("NonExistent");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginNotFound);
}

// ============================================================
// Failure Injection Tests
// ============================================================

void testPluginInitFailure() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<InitFailingPlugin>("FailInit", PluginCapability::None);
    mgr.load(std::move(plugin));
    auto result = mgr.initialize("FailInit");
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::PluginInitFailed);
    REQUIRE(mgr.pluginState("FailInit") == PluginState::Error);
}

void testPluginRegistrationFailure() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    auto plugin = std::make_unique<FailingPlugin>("FailReg", PluginCapability::None);
    mgr.load(std::move(plugin));
    auto result = mgr.initialize("FailReg");
    REQUIRE(result.isError());
    REQUIRE(mgr.pluginState("FailReg") == PluginState::Error);
}

// ============================================================
// Stress Tests
// ============================================================

void testStressManyPlugins() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    const int count = 50;
    std::vector<std::unique_ptr<IPlugin>> plugins;
    for (int i = 0; i < count; ++i) {
        plugins.push_back(std::make_unique<VirtualPlugin>("Stress" + std::to_string(i)));
    }

    auto result = mgr.loadAll(std::move(plugins));
    REQUIRE(result.isOk());
    REQUIRE(mgr.pluginCount() == static_cast<std::size_t>(count));

    result = mgr.initializeAll();
    REQUIRE(result.isOk());

    result = mgr.shutdownAll();
    REQUIRE(result.isOk());
}

void testStressRepeatedLoadUnload() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    for (int i = 0; i < 20; ++i) {
        auto plugin = std::make_unique<VirtualPlugin>("Cycle" + std::to_string(i));
        REQUIRE(mgr.load(std::move(plugin)).isOk());
    }

    for (int i = 0; i < 20; ++i) {
        REQUIRE(mgr.initialize("Cycle" + std::to_string(i)).isOk());
    }

    for (int i = 0; i < 20; ++i) {
        REQUIRE(mgr.shutdown("Cycle" + std::to_string(i)).isOk());
    }

    for (int i = 0; i < 20; ++i) {
        REQUIRE(mgr.unload("Cycle" + std::to_string(i)).isOk());
    }

    REQUIRE(mgr.pluginCount() == std::size_t(0));
}

void testStressRandomLoadOrder() {
    disc::ProtocolRegistry registry;
    PluginContext context(registry);
    PluginManager mgr(context);

    PluginDependency depB; depB.pluginName = "ChainB";
    PluginDependency depC; depC.pluginName = "ChainC";
    PluginDependency depD; depD.pluginName = "ChainD";

    auto d = std::make_unique<VirtualPlugin>("ChainD");
    auto c = std::make_unique<VirtualPlugin>("ChainC", PluginCapability::None,
        std::vector<PluginDependency>{depD});
    auto b = std::make_unique<VirtualPlugin>("ChainB", PluginCapability::None,
        std::vector<PluginDependency>{depC});
    auto a = std::make_unique<VirtualPlugin>("ChainA", PluginCapability::None,
        std::vector<PluginDependency>{depB});

    REQUIRE(mgr.load(std::move(d)).isOk());
    REQUIRE(mgr.load(std::move(c)).isOk());
    REQUIRE(mgr.load(std::move(b)).isOk());
    REQUIRE(mgr.load(std::move(a)).isOk());
    REQUIRE(mgr.pluginCount() == std::size_t(4));
    REQUIRE(mgr.initializeAll().isOk());
    REQUIRE(mgr.shutdownAll().isOk());
}

