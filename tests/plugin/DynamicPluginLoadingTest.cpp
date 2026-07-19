#include <catch2/catch_test_macros.hpp>

#include <mbootcore/plugin/PluginTypes.hpp>
#include <mbootcore/plugin/IPlugin.hpp>
#include <mbootcore/plugin/PluginManager.hpp>
#include <mbootcore/plugin/PluginContext.hpp>
#include <mbootcore/plugin/PluginABI.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/domain/Error.hpp>
#include <mbootcore/domain/ILogger.hpp>

#include <memory>
#include <string>
#include <cstdint>

using namespace mbootcore;
using namespace mbootcore::plugin;
namespace disc = mbootcore::discovery;

#ifdef _WIN32
constexpr const char* TestPluginPath = MBOOTCORE_TEST_PLUGIN_DIR "/TestDynamicPlugin.dll";
#elif defined(__APPLE__)
constexpr const char* TestPluginPath = MBOOTCORE_TEST_PLUGIN_DIR "/TestDynamicPlugin.dylib";
#else
constexpr const char* TestPluginPath = MBOOTCORE_TEST_PLUGIN_DIR "/TestDynamicPlugin.so";
#endif

TEST_CASE("DynamicPluginLoading", "[plugin][dynamic]") {

    SECTION("loadFromFile_NonExistent_ReturnsError") {
        disc::ProtocolRegistry registry;
        PluginContext context(registry);
        PluginManager mgr(context);

        auto result = mgr.loadFromFile("/nonexistent/plugin.so");
        REQUIRE(result.isError());
    }

    SECTION("loadFromFile_ValidPlugin_Succeeds") {
        disc::ProtocolRegistry registry;
        PluginContext context(registry);
        PluginManager mgr(context);

        auto result = mgr.loadFromFile(TestPluginPath);
        REQUIRE(result.isOk());
        REQUIRE(mgr.pluginCount() == 1);
        REQUIRE(mgr.pluginState("TestDynamicPlugin") == PluginState::Loaded);
    }

    SECTION("loadFromFile_Initialize_FullLifecycle") {
        disc::ProtocolRegistry registry;
        PluginContext context(registry);
        PluginManager mgr(context);

        auto loadResult = mgr.loadFromFile(TestPluginPath);
        REQUIRE(loadResult.isOk());

        auto initResult = mgr.initialize("TestDynamicPlugin");
        REQUIRE(initResult.isOk());
        REQUIRE(mgr.pluginState("TestDynamicPlugin") == PluginState::Enabled);

        auto shutdownResult = mgr.shutdown("TestDynamicPlugin");
        REQUIRE(shutdownResult.isOk());
        REQUIRE(mgr.pluginState("TestDynamicPlugin") == PluginState::Loaded);

        auto unloadResult = mgr.unload("TestDynamicPlugin");
        REQUIRE(unloadResult.isOk());
        REQUIRE(mgr.pluginCount() == 0);
    }

    SECTION("loadFromFile_Duplicate_ReturnsError") {
        disc::ProtocolRegistry registry;
        PluginContext context(registry);
        PluginManager mgr(context);

        auto r1 = mgr.loadFromFile(TestPluginPath);
        REQUIRE(r1.isOk());

        auto r2 = mgr.loadFromFile(TestPluginPath);
        REQUIRE(r2.isError());
        REQUIRE(r2.error() == ErrorCode::PluginDuplicate);
    }

    SECTION("loadFromFile_FindPlugin_Works") {
        disc::ProtocolRegistry registry;
        PluginContext context(registry);
        PluginManager mgr(context);

        REQUIRE(mgr.loadFromFile(TestPluginPath).isOk());

        auto* plugin = mgr.findPlugin("TestDynamicPlugin");
        REQUIRE(plugin != nullptr);
        REQUIRE(plugin->metadata().name == std::string("TestDynamicPlugin"));
        REQUIRE(plugin->metadata().apiVersion == PluginABIVersion);
    }

    SECTION("ABIVersion_Matches") {
        REQUIRE(PluginABIVersion == 1u);
    }
}
