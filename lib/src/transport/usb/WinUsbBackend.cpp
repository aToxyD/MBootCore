#include "transport/usb/WinUsbBackend.hpp"
#include "platform/DynamicLibrary.hpp"
#include <mbootcore/domain/ILogger.hpp>

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <cstring>
#include <sstream>
#include <thread>
#include <iomanip>

namespace mbootcore {
namespace transport {
namespace usb {

#pragma pack(push, 1)
struct WinUsbSetupPacket {
    UCHAR   RequestType;
    UCHAR   Request;
    USHORT  Value;
    USHORT  Index;
    USHORT  Length;
};
#pragma pack(pop)

using WinUsb_Initialize_t = BOOL(WINAPI*)(HANDLE, void**);
using WinUsb_Free_t = BOOL(WINAPI*)(void*);
using WinUsb_ReadPipe_t = BOOL(WINAPI*)(void*, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
using WinUsb_WritePipe_t = BOOL(WINAPI*)(void*, UCHAR, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
using WinUsb_ControlTransfer_t = BOOL(WINAPI*)(void*, WinUsbSetupPacket*, PUCHAR, ULONG, PULONG, LPOVERLAPPED);
using WinUsb_GetAssociatedInterface_t = BOOL(WINAPI*)(void*, UCHAR, void**);
using WinUsb_QueryPipe_t = BOOL(WINAPI*)(void*, UCHAR, UCHAR, void*);
using WinUsb_ResetDevice_t = BOOL(WINAPI*)(void*);
using WinUsb_ResetPipe_t = BOOL(WINAPI*)(void*, UCHAR);
using WinUsb_AbortPipe_t = BOOL(WINAPI*)(void*, UCHAR);
using WinUsb_SetPipePolicy_t = BOOL(WINAPI*)(void*, UCHAR, ULONG, ULONG, PVOID);

struct WinUsbBackend::WinUsbFunctions {
    WinUsb_Initialize_t WinUsb_Initialize{nullptr};
    WinUsb_Free_t WinUsb_Free{nullptr};
    WinUsb_ReadPipe_t WinUsb_ReadPipe{nullptr};
    WinUsb_WritePipe_t WinUsb_WritePipe{nullptr};
    WinUsb_ControlTransfer_t WinUsb_ControlTransfer{nullptr};
    WinUsb_GetAssociatedInterface_t WinUsb_GetAssociatedInterface{nullptr};
    WinUsb_QueryPipe_t WinUsb_QueryPipe{nullptr};
    WinUsb_ResetDevice_t WinUsb_ResetDevice{nullptr};
    WinUsb_ResetPipe_t WinUsb_ResetPipe{nullptr};
    WinUsb_AbortPipe_t WinUsb_AbortPipe{nullptr};
    WinUsb_SetPipePolicy_t WinUsb_SetPipePolicy{nullptr};
};

WinUsbBackend::WinUsbBackend(ILogger* logger)
    : m_logger(logger) {
    m_wusb = std::make_unique<WinUsbFunctions>();
    m_winUsbAvailable = isWinUsbAvailable();
}

WinUsbBackend::~WinUsbBackend() {
    close();
}

bool WinUsbBackend::isWinUsbAvailable() {
    auto lib = platform::DynamicLibrary::load("winusb.dll");
    return lib.isOk();
}

bool WinUsbBackend::isAvailable() const noexcept {
    return m_winUsbAvailable;
}

std::string WinUsbBackend::findDevicePath(uint16_t vendorId, uint16_t productId, int interfaceNumber) {
    (void)interfaceNumber;
    GUID guid = GUID_DEVINTERFACE_USB_DEVICE;
    HDEVINFO devInfo = SetupDiGetClassDevsW(&guid, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return {};

    std::string foundPath;
    SP_DEVICE_INTERFACE_DATA ifcData = {};
    ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &guid, i, &ifcData); ++i) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &ifcData, nullptr, 0, &needed, nullptr);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) continue;

        auto detailBuf = std::make_unique<uint8_t[]>(needed);
        auto* detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuf.get());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifcData, detail, needed, nullptr, nullptr)) {
            continue;
        }

        std::wstring path = detail->DevicePath;
        wchar_t vidStr[16], pidStr[16];
        swprintf(vidStr, 16, L"VID_%04X", vendorId);
        swprintf(pidStr, 16, L"PID_%04X", productId);

        if (path.find(vidStr) != std::wstring::npos &&
            path.find(pidStr) != std::wstring::npos) {
            int len = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                foundPath.resize(static_cast<size_t>(len) - 1);
                WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &foundPath[0], len, nullptr, nullptr);
            }
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return foundPath;
}

