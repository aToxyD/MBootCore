# Quick Start

Get MBootCore running in 5 minutes.

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
```

Qt is not required. The GUI (mboot-studio) builds automatically when Qt 6 is
detected. If Qt is not installed, only the core library, CLI, SDK, and tests
are built.

## Detect a Device

```bash
# Using the CLI
./mboot-cli detect

# Using the SDK (C++)
```

```cpp
#include <mbootcore/MBootCore.hpp>
#include <iostream>

int main() {
    mbootcore::Session session;

    auto result = session.connect();
    if (!result.isOk()) {
        std::cerr << "Failed to connect\n";
        return 1;
    }

    result = session.detectDevice();
    if (!result.isOk()) {
        std::cerr << "No device detected\n";
        session.disconnect();
        return 1;
    }

    std::cout << "Device detected and connected\n";
    session.disconnect();
    return 0;
}
```

## Run Tests

```bash
ctest --output-on-failure -j$(nproc)
```

## Next Steps

- [CLI Guide](../user-guide/CLI.md) — command-line tool usage
- [Build Options](../build/BuildOptions.md) — all CMake flags
- [Architecture](../architecture/Overview.md) — understand the system
- [Plugin Development](../sdk/PluginDevelopment.md) — extend MBootCore
- [Virtual Devices](../testing/VirtualDevices.md) — test without hardware
