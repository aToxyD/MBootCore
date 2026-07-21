# Installation

## Prerequisites

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | 3.20+ (3.23+ for presets) | Build system |
| C++ compiler | GCC 11+, Clang 15+, or MSVC | C++17 support required |
| Qt (optional) | 6.5+ | Required only for mboot-studio GUI |

All third-party libraries (nlohmann_json, zlib, Mbed TLS, libusb, Catch2) are
auto-downloaded and statically built by the dependency manager. No manual
installation required.

## Linux

```bash
# Ubuntu/Debian (core only)
sudo apt install build-essential cmake libusb-1.0-0-dev

# Ubuntu/Debian (with GUI support)
sudo apt install build-essential cmake qt6-base-dev libusb-1.0-0-dev

# Fedora (core only)
sudo dnf install gcc-c++ cmake libusbx-devel

# Arch (core only)
sudo pacman -S base-devel cmake libusb
```

### udev Rules (for USB device access)

```bash
# /etc/udev/rules.d/51-android.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="9008", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="0e8d", ATTR{idProduct}=="0003", MODE="0666"

sudo udevadm control --reload-rules
sudo udevadm trigger
```

## macOS

```bash
# Core only
brew install cmake libusb

# With GUI support
brew install cmake qt@6 libusb
```

## Windows

- Windows 10 or later
- MinGW-w64 from [WinLibs](https://winlibs.com/)
- WinUSB driver (installed via [Zadig](https://zadig.akeo.ie/))

```powershell
# Set up MinGW path
$env:Path = "C:\winlibs\mingw32\bin;$env:Path"
```

## Build from Source

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure -j$(nproc)
```

See [Build Guide](../build/Build.md) for CMake presets and advanced options.
See [Platform Support](../build/PlatformSupport.md) for platform-specific details.
