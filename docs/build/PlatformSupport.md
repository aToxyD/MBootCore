# Platform Support

## Host Platforms

| Platform | Status | Build System | Notes |
|----------|--------|-------------|-------|
| Windows 10 64-bit | ✅ Tier 1 | MinGW-w64 (WinLibs) | Primary dev platform |
| Windows 10 32-bit | ✅ Tier 1 | MinGW-w64 (WinLibs) | Primary dev platform |
| Windows 11 64-bit | ✅ Tier 1 | MinGW-w64 (WinLibs) | Compatible |
| Ubuntu 20.04+ | ✅ Tier 1 | GCC + CMake | Release-validated |
| Ubuntu 22.04+ | ✅ Tier 1 | GCC + CMake | Release-validated |
| macOS 12+ (Intel) | ✅ Tier 2 | Clang + CMake | Tested quarterly |
| macOS 13+ (Apple Silicon) | ✅ Tier 2 | Clang + CMake | Tested quarterly |
| Fedora 36+ | ⚠️ Tier 3 | GCC + CMake | Community maintained |
| Arch Linux | ⚠️ Tier 3 | GCC + CMake | Community maintained |

### Tier Definitions

- **Tier 1**: Full release packaging, all tests passing locally, release gates
- **Tier 2**: Release packaging, quarterly full validation
- **Tier 3**: Community-maintained, partial coverage

## Target Devices

| Vendor | Protocol | Transport | Status |
|--------|----------|-----------|--------|
| Qualcomm | Sahara → Firehose | USB (WinUSB) | ✅ Production (Tier 1) |
| MediaTek | BROM → DA | USB (WinUSB/VCOM) | 🏗️ Scaffold (reference only) |
| UNISOC | SPRD BROM | USB | 🏗️ Scaffold (reference only) |
| Samsung | Odin Download | USB | 🔮 Future |
| Rockchip | Rockchip Loader | USB | 🔮 Future |
| Generic | Raw Flash | USB/Serial/TCP | 🧩 Pattern demo |

## Windows

### Requirements

- Windows 10 or later (32-bit or 64-bit)
- WinUSB driver (installed via Zadig)
- MinGW-w64 from [WinLibs](https://winlibs.com/)
- Qt 6.11.1 static for MinGW 32-bit (optional — only for building `mboot-studio`)

### Environment Setup

```powershell
# Set up MinGW path
$env:Path = "C:\winlibs\mingw32\bin;$env:Path"

# Build (Qt is not required for core library and CLI; Studio auto-detects Qt)
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
ctest --output-on-failure -j4
```

### WinUSB Driver Installation

1. Download [Zadig](https://zadig.akeo.ie/)
2. Connect device in boot mode
3. Select device from list
4. Choose "WinUSB" driver
5. Click "Replace Driver"

## Linux

### Requirements

- GCC 9+ or Clang 14+
- CMake 3.20+
- libusb 1.0 (for USB transport on Linux)
- pkg-config
- Qt 6.5+ (optional — required only for building `mboot-studio`)

### Installation

```bash
# Ubuntu/Debian (core only — no Qt)
sudo apt install build-essential cmake libusb-1.0-0-dev

# Ubuntu/Debian (with GUI support)
sudo apt install build-essential cmake qt6-base-dev libusb-1.0-0-dev

# Fedora (core only)
sudo dnf install gcc-c++ cmake libusbx-devel

# Fedora (with GUI support)
sudo dnf install gcc-c++ cmake qt6-qtbase-devel libusbx-devel

# Arch (core only)
sudo pacman -S base-devel cmake libusb

# Arch (with GUI support)
sudo pacman -S base-devel cmake qt6-base libusb
```

### udev Rules

```bash
# /etc/udev/rules.d/51-android.rules
# Qualcomm EDL
SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="9008", MODE="0666"
# MediaTek BROM
SUBSYSTEM=="usb", ATTR{idVendor}=="0e8d", ATTR{idProduct}=="0003", MODE="0666"

sudo udevadm control --reload-rules
sudo udevadm trigger
```

## macOS

### Requirements

- macOS 12 (Monterey) or later
- Xcode 14+ or Command Line Tools
- CMake 3.20+
- libusb (via Homebrew)
- Qt 6.5+ (optional — required only for building `mboot-studio`)

### Installation

```bash
# Core only
brew install cmake libusb

# With GUI support
brew install cmake qt@6 libusb
```
