# Configuration System

## Overview

MBootCore provides a `ConfigManager` that loads configuration from JSON, YAML, or INI files using a strategy pattern (`ConfigParserStrategy`). Configuration is organized into typed key-value sections.

## ConfigManager

```cpp
#include <mbootcore/config/ConfigManager.hpp>

mbootcore::ConfigManager config;
config.loadFromFile("config.json");
// or
config.loadFromFile("config.yaml");
// or
config.loadFromFile("config.ini");
```

## Supported Formats

### JSON

```json
{
    "transport": {
        "type": "usb",
        "timeout": 5000
    },
    "session": {
        "maxRetries": 3,
        "operationTimeout": 30000
    }
}
```

### YAML

```yaml
transport:
  type: usb
  timeout: 5000
session:
  maxRetries: 3
  operationTimeout: 30000
```

### INI

```ini
[transport]
type=usb
timeout=5000

[session]
maxRetries=3
operationTimeout=30000
```

## Configuration Values

Configuration values are stored as typed key-value pairs within sections:

```cpp
// Query configuration
auto timeout = config.getValue("transport", "timeout");
auto retries = config.getValue("session", "maxRetries");

// Set configuration at runtime
config.setValue("transport", "timeout", "10000");

// Check if a key exists
if (config.hasKey("session", "operationTimeout")) {
    // ...
}
```

## Configuration Types

The `ConfigTypes.hpp` header defines the configuration data model:

- `ConfigSection` — named group of configuration entries
- `ConfigEntry` — single key-value pair with type information
- `ConfigValue` — variant type supporting string, integer, boolean, and floating-point values
