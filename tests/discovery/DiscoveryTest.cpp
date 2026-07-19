#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <mbootcore/discovery/DiscoveryTypes.hpp>
#include <mbootcore/discovery/IDeviceDetector.hpp>
#include <mbootcore/discovery/IProtocolNegotiator.hpp>
#include <mbootcore/discovery/IProtocolFactory.hpp>
#include <mbootcore/discovery/ProtocolRegistry.hpp>
#include <mbootcore/discovery/DeviceDiscoveryEngine.hpp>
#include <mbootcore/discovery/ProtocolNegotiationEngine.hpp>
#include <mbootcore/discovery/VirtualDeviceDetector.hpp>
#include <mbootcore/pipeline/BootPipelineFactory.hpp>

#include <thread>
#include <chrono>

using namespace mbootcore;
using namespace mbootcore::discovery;

// Helper to disambiguate BootMode when both discovery:: and generic:: are visible
using DBM = discovery::BootMode;

namespace {

class TestFactory : public IProtocolFactory {
public:
    explicit TestFactory(ProtocolType type) : m_type(type) {}
    ProtocolType protocolType() const override { return m_type; }
    std::unique_ptr<IFlashDevice> createFlashDevice(const DeviceDescriptor&) override { return nullptr; }
    std::unique_ptr<pipeline::BootPipeline> createPipeline(const DeviceDescriptor&) override { return nullptr; }
private:
    ProtocolType m_type;
};

} // anonymous namespace