bool WinUsbBackend::loadWinUsb() {
    auto result = platform::DynamicLibrary::load("winusb.dll");
    if (result.isError()) {
        if (m_logger) m_logger->warn("WinUsbBackend", "winusb.dll not found");
        return false;
    }
    m_lib = std::move(result.value());

#define LOAD_SYM(name) \
    do { \
        auto sym = m_lib->symbol(#name); \
        if (sym.isError()) { \
            if (m_logger) m_logger->warn("WinUsbBackend", "Failed to load " #name); \
            m_lib.reset(); \
            return false; \
        } \
        m_wusb->name = reinterpret_cast<name##_t>(sym.value()); \
    } while(0)

    LOAD_SYM(WinUsb_Initialize);
    LOAD_SYM(WinUsb_Free);
    LOAD_SYM(WinUsb_ReadPipe);
    LOAD_SYM(WinUsb_WritePipe);
    LOAD_SYM(WinUsb_ControlTransfer);
    LOAD_SYM(WinUsb_GetAssociatedInterface);
    LOAD_SYM(WinUsb_QueryPipe);
    LOAD_SYM(WinUsb_ResetDevice);
    LOAD_SYM(WinUsb_ResetPipe);
    LOAD_SYM(WinUsb_AbortPipe);
    LOAD_SYM(WinUsb_SetPipePolicy);

#undef LOAD_SYM

    if (m_logger) m_logger->info("WinUsbBackend", "WinUSB loaded successfully");
    return true;
}

Result<void> WinUsbBackend::open(uint16_t vendorId, uint16_t productId, int interfaceNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_open) return ErrorCode::TransportAlreadyOpen;

    if (!loadWinUsb()) {
        return ErrorCode::TransportBackendUnavailable;
    }

    auto path = findDevicePath(vendorId, productId, interfaceNumber);
    if (path.empty()) {
        if (m_logger) {
            std::ostringstream ss;
            ss << "Device not found: VID=0x" << std::hex << std::setw(4) << std::setfill('0')
               << vendorId << " PID=0x" << std::setw(4) << std::setfill('0') << productId;
            m_logger->error("WinUsbBackend", ss.str());
        }
        return ErrorCode::DeviceNotFound;
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);

    HANDLE devHandle = CreateFileW(wpath.c_str(),
                                     GENERIC_WRITE | GENERIC_READ,
                                     FILE_SHARE_WRITE | FILE_SHARE_READ,
                                     nullptr,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                     nullptr);

    if (devHandle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (m_logger) m_logger->error("WinUsbBackend",
            "CreateFile failed: " + std::to_string(err));
        return ErrorCode::TransportBackendUnavailable;
    }
    m_deviceHandle = devHandle;

    if (!m_wusb->WinUsb_Initialize(m_deviceHandle, &m_winUsbHandle)) {
        DWORD err = GetLastError();
        CloseHandle(devHandle);
        m_deviceHandle = nullptr;
        if (m_logger) m_logger->error("WinUsbBackend",
            "WinUsb_Initialize failed: " + std::to_string(err));
        return ErrorCode::TransportBackendUnavailable;
    }

    m_interfaceNumber = interfaceNumber;
    m_open = true;

    m_deviceInfo.vendorId = vendorId;
    m_deviceInfo.productId = productId;
    m_deviceInfo.devicePath = path;

    if (m_logger) {
        std::ostringstream ss;
        ss << "Opened VID=0x" << std::hex << std::setw(4) << std::setfill('0')
           << vendorId << " PID=0x" << std::setw(4) << std::setfill('0') << productId
           << " iface=" << interfaceNumber;
        m_logger->info("WinUsbBackend", ss.str());
    }
    return {};
}

void WinUsbBackend::close() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return;

    if (m_wusb && m_wusb->WinUsb_Free && m_winUsbHandle) {
        m_wusb->WinUsb_Free(m_winUsbHandle);
    }
    m_winUsbHandle = nullptr;

    if (m_deviceHandle) {
        CloseHandle(m_deviceHandle);
        m_deviceHandle = nullptr;
    }

    m_open = false;
    if (m_logger) m_logger->info("WinUsbBackend", "Closed");
}

bool WinUsbBackend::isOpen() const noexcept {
    return m_open;
}

Result<size_t> WinUsbBackend::bulkRead(uint8_t endpoint, uint8_t* buffer, size_t size,
                                        std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    ULONG transferred = 0;
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return ErrorCode::TransportError;

    BOOL result = m_wusb->WinUsb_ReadPipe(m_winUsbHandle, endpoint, buffer,
                                    static_cast<ULONG>(size), &transferred, &ov);

    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitResult = WaitForSingleObject(ov.hEvent,
                                                 static_cast<DWORD>(timeout.count()));
        if (waitResult == WAIT_TIMEOUT) {
            CancelIo(m_deviceHandle);
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportTimeout;
        }
        if (!GetOverlappedResult(m_deviceHandle, &ov, &transferred, FALSE)) {
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportReadFailed;
        }
    } else if (!result) {
        CloseHandle(ov.hEvent);
        return ErrorCode::TransportReadFailed;
    }

    CloseHandle(ov.hEvent);
    return static_cast<size_t>(transferred);
}

