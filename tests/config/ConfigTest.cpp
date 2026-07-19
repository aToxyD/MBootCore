#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>

#include <mbootcore/config/ConfigTypes.hpp>
#include <mbootcore/config/ConfigManager.hpp>
#include <mbootcore/domain/Error.hpp>

using namespace mbootcore;
using namespace mbootcore::config;

namespace fs = std::filesystem;

static std::string makeTempDir() {
    for (int i = 0; i < 200; ++i) {
        auto p = fs::temp_directory_path() / ("mboot_cfg_test_" + std::to_string(i));
        if (!fs::exists(p)) {
            fs::create_directories(p);
            return p.string();
        }
    }
    return (fs::temp_directory_path() / "mboot_cfg_test_fallback").string();
}

TEST_CASE("ConfigTest", "[config]") {

    // ---- Config Enums ----
    SECTION("testConfigSourceEnum") {
    }
    SECTION("testConfigStatusEnum") {

    // ---- Config Defaults ----
    }
    SECTION("testRuntimeConfigDefaults") {
    }
    SECTION("testTransportConfigDefaults") {
    }
    SECTION("testWorkflowConfigDefaults") {
    }
    SECTION("testJobConfigDefaults") {
    }
    SECTION("testDSPConfigDefaults") {
    }
    SECTION("testPluginConfigDefaults") {
    }
    SECTION("testVendorConfigDefaults") {
    }
    SECTION("testGUIConfigDefaults") {
    }
    SECTION("testCLIConfigDefaults") {
    }
    SECTION("testFullConfigDefaults") {

    // ---- ConfigManager Basics ----
    }
    SECTION("testLoadReturnsDefaultsInitially") {
    }
    SECTION("testSetGetRuntimeConfig") {
    }
    SECTION("testSetGetTransportConfig") {
    }
    SECTION("testSetGetWorkflowConfig") {
    }
    SECTION("testSetGetJobConfig") {
    }
    SECTION("testSetGetDSPConfig") {
    }
    SECTION("testSetGetPluginConfig") {
    }
    SECTION("testSetGetVendorConfig") {
    }
    SECTION("testSetGetGUIConfig") {
    }
    SECTION("testSetGetCLIConfig") {
    }
    SECTION("testCustomOptions") {
    }
    SECTION("testCustomOptionNotFound") {
    }
    SECTION("testImportExportJsonRoundTrip") {
    }
    SECTION("testDefaults") {

    // ---- Config Validation ----
    }
    SECTION("testValidateValidConfig") {
    }
    SECTION("testValidateInvalidTimeout") {
    }
    SECTION("testValidateInvalidThreadPool") {
    }
    SECTION("testInvalidTransportTimeout") {
    }
    SECTION("testInvalidWorkflowRetries") {
    }
    SECTION("testInvalidJobMaxParallel") {
    }
    SECTION("testInvalidGUIWidth") {
    }
    SECTION("testInvalidCLIFormat") {
    }
    SECTION("testInvalidDSPSize") {

    // ---- Version Migration ----
    }
    SECTION("testMigrateV1ToV2") {

    // ---- Persistence ----
    }
    SECTION("testFilePersistence") {
    }
    SECTION("testSaveWithoutPathDoesNotFail") {
    }
}

void testConfigSourceEnum() {
    REQUIRE(static_cast<uint32_t>(ConfigSource::Default) == 0u);
    REQUIRE(static_cast<uint32_t>(ConfigSource::File) == 1u);
    REQUIRE(static_cast<uint32_t>(ConfigSource::Environment) == 2u);
    REQUIRE(static_cast<uint32_t>(ConfigSource::API) == 3u);
    REQUIRE(static_cast<uint32_t>(ConfigSource::CLI) == 4u);
}

void testConfigStatusEnum() {
    REQUIRE(static_cast<uint32_t>(ConfigStatus::Valid) == 0u);
    REQUIRE(static_cast<uint32_t>(ConfigStatus::Invalid) == 1u);
    REQUIRE(static_cast<uint32_t>(ConfigStatus::Missing) == 2u);
    REQUIRE(static_cast<uint32_t>(ConfigStatus::Deprecated) == 3u);
    REQUIRE(static_cast<uint32_t>(ConfigStatus::Migrated) == 4u);
}

void testRuntimeConfigDefaults() {
    RuntimeConfig cfg;
    REQUIRE(cfg.threadPoolSize == 4u);
    REQUIRE(cfg.operationTimeoutMs == 30000u);
    REQUIRE(cfg.enableLogging);
    REQUIRE(!cfg.enableTelemetry);
    REQUIRE(!cfg.enableProfiling);
    REQUIRE(cfg.logLevel == std::string("info"));
    REQUIRE(cfg.tempDirectory == std::string("/tmp/mbootcore"));
}