TEST_CASE("DiscoveryTest", "[discovery]") {

    // --- Enum/String Tests ---
    SECTION("testVendorEnumValues") {
        REQUIRE(static_cast<uint32_t>(Vendor::Unknown) == 0);
        REQUIRE(static_cast<uint32_t>(Vendor::Qualcomm) == 1);
        REQUIRE(static_cast<uint32_t>(Vendor::MediaTek) == 2);
        REQUIRE(static_cast<uint32_t>(Vendor::UNISOC) == 3);
        REQUIRE(static_cast<uint32_t>(Vendor::Samsung) == 4);
        REQUIRE(static_cast<uint32_t>(Vendor::Rockchip) == 5);
        REQUIRE(static_cast<uint32_t>(Vendor::Spreadtrum) == 6);
        REQUIRE(static_cast<uint32_t>(Vendor::Apple) == 7);
        REQUIRE(static_cast<uint32_t>(Vendor::Google) == 8);
        REQUIRE(static_cast<uint32_t>(Vendor::Huawei) == 9);
        REQUIRE(static_cast<uint32_t>(Vendor::Custom) == 0xFF);
    }

    SECTION("testBootModeEnumValues") {
        REQUIRE(static_cast<uint32_t>(BootMode::Unknown) == 0);
        REQUIRE(static_cast<uint32_t>(BootMode::BootROM) == 1);
        REQUIRE(static_cast<uint32_t>(BootMode::EDL) == 2);
        REQUIRE(static_cast<uint32_t>(BootMode::Firehose) == 3);
        REQUIRE(static_cast<uint32_t>(BootMode::Fastboot) == 4);
        REQUIRE(static_cast<uint32_t>(BootMode::ADB) == 5);
        REQUIRE(static_cast<uint32_t>(BootMode::Recovery) == 6);
        REQUIRE(static_cast<uint32_t>(BootMode::DownloadMode) == 7);
        REQUIRE(static_cast<uint32_t>(BootMode::Preloader) == 8);
        REQUIRE(static_cast<uint32_t>(BootMode::BROM) == 9);
        REQUIRE(static_cast<uint32_t>(BootMode::Normal) == 10);
        REQUIRE(static_cast<uint32_t>(BootMode::Download) == 11);
        REQUIRE(static_cast<uint32_t>(BootMode::Custom) == 0xFF);
    }

    SECTION("testTransportTypeEnumValues") {
        REQUIRE(static_cast<uint32_t>(TransportType::Unknown) == 0);
        REQUIRE(static_cast<uint32_t>(TransportType::Mock) == 1);
        REQUIRE(static_cast<uint32_t>(TransportType::Virtual) == 2);
        REQUIRE(static_cast<uint32_t>(TransportType::USB) == 3);
        REQUIRE(static_cast<uint32_t>(TransportType::Serial) == 4);
        REQUIRE(static_cast<uint32_t>(TransportType::TCP) == 5);
        REQUIRE(static_cast<uint32_t>(TransportType::UDP) == 6);
        REQUIRE(static_cast<uint32_t>(TransportType::Bluetooth) == 7);
        REQUIRE(static_cast<uint32_t>(TransportType::HID) == 8);
    }

    SECTION("testProtocolTypeEnumValues") {
        REQUIRE(static_cast<uint32_t>(ProtocolType::Unknown) == 0);
        REQUIRE(static_cast<uint32_t>(ProtocolType::Sahara) == 1);
        REQUIRE(static_cast<uint32_t>(ProtocolType::Firehose) == 2);
        REQUIRE(static_cast<uint32_t>(ProtocolType::Fastboot) == 3);
        REQUIRE(static_cast<uint32_t>(ProtocolType::MediaTekBROM) == 4);
        REQUIRE(static_cast<uint32_t>(ProtocolType::MediaTekDA) == 5);
        REQUIRE(static_cast<uint32_t>(ProtocolType::UNISOCBootROM) == 6);
        REQUIRE(static_cast<uint32_t>(ProtocolType::UNISOCFDL) == 7);
        REQUIRE(static_cast<uint32_t>(ProtocolType::USBStream) == 8);
        REQUIRE(static_cast<uint32_t>(ProtocolType::Custom) == 0xFF);
    }

    // --- DeviceDescriptor Tests ---
    SECTION("testDeviceDescriptorDefaults") {
        DeviceDescriptor desc;
        REQUIRE(desc.vendor == Vendor::Unknown);
        REQUIRE(desc.bootMode == BootMode::Unknown);
        REQUIRE(desc.transport == TransportType::Unknown);
        REQUIRE(desc.protocolHint == ProtocolType::Unknown);
        REQUIRE(desc.usbVid == 0);
        REQUIRE(desc.usbPid == 0);
        REQUIRE(desc.serialPort.empty());
        REQUIRE(desc.serialBaudRate == 115200);
        REQUIRE(desc.tcpHost.empty());
        REQUIRE(desc.tcpPort == 0);
        REQUIRE(desc.friendlyName.empty());
        REQUIRE(desc.connectionPath.empty());
        REQUIRE(desc.capabilities.empty());
        REQUIRE(desc.properties.empty());
    }

    SECTION("testDeviceDescriptorUsb") {
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::EDL;
        desc.transport = TransportType::USB;
        desc.protocolHint = ProtocolType::Sahara;
        desc.usbVid = 0x05C6;
        desc.usbPid = 0x9008;
        desc.friendlyName = "Qualcomm EDL Device";
        desc.connectionPath = "usb:1-2";

        REQUIRE(desc.vendor == Vendor::Qualcomm);
        REQUIRE(desc.bootMode == DBM::EDL);
        REQUIRE(desc.transport == TransportType::USB);
        REQUIRE(desc.protocolHint == ProtocolType::Sahara);
        REQUIRE(desc.usbVid == 0x05C6);
        REQUIRE(desc.usbPid == 0x9008);
        REQUIRE(desc.friendlyName == "Qualcomm EDL Device");
        REQUIRE(desc.connectionPath == "usb:1-2");
        REQUIRE(desc.isUsb() == true);
        REQUIRE(desc.isSerial() == false);
        REQUIRE(desc.isTcp() == false);
        REQUIRE(desc.isValid() == true);
    }

    SECTION("testDeviceDescriptorSerial") {
        DeviceDescriptor desc;
        desc.vendor = Vendor::MediaTek;
        desc.bootMode = BootMode::Preloader;
        desc.transport = TransportType::Serial;
        desc.protocolHint = ProtocolType::MediaTekBROM;
        desc.serialPort = "/dev/ttyUSB0";
        desc.serialBaudRate = 921600;

        REQUIRE(desc.vendor == Vendor::MediaTek);
        REQUIRE(desc.bootMode == DBM::Preloader);
        REQUIRE(desc.transport == TransportType::Serial);
        REQUIRE(desc.protocolHint == ProtocolType::MediaTekBROM);
        REQUIRE(desc.serialPort == "/dev/ttyUSB0");
        REQUIRE(desc.serialBaudRate == 921600);
        REQUIRE(desc.isSerial() == true);
        REQUIRE(desc.isUsb() == false);
        REQUIRE(desc.isTcp() == false);
    }

    SECTION("testDeviceDescriptorTcp") {
        DeviceDescriptor desc;
        desc.vendor = Vendor::UNISOC;
        desc.bootMode = BootMode::BROM;
        desc.transport = TransportType::TCP;
        desc.protocolHint = ProtocolType::UNISOCBootROM;
        desc.tcpHost = "192.168.1.100";
        desc.tcpPort = 9008;

        REQUIRE(desc.vendor == Vendor::UNISOC);
        REQUIRE(desc.bootMode == DBM::BROM);
        REQUIRE(desc.transport == TransportType::TCP);
        REQUIRE(desc.protocolHint == ProtocolType::UNISOCBootROM);
        REQUIRE(desc.tcpHost == "192.168.1.100");
        REQUIRE(desc.tcpPort == 9008);
        REQUIRE(desc.isTcp() == true);
        REQUIRE(desc.isUsb() == false);
        REQUIRE(desc.isSerial() == false);
        REQUIRE(desc.isValid() == true);
    }

    SECTION("testDeviceDescriptorTransportChecks") {
        DeviceDescriptor usbDesc;
        usbDesc.transport = TransportType::USB;
        REQUIRE(usbDesc.isUsb() == true);
        REQUIRE(usbDesc.isSerial() == false);
        REQUIRE(usbDesc.isTcp() == false);
        REQUIRE(usbDesc.isVirtual() == false);

        DeviceDescriptor serialDesc;
        serialDesc.transport = TransportType::Serial;
        REQUIRE(serialDesc.isSerial() == true);
        REQUIRE(serialDesc.isUsb() == false);
        REQUIRE(serialDesc.isTcp() == false);
        REQUIRE(serialDesc.isVirtual() == false);

        DeviceDescriptor tcpDesc;
        tcpDesc.transport = TransportType::TCP;
        REQUIRE(tcpDesc.isTcp() == true);
        REQUIRE(tcpDesc.isUsb() == false);
        REQUIRE(tcpDesc.isSerial() == false);
        REQUIRE(tcpDesc.isVirtual() == false);

        DeviceDescriptor virtualDesc;
        virtualDesc.transport = TransportType::Virtual;
        REQUIRE(virtualDesc.isVirtual() == true);
        REQUIRE(virtualDesc.isUsb() == false);
        REQUIRE(virtualDesc.isSerial() == false);
        REQUIRE(virtualDesc.isTcp() == false);

        DeviceDescriptor unknownDesc;
        REQUIRE(unknownDesc.isUsb() == false);
        REQUIRE(unknownDesc.isSerial() == false);
        REQUIRE(unknownDesc.isTcp() == false);
        REQUIRE(unknownDesc.isVirtual() == false);
    }

    SECTION("testDeviceDescriptorValidity") {
        DeviceDescriptor empty;
        REQUIRE(empty.isValid() == false);

        DeviceDescriptor withVendor;
        withVendor.vendor = Vendor::Qualcomm;
        REQUIRE(withVendor.isValid() == true);

        DeviceDescriptor withPath;
        withPath.connectionPath = "usb:1-1";
        REQUIRE(withPath.isValid() == true);

        DeviceDescriptor both;
        both.vendor = Vendor::Samsung;
        both.connectionPath = "usb:2-1";
        REQUIRE(both.isValid() == true);
    }

    // --- VirtualDeviceDetector Tests ---
    SECTION("testVirtualDetectorEmpty") {
        VirtualDeviceDetector detector;
        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.empty() == true);
        REQUIRE(detector.deviceCount() == 0);
    }

    SECTION("testVirtualDetectorQualcommEDL") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::Qualcomm);
        REQUIRE(dev.bootMode == DBM::EDL);
        REQUIRE(dev.protocolHint == ProtocolType::Sahara);
        REQUIRE(dev.usbVid == 0x05C6);
        REQUIRE(dev.usbPid == 0x9008);
    }

    SECTION("testVirtualDetectorQualcommFirehose") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommFirehose();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::Qualcomm);
        REQUIRE(dev.bootMode == DBM::Firehose);
        REQUIRE(dev.protocolHint == ProtocolType::Firehose);
        REQUIRE(dev.usbVid == 0x05C6);
        REQUIRE(dev.usbPid == 0x900E);
    }

    SECTION("testVirtualDetectorMediaTek") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createMediaTekPreloader();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::MediaTek);
        REQUIRE(dev.bootMode == DBM::Preloader);
        REQUIRE(dev.protocolHint == ProtocolType::MediaTekBROM);
        REQUIRE(dev.usbVid == 0x0E8D);
        REQUIRE(dev.usbPid == 0x2000);
    }

    SECTION("testVirtualDetectorUNISOC") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createUNISOCBootROM();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::UNISOC);
        REQUIRE(dev.bootMode == DBM::BROM);
        REQUIRE(dev.protocolHint == ProtocolType::UNISOCBootROM);
        REQUIRE(dev.usbVid == 0x1782);
        REQUIRE(dev.usbPid == 0x4D00);
    }

    SECTION("testVirtualDetectorSamsung") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createSamsungDownload();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::Samsung);
        REQUIRE(dev.bootMode == DBM::DownloadMode);
        REQUIRE(dev.protocolHint == ProtocolType::USBStream);
        REQUIRE(dev.usbVid == 0x04E8);
        REQUIRE(dev.usbPid == 0x685D);
    }

    SECTION("testVirtualDetectorRockchip") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createRockchipMaskROM();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::Rockchip);
        REQUIRE(dev.bootMode == DBM::BootROM);
        REQUIRE(dev.protocolHint == ProtocolType::USBStream);
        REQUIRE(dev.usbVid == 0x2207);
        REQUIRE(dev.usbPid == 0x350A);
    }

    SECTION("testVirtualDetectorUnknown") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createUnknownDevice();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
        auto& dev = devices[0];
        REQUIRE(dev.vendor == Vendor::Unknown);
        REQUIRE(dev.bootMode == DBM::Unknown);
        REQUIRE(dev.protocolHint == ProtocolType::Unknown);
        REQUIRE(dev.friendlyName == "Unknown Device");
    }

    SECTION("testVirtualDetectorDisconnected") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createDisconnectedDevice();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 0);
        REQUIRE(spec.connectable == false);
    }

    SECTION("testVirtualDetectorTimeout") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createTimeoutDevice();
        spec.probeDelayMs = 0;
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 1);
    }

    SECTION("testVirtualDetectorIdentify") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);

        DeviceDescriptor hint;
        hint.usbVid = 0x05C6;
        hint.usbPid = 0x9008;
        hint.connectionPath = "usb:05C6:9008";

        auto result = detector.identify(hint);
        REQUIRE(result.isOk() == true);
        auto identified = result.value();
        REQUIRE(identified.vendor == Vendor::Qualcomm);
        REQUIRE(identified.usbVid == 0x05C6);
        REQUIRE(identified.usbPid == 0x9008);
    }

    SECTION("testVirtualDetectorProbe") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);

        DeviceDescriptor desc;
        desc.usbVid = 0x05C6;
        desc.usbPid = 0x9008;
        desc.connectionPath = "usb:05C6:9008";

        auto result = detector.probe(desc);
        REQUIRE(result.isOk() == true);
    }

    SECTION("testVirtualDetectorIdentifyNotFound") {
        VirtualDeviceDetector detector;
        DeviceDescriptor hint;
        hint.usbVid = 0xFFFF;
        hint.usbPid = 0xFFFF;

        auto result = detector.identify(hint);
        REQUIRE(result.isError() == true);
        REQUIRE(result.error() == ErrorCode::DeviceNotFound);
    }

    SECTION("testVirtualDetectorRemoveDevice") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        detector.removeDevice(spec.connectionPath);
        REQUIRE(detector.deviceCount() == 0);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        REQUIRE(result.value().empty() == true);
    }

    SECTION("testVirtualDetectorClearDevices") {
        VirtualDeviceDetector detector;
        detector.addDevice(VirtualDeviceDetector::createQualcommEDL());
        detector.addDevice(VirtualDeviceDetector::createMediaTekPreloader());
        detector.addDevice(VirtualDeviceDetector::createUNISOCBootROM());
        REQUIRE(detector.deviceCount() == 3);

        detector.clearDevices();
        REQUIRE(detector.deviceCount() == 0);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        REQUIRE(result.value().empty() == true);
    }

    SECTION("testVirtualDetectorFailureInjection") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);

        detector.setFailProbability(1.0);
        detector.setRandomSeed(42);

        DeviceDescriptor desc;
        desc.usbVid = 0x05C6;
        desc.usbPid = 0x9008;

        auto probeResult = detector.probe(desc);
        REQUIRE(probeResult.isError() == true);
    }

    SECTION("testVirtualDetectorMultipleDevices") {
        VirtualDeviceDetector detector;
        detector.addDevice(VirtualDeviceDetector::createQualcommEDL());
        detector.addDevice(VirtualDeviceDetector::createMediaTekPreloader());
        detector.addDevice(VirtualDeviceDetector::createUNISOCBootROM());
        detector.addDevice(VirtualDeviceDetector::createSamsungDownload());
        detector.addDevice(VirtualDeviceDetector::createRockchipMaskROM());
        REQUIRE(detector.deviceCount() == 5);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        auto devices = result.value();
        REQUIRE(devices.size() == 5);

        REQUIRE(devices[0].vendor == Vendor::Qualcomm);
        REQUIRE(devices[1].vendor == Vendor::MediaTek);
        REQUIRE(devices[2].vendor == Vendor::UNISOC);
        REQUIRE(devices[3].vendor == Vendor::Samsung);
        REQUIRE(devices[4].vendor == Vendor::Rockchip);
    }

    // --- ProtocolRegistry Tests ---
    SECTION("testRegistryEmpty") {
        ProtocolRegistry registry;
        REQUIRE(registry.detectorCount() == 0);
        REQUIRE(registry.negotiatorCount() == 0);
        REQUIRE(registry.factoryCount() == 0);
        REQUIRE(registry.detectors().empty() == true);
        REQUIRE(registry.negotiators().empty() == true);
        REQUIRE(registry.factories().empty() == true);
    }

    SECTION("testRegistryRegisterDetector") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        registry.registerDetector(std::move(detector));
        REQUIRE(registry.detectorCount() == 1);
        REQUIRE(registry.detectors().empty() == false);
    }

    SECTION("testRegistryRegisterNegotiator") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara);
        registry.registerNegotiator(std::move(negotiator));
        REQUIRE(registry.negotiatorCount() == 1);
        REQUIRE(registry.negotiators().empty() == false);
    }

    SECTION("testRegistryRegisterFactory") {
        ProtocolRegistry registry;
        auto factory = std::make_unique<TestFactory>(ProtocolType::Sahara);
        registry.registerFactory(std::move(factory));
        REQUIRE(registry.factoryCount() == 1);
        REQUIRE(registry.factories().empty() == false);
    }

    SECTION("testRegistryFindDetector") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        registry.registerDetector(std::move(detector));
        auto* found = registry.findDetector("VirtualDeviceDetector");
        REQUIRE(found != nullptr);
        REQUIRE(found->name() == "VirtualDeviceDetector");
    }

    SECTION("testRegistryFindNegotiator") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara);
        registry.registerNegotiator(std::move(negotiator));
        auto* found = registry.findNegotiator("SaharaNegotiator");
        REQUIRE(found != nullptr);
        REQUIRE(found->name() == "SaharaNegotiator");
    }

    SECTION("testRegistryFindFactory") {
        ProtocolRegistry registry;
        auto factory = std::make_unique<TestFactory>(ProtocolType::Sahara);
        registry.registerFactory(std::move(factory));
        auto* found = registry.findFactory(ProtocolType::Sahara);
        REQUIRE(found != nullptr);
        REQUIRE(found->protocolType() == ProtocolType::Sahara);
    }

    SECTION("testRegistryUnregisterDetector") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        registry.registerDetector(std::move(detector));
        REQUIRE(registry.detectorCount() == 1);

        registry.unregisterDetector("VirtualDeviceDetector");
        REQUIRE(registry.detectorCount() == 0);
        auto* found = registry.findDetector("VirtualDeviceDetector");
        REQUIRE(found == nullptr);
    }

    SECTION("testRegistryUnregisterNegotiator") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Firehose);
        registry.registerNegotiator(std::move(negotiator));
        REQUIRE(registry.negotiatorCount() == 1);

        registry.unregisterNegotiator("FirehoseNegotiator");
        REQUIRE(registry.negotiatorCount() == 0);
        auto* found = registry.findNegotiator("FirehoseNegotiator");
        REQUIRE(found == nullptr);
    }

    SECTION("testRegistryUnregisterFactory") {
        ProtocolRegistry registry;
        auto factory = std::make_unique<TestFactory>(ProtocolType::Sahara);
        registry.registerFactory(std::move(factory));
        REQUIRE(registry.factoryCount() == 1);

        registry.unregisterFactory(ProtocolType::Sahara);
        REQUIRE(registry.factoryCount() == 0);
        auto* found = registry.findFactory(ProtocolType::Sahara);
        REQUIRE(found == nullptr);
    }

    SECTION("testRegistryClear") {
        ProtocolRegistry registry;
        registry.registerDetector(std::make_unique<VirtualDeviceDetector>());
        registry.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara));
        registry.registerFactory(std::make_unique<TestFactory>(ProtocolType::Firehose));
        REQUIRE(registry.detectorCount() == 1);
        REQUIRE(registry.negotiatorCount() == 1);
        REQUIRE(registry.factoryCount() == 1);

        registry.clear();
        REQUIRE(registry.detectorCount() == 0);
        REQUIRE(registry.negotiatorCount() == 0);
        REQUIRE(registry.factoryCount() == 0);
    }

    // --- DeviceDiscoveryEngine Tests ---
    SECTION("testDiscoveryEngineEmptyRegistry") {
        ProtocolRegistry registry;
        DeviceDiscoveryEngine engine(registry);
        REQUIRE(engine.registry() == &registry);

        auto result = engine.discoverAll(std::chrono::milliseconds(100));
        REQUIRE(result.devices.empty() == true);
        REQUIRE(result.error == true);
    }

    SECTION("testDiscoveryEngineDiscoverAll") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        detector->addDevice(VirtualDeviceDetector::createQualcommEDL());
        registry.registerDetector(std::move(detector));

        DeviceDiscoveryEngine engine(registry);
        auto result = engine.discoverAll(std::chrono::milliseconds(100));
        REQUIRE(result.devices.size() == 1);
        REQUIRE(result.devices[0].vendor == Vendor::Qualcomm);
    }

    SECTION("testDiscoveryEngineDiscoverByVendor") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        detector->addDevice(VirtualDeviceDetector::createQualcommEDL());
        detector->addDevice(VirtualDeviceDetector::createMediaTekPreloader());
        registry.registerDetector(std::move(detector));

        DeviceDiscoveryEngine engine(registry);
        auto qualcommDevices = engine.discoverByVendor(Vendor::Qualcomm, std::chrono::milliseconds(100));
        REQUIRE(qualcommDevices.size() == 1);
        REQUIRE(qualcommDevices[0].vendor == Vendor::Qualcomm);
    }

    SECTION("testDiscoveryEngineProbeDevice") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        detector->addDevice(VirtualDeviceDetector::createQualcommEDL());
        registry.registerDetector(std::move(detector));

        DeviceDiscoveryEngine engine(registry);
        DeviceDescriptor hint;
        hint.usbVid = 0x05C6;
        hint.usbPid = 0x9008;
        hint.connectionPath = "usb:05C6:9008";

        auto result = engine.probeDevice(hint);
        REQUIRE(result.isOk() == true);
    }

    SECTION("testDiscoveryEngineProbeUnknown") {
        ProtocolRegistry registry;
        auto detector = std::make_unique<VirtualDeviceDetector>();
        registry.registerDetector(std::move(detector));

        DeviceDiscoveryEngine engine(registry);
        DeviceDescriptor hint;
        hint.usbVid = 0xFFFF;
        hint.usbPid = 0xFFFF;

        auto result = engine.probeDevice(hint);
        REQUIRE(result.isError() == true);
        REQUIRE(result.error() == ErrorCode::DeviceNotFound);
    }

    // --- ProtocolNegotiationEngine Tests ---
    SECTION("testNegotiationEngineEmpty") {
        ProtocolRegistry registry;
        ProtocolNegotiationEngine engine(registry);

        DeviceDescriptor desc;
        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::Unknown);
        REQUIRE(result.confidence == 0);
    }

    SECTION("testNegotiationEngineSaharaMatch") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara, 100);
        registry.registerNegotiator(std::move(negotiator));

        ProtocolNegotiationEngine engine(registry);
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::EDL;

        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::Sahara);
        REQUIRE(result.confidence > 0);
    }

    SECTION("testNegotiationEngineFirehoseMatch") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Firehose, 100);
        registry.registerNegotiator(std::move(negotiator));

        ProtocolNegotiationEngine engine(registry);
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::Firehose;

        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::Firehose);
        REQUIRE(result.confidence > 0);
    }

    SECTION("testNegotiationEngineMediaTekMatch") {
        ProtocolRegistry registry;
        auto negotiator = std::make_unique<VirtualProtocolNegotiator>(ProtocolType::MediaTekBROM, 100);
        registry.registerNegotiator(std::move(negotiator));

        ProtocolNegotiationEngine engine(registry);
        DeviceDescriptor desc;
        desc.vendor = Vendor::MediaTek;
        desc.bootMode = BootMode::Preloader;

        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::MediaTekBROM);
        REQUIRE(result.confidence > 0);
    }

    SECTION("testNegotiationEngineMultipleMatches") {
        ProtocolRegistry registry;
        registry.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara, 80));
        registry.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Firehose, 90));

        ProtocolNegotiationEngine engine(registry);
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::EDL;

        auto allResults = engine.negotiateAll(desc);
        REQUIRE(allResults.size() == 2);
    }

    SECTION("testNegotiationEngineBestMatchConfidence") {
        ProtocolRegistry registry;
        registry.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Sahara, 50));
        registry.registerNegotiator(std::make_unique<VirtualProtocolNegotiator>(ProtocolType::Firehose, 100));

        ProtocolNegotiationEngine engine(registry);
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::Firehose;

        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::Firehose);
        REQUIRE(result.confidence == 120);
    }

    SECTION("testNegotiationEngineNoMatch") {
        ProtocolRegistry registry;
        ProtocolNegotiationEngine engine(registry);

        DeviceDescriptor desc;
        desc.vendor = Vendor::Apple;

        auto result = engine.negotiate(desc);
        REQUIRE(result.protocol == ProtocolType::Unknown);
        REQUIRE(result.confidence == 0);
    }

    // --- VirtualProtocolNegotiator Tests ---
    SECTION("testVirtualNegotiatorNames") {
        VirtualProtocolNegotiator saharaNeg(ProtocolType::Sahara);
        REQUIRE(saharaNeg.name() == "SaharaNegotiator");
        REQUIRE(saharaNeg.confidence() == 100);

        VirtualProtocolNegotiator firehoseNeg(ProtocolType::Firehose);
        REQUIRE(firehoseNeg.name() == "FirehoseNegotiator");

        VirtualProtocolNegotiator mtkNeg(ProtocolType::MediaTekBROM);
        REQUIRE(mtkNeg.name() == "MediaTekBROMNegotiator");

        VirtualProtocolNegotiator customNeg(ProtocolType::Custom, 50);
        REQUIRE(customNeg.name() == "CustomNegotiator");
        REQUIRE(customNeg.confidence() == 50);

        customNeg.setConfidence(75);
        REQUIRE(customNeg.confidence() == 75);
    }

    // --- Factory Integration Tests ---
    SECTION("testBootPipelineFromDescriptor") {
        DeviceDescriptor desc;
        desc.vendor = Vendor::Qualcomm;
        desc.bootMode = BootMode::EDL;
        desc.protocolHint = ProtocolType::Sahara;
        desc.friendlyName = "Test Device";
        desc.connectionPath = "usb:1-1";

        auto pipeline = pipeline::BootPipelineFactory::createFromDescriptor(desc);
        REQUIRE(pipeline != nullptr);

        auto& ctx = pipeline->context();
        REQUIRE(ctx.properties.at("vendor") == "1");
        REQUIRE(ctx.properties.at("bootMode") == "2");
        REQUIRE(ctx.properties.at("protocolHint") == "1");
        REQUIRE(ctx.properties.at("friendlyName") == "Test Device");
        REQUIRE(ctx.properties.at("connectionPath") == "usb:1-1");
    }

    // --- Hotplug / Reconnect Tests ---
    SECTION("testVirtualDetectorHotPlug") {
        VirtualDeviceDetector detector;
        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        REQUIRE(result.value().empty() == true);

        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto afterPlug = detector.enumerate();
        REQUIRE(afterPlug.isOk() == true);
        REQUIRE(afterPlug.value().size() == 1);
        REQUIRE(afterPlug.value()[0].vendor == Vendor::Qualcomm);
    }

    SECTION("testVirtualDetectorHotUnplug") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        detector.removeDevice(spec.connectionPath);
        REQUIRE(detector.deviceCount() == 0);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        REQUIRE(result.value().empty() == true);
    }

    SECTION("testVirtualDetectorReconnect") {
        VirtualDeviceDetector detector;
        auto spec = VirtualDeviceDetector::createQualcommEDL();
        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto first = detector.enumerate();
        REQUIRE(first.isOk() == true);
        REQUIRE(first.value().size() == 1);

        detector.removeDevice(spec.connectionPath);
        REQUIRE(detector.deviceCount() == 0);

        detector.addDevice(spec);
        REQUIRE(detector.deviceCount() == 1);

        auto second = detector.enumerate();
        REQUIRE(second.isOk() == true);
        REQUIRE(second.value().size() == 1);
        REQUIRE(second.value()[0].vendor == Vendor::Qualcomm);
    }

    // --- Stress Tests ---
    SECTION("testStressManyDevices") {
        VirtualDeviceDetector detector;
        constexpr int Count = 100;
        for (int i = 0; i < Count; ++i) {
            auto spec = VirtualDeviceDetector::createUnknownDevice();
            spec.friendlyName = "Device " + std::to_string(i);
            spec.connectionPath = "virt:" + std::to_string(i);
            detector.addDevice(spec);
        }
        REQUIRE(detector.deviceCount() == Count);

        auto result = detector.enumerate();
        REQUIRE(result.isOk() == true);
        REQUIRE(result.value().size() == static_cast<std::size_t>(Count));

        for (int i = 0; i < Count; ++i) {
            REQUIRE(result.value()[static_cast<std::size_t>(i)].friendlyName == "Device " + std::to_string(i));
        }
    }

    SECTION("testStressRapidRegistryOps") {
        ProtocolRegistry registry;
        constexpr int OpCount = 50;

        for (int i = 0; i < OpCount; ++i) {
            auto detector = std::make_unique<VirtualDeviceDetector>();
            registry.registerDetector(std::move(detector));
        }
        REQUIRE(registry.detectorCount() == static_cast<std::size_t>(OpCount));

        for (int i = 0; i < OpCount; ++i) {
            registry.unregisterDetector("VirtualDeviceDetector");
        }

        registry.clear();
        REQUIRE(registry.detectorCount() == 0);
        REQUIRE(registry.negotiatorCount() == 0);
        REQUIRE(registry.factoryCount() == 0);
    }
}
