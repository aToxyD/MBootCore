#pragma once

#include <mbootcore/domain/Error.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <iosfwd>
#include <functional>

namespace mbootcore {
namespace network {

enum class AddressFamily : uint8_t {
    IPv4,
    IPv6,
    Hostname
};

class NetworkAddress {
public:
    NetworkAddress() = default;
    NetworkAddress(const NetworkAddress&) = default;
    NetworkAddress& operator=(const NetworkAddress&) = default;
    NetworkAddress(NetworkAddress&&) = default;
    NetworkAddress& operator=(NetworkAddress&&) = default;

    static Result<NetworkAddress> fromString(std::string_view address, uint16_t port = 0);
    static NetworkAddress loopback(AddressFamily family = AddressFamily::IPv4);
    static NetworkAddress any(AddressFamily family = AddressFamily::IPv4);

    AddressFamily family() const noexcept { return m_family; }
    uint16_t port() const noexcept { return m_port; }
    std::string toString() const;

    bool operator==(const NetworkAddress& other) const noexcept;
    bool operator!=(const NetworkAddress& other) const noexcept { return !(*this == other); }
    bool operator<(const NetworkAddress& other) const noexcept;

    std::size_t hash() const noexcept;

private:
    struct IPv4Data { uint8_t bytes[4] = {}; };
    struct IPv6Data { uint8_t bytes[16] = {}; };
    struct HostnameData { std::string name; };

    using Data = std::variant<IPv4Data, IPv6Data, HostnameData>;

    NetworkAddress(AddressFamily family, Data data, uint16_t port);

    AddressFamily m_family = AddressFamily::IPv4;
    Data m_data = IPv4Data{};
    uint16_t m_port = 0;
};

std::ostream& operator<<(std::ostream& os, const NetworkAddress& addr);

} // namespace network
} // namespace mbootcore

template<>
struct std::hash<mbootcore::network::NetworkAddress> {
    std::size_t operator()(const mbootcore::network::NetworkAddress& addr) const noexcept {
        return addr.hash();
    }
};