void testTransportConfigDefaults() {
    TransportConfig cfg;
    REQUIRE(cfg.usbTimeoutMs == 5000u);
    REQUIRE(cfg.serialTimeoutMs == 5000u);
    REQUIRE(cfg.tcpTimeoutMs == 10000u);
    REQUIRE(cfg.maxRetries == 3u);
    REQUIRE(cfg.enableKeepAlive);
    REQUIRE(cfg.keepAliveIntervalMs == 1000u);
    REQUIRE(cfg.bufferSize == 65536u);
}

void testWorkflowConfigDefaults() {
    WorkflowConfig cfg;
    REQUIRE(cfg.maxRetries == 3u);
    REQUIRE(cfg.retryDelayMs == 1000u);
    REQUIRE(cfg.enableRollback);
    REQUIRE(cfg.enableCheckpoints);
    REQUIRE(cfg.enableValidation);
    REQUIRE(cfg.maxParallelStages == 4u);
}

void testJobConfigDefaults() {
    JobConfig cfg;
    REQUIRE(cfg.maxRetries == 3u);
    REQUIRE(cfg.retryDelayMs == 1000u);
    REQUIRE(cfg.maxParallel == 4u);
    REQUIRE(cfg.enableRecovery);
    REQUIRE(cfg.enableHistory);
    REQUIRE(cfg.historyLimit == 1000u);
}

void testDSPConfigDefaults() {
    DSPConfig cfg;
    REQUIRE(cfg.enableVerification);
    REQUIRE(cfg.enableCaching);
    REQUIRE(cfg.cacheSizeMb == 256u);
    REQUIRE(cfg.repositoryPath == std::string("/etc/mbootcore/dsp"));
    REQUIRE(cfg.trustedVendors.empty());
}

void testPluginConfigDefaults() {
    PluginConfig cfg;
    REQUIRE(cfg.enableVerification);
    REQUIRE(!cfg.enableHotReload);
    REQUIRE(cfg.maxPlugins == 100u);
    REQUIRE(cfg.pluginPaths.empty());
    REQUIRE(cfg.blacklist.empty());
}

void testVendorConfigDefaults() {
    VendorConfig cfg;
    REQUIRE(cfg.enableAutoDetect);
    REQUIRE(cfg.preferredVendors.empty());
    REQUIRE(cfg.vendorOptions.empty());
}

void testGUIConfigDefaults() {
    GUIConfig cfg;
    REQUIRE(cfg.enableAnimations);
    REQUIRE(cfg.enableTrayIcon);
    REQUIRE(cfg.theme == std::string("dark"));
    REQUIRE(cfg.language == std::string("auto"));
    REQUIRE(cfg.windowWidth == 1280u);
    REQUIRE(cfg.windowHeight == 720u);
    REQUIRE(!cfg.maximizeOnStart);
}

void testCLIConfigDefaults() {
    CLIConfig cfg;
    REQUIRE(cfg.enableColor);
    REQUIRE(cfg.enableProgress);
    REQUIRE(cfg.outputFormat == std::string("text"));
    REQUIRE(cfg.verbosity == 1u);
}

void testFullConfigDefaults() {
    FullConfig cfg;
    REQUIRE(cfg.source == ConfigSource::Default);
    REQUIRE(cfg.customOptions.empty());
}

void testLoadReturnsDefaultsInitially() {
    ConfigManager mgr;
    auto result = mgr.load();
    REQUIRE(result.isOk());
    REQUIRE(result.value().runtime.threadPoolSize == 4u);
    REQUIRE(result.value().transport.usbTimeoutMs == 5000u);
}

void testSetGetRuntimeConfig() {
    ConfigManager mgr;
    RuntimeConfig rc;
    rc.threadPoolSize = 8;
    rc.operationTimeoutMs = 60000;
    rc.enableTelemetry = true;
    rc.logLevel = "debug";
    REQUIRE(mgr.setRuntimeConfig(rc).isOk());
    auto result = mgr.runtimeConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().threadPoolSize == 8u);
    REQUIRE(result.value().operationTimeoutMs == 60000u);
    REQUIRE(result.value().enableTelemetry);
    REQUIRE(result.value().logLevel == std::string("debug"));
}

