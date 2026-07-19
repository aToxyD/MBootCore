#include <mbootcore/transport/usb/UsbDeviceEnumerator.hpp>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <usb200.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

namespace mbootcore {
namespace transport {
namespace usb {

namespace {

std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(static_cast<size_t>(len) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::wstring getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property) {
    DWORD dataType = 0;
    DWORD size = 0;
    SetupDiGetDeviceRegistryPropertyW(devInfo, devData, property, &dataType, nullptr, 0, &size);
    if (size == 0) return {};

    auto buffer = std::make_unique<uint8_t[]>(size);
    if (!SetupDiGetDeviceRegistryPropertyW(devInfo, devData, property, &dataType,
                                            buffer.get(), size, nullptr)) {
        return {};
    }
    return std::wstring(reinterpret_cast<const wchar_t*>(buffer.get()));
}

uint16_t parseHex(const std::wstring& str, size_t pos, size_t len) {
    uint16_t val = 0;
    for (size_t i = 0; i < len && (pos + i) < str.size(); ++i) {
        wchar_t c = str[pos + i];
        val <<= 4;
        if (c >= L'0' && c <= L'9') val |= static_cast<uint16_t>(c - L'0');
        else if (c >= L'A' && c <= L'F') val |= static_cast<uint16_t>(c - L'A' + 10);
        else if (c >= L'a' && c <= L'f') val |= static_cast<uint16_t>(c - L'a' + 10);
    }
    return val;
}

void extractVidPid(const std::wstring& path, uint16_t& vid, uint16_t& pid) {
    vid = 0; pid = 0;
    auto vidPos = path.find(L"VID_");
    auto pidPos = path.find(L"PID_");
    if (vidPos != std::wstring::npos) vid = parseHex(path, vidPos + 4, 4);
    if (pidPos != std::wstring::npos) pid = parseHex(path, pidPos + 4, 4);
}

UsbSpeed getDeviceSpeed(const std::wstring&) {
    return UsbSpeed::Unknown;
}

} // anonymous namespace

std::vector<UsbDeviceInfo> UsbDeviceEnumerator::enumerate(const EnumerateFilter& filter) {
    std::vector<UsbDeviceInfo> devices;

    GUID guid = GUID_DEVINTERFACE_USB_DEVICE;
    HDEVINFO devInfo = SetupDiGetClassDevsW(&guid, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return devices;

    SP_DEVICE_INTERFACE_DATA ifcData = {};
    ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &guid, i, &ifcData); ++i) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifcData, nullptr, 0, &needed, nullptr);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) continue;

        auto detailBuf = std::make_unique<uint8_t[]>(needed);
        auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuf.get());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA devData = {};
        devData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifcData, detail, needed, nullptr, &devData)) {
            continue;
        }

        UsbDeviceInfo info;
        info.devicePath = wstringToString(detail->DevicePath);

        // Extract VID/PID from device path
        extractVidPid(detail->DevicePath, info.vendorId, info.productId);

        // Apply filter
        if (filter.vendorId != 0 && info.vendorId != filter.vendorId) continue;
        if (filter.productId != 0 && info.productId != filter.productId) continue;

        // Get manufacturer, product, serial
        info.manufacturer = wstringToString(getDeviceProperty(devInfo, &devData, SPDRP_MFG));
        info.product = wstringToString(getDeviceProperty(devInfo, &devData, SPDRP_FRIENDLYNAME));
        if (info.product.empty()) {
            info.product = wstringToString(getDeviceProperty(devInfo, &devData, SPDRP_DEVICEDESC));
        }

        // Get instance ID for serial number
        auto instanceId = getDeviceProperty(devInfo, &devData, SPDRP_HARDWAREID);
        if (!instanceId.empty()) {
            // Extract serial from instance ID (after last backslash)
            auto pos = instanceId.rfind(L'\\');
            if (pos != std::wstring::npos) {
                auto serialPart = instanceId.substr(pos + 1);
                info.serialNumber = wstringToString(serialPart);
            }
        }

        // Get bus address
        info.busAddress = wstringToString(getDeviceProperty(devInfo, &devData, SPDRP_LOCATION_INFORMATION));

        // Get speed
        auto devInstId = getDeviceProperty(devInfo, &devData, SPDRP_INSTALL_STATE);
        info.speed = getDeviceSpeed(instanceId);

        info.isAvailable = true;

        if (filter.bootModeOnly && !info.isBootMode()) continue;

        devices.push_back(std::move(info));
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return devices;
}

std::vector<UsbDeviceInfo> UsbDeviceEnumerator::findByVendor(uint16_t vendorId) {
    EnumerateFilter filter;
    filter.vendorId = vendorId;
    return enumerate(filter);
}

std::vector<UsbDeviceInfo> UsbDeviceEnumerator::findByProduct(uint16_t vendorId, uint16_t productId) {
    EnumerateFilter filter;
    filter.vendorId = vendorId;
    filter.productId = productId;
    return enumerate(filter);
}

UsbDeviceInfo UsbDeviceEnumerator::findFirst(uint16_t vendorId, uint16_t productId) {
    auto devices = findByProduct(vendorId, productId);
    if (!devices.empty()) return devices[0];
    return {};
}

size_t UsbDeviceEnumerator::deviceCount() {
    return enumerate().size();
}

std::vector<UsbDeviceInfo> UsbDeviceEnumerator::findBootModeDevices() {
    EnumerateFilter filter;
    filter.bootModeOnly = true;
    return enumerate(filter);
}

} // namespace usb
} // namespace transport
} // namespace mbootcore