Result<size_t> WinUsbBackend::bulkWrite(uint8_t endpoint, const uint8_t* data, size_t size,
                                         std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    ULONG transferred = 0;
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return ErrorCode::TransportError;

    BOOL result = m_wusb->WinUsb_WritePipe(m_winUsbHandle, endpoint,
                                    const_cast<PUCHAR>(data),
                                    static_cast<ULONG>(size), &transferred, &ov);

    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitResult = WaitForSingleObject(ov.hEvent,
                                                 static_cast<DWORD>(timeout.count()));
        if (waitResult == WAIT_TIMEOUT) {
            CancelIo(m_deviceHandle);
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportTimeout;
        }
        if (!GetOverlappedResult(m_deviceHandle, &ov, &transferred, FALSE)) {
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportWriteFailed;
        }
    } else if (!result) {
        CloseHandle(ov.hEvent);
        return ErrorCode::TransportWriteFailed;
    }

    CloseHandle(ov.hEvent);
    return static_cast<size_t>(transferred);
}

Result<size_t> WinUsbBackend::controlRead(uint8_t requestType, uint8_t request,
                                           uint16_t value, uint16_t index,
                                           uint8_t* buffer, size_t size,
                                           std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    WinUsbSetupPacket setup = {};
    setup.RequestType = requestType;
    setup.Request = request;
    setup.Value = value;
    setup.Index = index;
    setup.Length = static_cast<USHORT>(size);

    ULONG transferred = 0;
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return ErrorCode::TransportError;

    BOOL result = m_wusb->WinUsb_ControlTransfer(m_winUsbHandle, &setup, buffer,
                                          static_cast<ULONG>(size), &transferred, &ov);

    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitResult = WaitForSingleObject(ov.hEvent,
                                                 static_cast<DWORD>(timeout.count()));
        if (waitResult == WAIT_TIMEOUT) {
            CancelIo(m_deviceHandle);
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportTimeout;
        }
        if (!GetOverlappedResult(m_deviceHandle, &ov, &transferred, FALSE)) {
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportReadFailed;
        }
    } else if (!result) {
        CloseHandle(ov.hEvent);
        return ErrorCode::TransportReadFailed;
    }

    CloseHandle(ov.hEvent);
    return static_cast<size_t>(transferred);
}

Result<size_t> WinUsbBackend::controlWrite(uint8_t requestType, uint8_t request,
                                            uint16_t value, uint16_t index,
                                            const uint8_t* data, size_t size,
                                            std::chrono::milliseconds timeout) {
    (void)data;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return ErrorCode::TransportNotOpen;

    WinUsbSetupPacket setup = {};
    setup.RequestType = requestType;
    setup.Request = request;
    setup.Value = value;
    setup.Index = index;
    setup.Length = static_cast<USHORT>(size);

    ULONG transferred = 0;
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) return ErrorCode::TransportError;

    std::vector<uint8_t> writeBuf(size);
    if (size > 0 && data) std::memcpy(writeBuf.data(), data, size);

    BOOL result = m_wusb->WinUsb_ControlTransfer(m_winUsbHandle, &setup,
                                           writeBuf.data(),
                                          static_cast<ULONG>(size), &transferred, &ov);

    if (!result && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitResult = WaitForSingleObject(ov.hEvent,
                                                 static_cast<DWORD>(timeout.count()));
        if (waitResult == WAIT_TIMEOUT) {
            CancelIo(m_deviceHandle);
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportTimeout;
        }
        if (!GetOverlappedResult(m_deviceHandle, &ov, &transferred, FALSE)) {
            CloseHandle(ov.hEvent);
            return ErrorCode::TransportWriteFailed;
        }
    } else if (!result) {
        CloseHandle(ov.hEvent);
        return ErrorCode::TransportWriteFailed;
    }

    CloseHandle(ov.hEvent);
    return static_cast<size_t>(transferred);
}

Result<void> WinUsbBackend::claimInterface(int interfaceNumber) {
    (void)interfaceNumber;
    return {};
}

Result<void> WinUsbBackend::releaseInterface(int interfaceNumber) {
    (void)interfaceNumber;
    return {};
}

Result<void> WinUsbBackend::resetDevice() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_wusb->WinUsb_ResetDevice) return ErrorCode::TransportNotOpen;
    if (!m_wusb->WinUsb_ResetDevice(m_winUsbHandle)) {
        return ErrorCode::TransportError;
    }
    return {};
}

Result<void> WinUsbBackend::resetPipe(uint8_t endpoint) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_wusb->WinUsb_ResetPipe) return ErrorCode::TransportNotOpen;
    if (!m_wusb->WinUsb_ResetPipe(m_winUsbHandle, endpoint)) {
        return ErrorCode::TransportError;
    }
    return {};
}

Result<void> WinUsbBackend::abortPipe(uint8_t endpoint) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_wusb->WinUsb_AbortPipe) return ErrorCode::TransportNotOpen;
    if (!m_wusb->WinUsb_AbortPipe(m_winUsbHandle, endpoint)) {
        return ErrorCode::TransportError;
    }
    return {};
}

Result<UsbDeviceInfo> WinUsbBackend::deviceInfo() const {
    if (!m_open) return ErrorCode::TransportNotOpen;
    return m_deviceInfo;
}

} // namespace usb
} // namespace transport
} // namespace mbootcore
