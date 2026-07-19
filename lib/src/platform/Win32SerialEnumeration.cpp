#ifdef _WIN32
#include "Win32SerialEnumeration.hpp"

#include "SafeParser.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <regstr.h>
#include <sstream>
#include <iomanip>


namespace mbootcore {
namespace platform {

namespace {

DEFINE_GUID(GUID_DEVINTERFACE_COMPORT,
    0x86E0D1E2, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73);

std::string readRegistryPortName(HKEY devKey) {
    HKEY portKey;
    if (RegOpenKeyExA(devKey, "Device Parameters", 0, KEY_READ, &portKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD type = 0;
        DWORD size = sizeof(buffer);
        if (RegQueryValueExA(portKey, "PortName", nullptr, &type,
                            reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS
            && type == REG_SZ) {
            RegCloseKey(portKey);
            return std::string(buffer);
        }
        RegCloseKey(portKey);
    }
    return {};
}

std::string getRegistryString(HKEY key, const char* valueName) {
    char buffer[1024];
    DWORD type = 0;
    DWORD size = sizeof(buffer);
    if (RegQueryValueExA(key, valueName, nullptr, &type,
                        reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS
        && type == REG_SZ) {
        return std::string(buffer);
    }
    return {};
}

uint16_t parseHexValue(const std::string& s, size_t pos) {
    if (pos >= s.size()) return 0;
    std::string hex;
    while (pos < s.size() && std::isxdigit(static_cast<unsigned char>(s[pos]))) {
        hex += s[pos];
        ++pos;
    }
    if (hex.empty()) return 0;
    auto result = fromCharsUint16(hex, 16);
    return result.ok ? result.value : 0;
}

void parseHardwareId(const std::string& hwId, uint16_t& vid, uint16_t& pid) {
    // Format: USB\VID_0403&PID_6001 or PCI\VEN_...
    auto vidPos = hwId.find("VID_");
    if (vidPos != std::string::npos) {
        vid = parseHexValue(hwId, vidPos + 4);
    }
    auto pidPos = hwId.find("PID_");
    if (pidPos != std::string::npos) {
        pid = parseHexValue(hwId, pidPos + 4);
    }
}

std::string extractSerialFromInstanceId(const std::string& instanceId) {
    // USB\VID_0403&PID_6001\A12345  →  "A12345"
    auto backslash = instanceId.rfind('\\');
    if (backslash == std::string::npos || backslash + 1 >= instanceId.size()) {
        return {};
    }
    std::string last = instanceId.substr(backslash + 1);
    if (last.find('{') != std::string::npos) return {};
    return last;
}

std::string getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property) {
    char buffer[4096];
    if (SetupDiGetDeviceRegistryPropertyA(devInfo, devData, property,
                                          nullptr, reinterpret_cast<PBYTE>(buffer),
                                          sizeof(buffer), nullptr)) {
        return std::string(buffer);
    }
    return {};
}

} // anonymous namespace

std::vector<SerialPortInfo> enumerateSerialPorts() {
    std::vector<SerialPortInfo> result;

    HDEVINFO devInfo = SetupDiGetClassDevsA(
        &GUID_DEVINTERFACE_COMPORT,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (devInfo == INVALID_HANDLE_VALUE) {
        return result;
    }

    SP_DEVINFO_DATA devData;
    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); ++i) {
        SerialPortInfo info;

        info.description = getDeviceProperty(devInfo, &devData, SPDRP_DEVICEDESC);
        info.manufacturer = getDeviceProperty(devInfo, &devData, SPDRP_MFG);
        info.friendlyName = getDeviceProperty(devInfo, &devData, SPDRP_FRIENDLYNAME);

        char instanceId[512];
        if (SetupDiGetDeviceInstanceIdA(devInfo, &devData, instanceId,
                                        sizeof(instanceId), nullptr)) {
            info.hardwareId = instanceId;
            parseHardwareId(instanceId, info.vid, info.pid);
            info.serialNumber = extractSerialFromInstanceId(instanceId);
        }

        HKEY devKey = SetupDiOpenDevRegKey(devInfo, &devData,
                                           DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (devKey != INVALID_HANDLE_VALUE) {
            info.portName = readRegistryPortName(devKey);
            if (info.description.empty()) {
                info.description = getRegistryString(devKey, "FriendlyName");
            }
            RegCloseKey(devKey);
        }

        if (info.portName.empty()) {
            continue;
        }

        result.push_back(std::move(info));
    }

    DWORD err = GetLastError();
    if (err != ERROR_NO_MORE_ITEMS) {
        SetupDiDestroyDeviceInfoList(devInfo);
        return result;
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return result;
}

} // namespace platform
} // namespace mbootcore

#endif // _WIN32