void testSetGetTransportConfig() {
    ConfigManager mgr;
    TransportConfig tc;
    tc.usbTimeoutMs = 10000;
    tc.maxRetries = 5;
    tc.bufferSize = 131072;
    REQUIRE(mgr.setTransportConfig(tc).isOk());
    auto result = mgr.transportConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().usbTimeoutMs == 10000u);
    REQUIRE(result.value().maxRetries == 5u);
    REQUIRE(result.value().bufferSize == 131072u);
}

void testSetGetWorkflowConfig() {
    ConfigManager mgr;
    WorkflowConfig wc;
    wc.maxRetries = 5;
    wc.enableRollback = false;
    wc.maxParallelStages = 8;
    REQUIRE(mgr.setWorkflowConfig(wc).isOk());
    auto result = mgr.workflowConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().maxRetries == 5u);
    REQUIRE(!result.value().enableRollback);
    REQUIRE(result.value().maxParallelStages == 8u);
}

void testSetGetJobConfig() {
    ConfigManager mgr;
    JobConfig jc;
    jc.maxParallel = 8;
    jc.enableHistory = false;
    jc.historyLimit = 500;
    REQUIRE(mgr.setJobConfig(jc).isOk());
    auto result = mgr.jobConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().maxParallel == 8u);
    REQUIRE(!result.value().enableHistory);
    REQUIRE(result.value().historyLimit == 500u);
}

void testSetGetDSPConfig() {
    ConfigManager mgr;
    DSPConfig dc;
    dc.enableVerification = false;
    dc.cacheSizeMb = 512;
    dc.repositoryPath = "/opt/dsp";
    dc.trustedVendors = {"qualcomm", "mediatek"};
    REQUIRE(mgr.setDSPConfig(dc).isOk());
    auto result = mgr.dspConfig();
    REQUIRE(result.isOk());
    REQUIRE(!result.value().enableVerification);
    REQUIRE(result.value().cacheSizeMb == 512u);
    REQUIRE(result.value().repositoryPath == std::string("/opt/dsp"));
    REQUIRE(result.value().trustedVendors.size() == size_t(2));
}

void testSetGetPluginConfig() {
    ConfigManager mgr;
    PluginConfig pc;
    pc.enableHotReload = true;
    pc.maxPlugins = 50;
    pc.pluginPaths = {"/usr/lib/plugins"};
    pc.blacklist = {"bad_plugin"};
    REQUIRE(mgr.setPluginConfig(pc).isOk());
    auto result = mgr.pluginConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().enableHotReload);
    REQUIRE(result.value().maxPlugins == 50u);
    REQUIRE(result.value().pluginPaths.size() == size_t(1));
    REQUIRE(result.value().blacklist.size() == size_t(1));
}

void testSetGetVendorConfig() {
    ConfigManager mgr;
    VendorConfig vc;
    vc.enableAutoDetect = false;
    vc.preferredVendors = {"qualcomm"};
    vc.vendorOptions["mode"] = "emergency";
    REQUIRE(mgr.setVendorConfig(vc).isOk());
    auto result = mgr.vendorConfig();
    REQUIRE(result.isOk());
    REQUIRE(!result.value().enableAutoDetect);
    REQUIRE(result.value().preferredVendors.size() == size_t(1));
    REQUIRE(result.value().vendorOptions.at("mode") == std::string("emergency"));
}

void testSetGetGUIConfig() {
    ConfigManager mgr;
    GUIConfig gc;
    gc.theme = "light";
    gc.windowWidth = 1920;
    gc.windowHeight = 1080;
    gc.maximizeOnStart = true;
    REQUIRE(mgr.setGUIConfig(gc).isOk());
    auto result = mgr.guiConfig();
    REQUIRE(result.isOk());
    REQUIRE(result.value().theme == std::string("light"));
    REQUIRE(result.value().windowWidth == 1920u);
    REQUIRE(result.value().windowHeight == 1080u);
    REQUIRE(result.value().maximizeOnStart);
}

void testSetGetCLIConfig() {
    ConfigManager mgr;
    CLIConfig cc;
    cc.enableColor = false;
    cc.outputFormat = "json";
    cc.verbosity = 2;
    REQUIRE(mgr.setCLIConfig(cc).isOk());
    auto result = mgr.cliConfig();
    REQUIRE(result.isOk());
    REQUIRE(!result.value().enableColor);
    REQUIRE(result.value().outputFormat == std::string("json"));
    REQUIRE(result.value().verbosity == 2u);
}

void testCustomOptions() {
    ConfigManager mgr;
    REQUIRE(mgr.setCustomOption("key1", "value1").isOk());
    REQUIRE(mgr.setCustomOption("key2", "value2").isOk());
    auto r1 = mgr.customOption("key1");
    REQUIRE(r1.isOk());
    REQUIRE(r1.value() == std::string("value1"));
    auto r2 = mgr.customOption("key2");
    REQUIRE(r2.isOk());
    REQUIRE(r2.value() == std::string("value2"));
}

