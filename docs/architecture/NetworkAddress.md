# NetworkAddress

A value type representing IPv4, IPv6, or hostname-based network addresses
with a port. Used as endpoint addresses for TCP transport connections.

**Header:** `mbootcore/network/NetworkAddress.hpp`

## Address Family

| Value | Description |
|-------|-------------|
| `IPv4` | IPv4 address (4 bytes) |
| `IPv6` | IPv6 address (16 bytes) |
| `Hostname` | Hostname string |

## Construction

```cpp
#include <mbootcore/MBootCore.hpp>

// Parse from string
auto addr = mbootcore::NetworkAddress::fromString("192.168.1.100", 8080);
if (addr.isOk()) {
    auto a = addr.takeValue();
    std::cout << a.toString();  // "192.168.1.100:8080"
}

// Loopback addresses
auto loopback4 = mbootcore::NetworkAddress::loopback();          // 127.0.0.1
auto loopback6 = mbootcore::NetworkAddress::loopback(
    mbootcore::AddressFamily::IPv6);                              // ::1

// "Any" bind addresses
auto any4 = mbootcore::NetworkAddress::any();                     // 0.0.0.0
auto any6 = mbootcore::NetworkAddress::any(
    mbootcore::AddressFamily::IPv6);                              // ::
```

## Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `family()` | `AddressFamily` | Address family |
| `port()` | `uint16` | Port number |
| `toString()` | `string` | Human-readable string |

## Comparison and Hashing

`NetworkAddress` supports equality, inequality, and less-than comparison.
A `hash()` method and `std::hash` specialization allow use as keys in
`std::unordered_map` and `std::unordered_set`.

```cpp
auto a = mbootcore::NetworkAddress::fromString("127.0.0.1", 80).takeValue();
auto b = mbootcore::NetworkAddress::fromString("127.0.0.1", 80).takeValue();
assert(a == b);

// Use as unordered_map key
std::unordered_map<mbootcore::NetworkAddress, std::string> endpoints;
endpoints[a] = "server1";
```

## See Also

- [Transport Monitor](../gui/Architecture.md) — transport layer overview
- [Glossary](../reference/Glossary.md) — `NetworkAddress` definition