void testCustomOptionNotFound() {
    ConfigManager mgr;
    auto result = mgr.customOption("nonexistent");
    REQUIRE(result.isError());
}

void testImportExportJsonRoundTrip() {
    ConfigManager mgr;
    RuntimeConfig rc;
    rc.threadPoolSize = 16;
    rc.enableProfiling = true;
    REQUIRE(mgr.setRuntimeConfig(rc).isOk());
    TransportConfig tc;
    tc.usbTimeoutMs = 9999;
    REQUIRE(mgr.setTransportConfig(tc).isOk());
    REQUIRE(mgr.setCustomOption("custom_key", "custom_val").isOk());

    auto exported = mgr.exportJson();
    REQUIRE(exported.isOk());

    ConfigManager mgr2;
    REQUIRE(mgr2.importJson(exported.value()).isOk());
    auto rt = mgr2.runtimeConfig();
    REQUIRE(rt.isOk());
    REQUIRE(rt.value().threadPoolSize == 16u);
    REQUIRE(rt.value().enableProfiling);
    auto tr = mgr2.transportConfig();
    REQUIRE(tr.isOk());
    REQUIRE(tr.value().usbTimeoutMs == 9999u);
    auto co = mgr2.customOption("custom_key");
    REQUIRE(co.isOk());
    REQUIRE(co.value() == std::string("custom_val"));
}

void testDefaults() {
    ConfigManager mgr;
    auto result = mgr.defaults();
    REQUIRE(result.isOk());
    REQUIRE(result.value().runtime.threadPoolSize == 4u);
    REQUIRE(result.value().source == ConfigSource::Default);
}

void testValidateValidConfig() {
    ConfigManager mgr;
    FullConfig cfg;
    auto result = mgr.validate(cfg);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == ConfigStatus::Valid);
}

void testValidateInvalidTimeout() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.runtime.operationTimeoutMs = 0;
    auto result = mgr.validate(cfg);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == ConfigStatus::Invalid);
}

void testValidateInvalidThreadPool() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.runtime.threadPoolSize = 0;
    auto result = mgr.validate(cfg);
    REQUIRE(result.isOk());
    REQUIRE(result.value() == ConfigStatus::Invalid);
}

void testInvalidTransportTimeout() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.transport.usbTimeoutMs = 0;
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testInvalidWorkflowRetries() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.workflow.maxRetries = 0;
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testInvalidJobMaxParallel() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.job.maxParallel = 0;
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testInvalidGUIWidth() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.gui.windowWidth = 0;
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testInvalidCLIFormat() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.cli.outputFormat = "";
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testInvalidDSPSize() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.dsp.cacheSizeMb = 0;
    REQUIRE(mgr.validate(cfg).value() == ConfigStatus::Invalid);
}

void testMigrateV1ToV2() {
    ConfigManager mgr;
    FullConfig old;
    old.runtime.threadPoolSize = 1;
    old.job.historyLimit = 0;

    auto result = mgr.migrate(old, 0, 2);
    REQUIRE(result.isOk());
    REQUIRE(result.value().runtime.threadPoolSize == 4u);
    REQUIRE(result.value().job.historyLimit == 1000u);
    REQUIRE(result.value().workflow.maxParallelStages == 4u);
}

void testFilePersistence() {
    ConfigManager mgr;
    auto dir = makeTempDir();
    auto path = dir + "/config.json";
    REQUIRE(mgr.setConfigPath(path).isOk());

    RuntimeConfig rc;
    rc.threadPoolSize = 32;
    rc.logLevel = "trace";
    REQUIRE(mgr.setRuntimeConfig(rc).isOk());
    FullConfig full;
    full.runtime = rc;
    REQUIRE(mgr.save(full).isOk());

    ConfigManager mgr2;
    REQUIRE(mgr2.setConfigPath(path).isOk());
    auto loaded = mgr2.load();
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().runtime.threadPoolSize == 32u);
    REQUIRE(loaded.value().runtime.logLevel == std::string("trace"));

    fs::remove_all(dir);
}

void testSaveWithoutPathDoesNotFail() {
    ConfigManager mgr;
    FullConfig cfg;
    cfg.runtime.threadPoolSize = 42;
    auto result = mgr.save(cfg);
    REQUIRE(result.isOk());
    auto loaded = mgr.load();
    REQUIRE(loaded.isOk());
    REQUIRE(loaded.value().runtime.threadPoolSize == 42u);
}

