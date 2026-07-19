#include <mbootcore/network/NetworkAddress.hpp>

#include <algorithm>
#include <cstring>
#include <ostream>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace mbootcore {
namespace network {

namespace {

bool parseIPv4(std::string_view s, uint8_t (&out)[4]) {
#ifdef _WIN32
    in_addr addr;
    if (InetPtonA(AF_INET, s.data(), &addr) != 1)
        return false;
    std::memcpy(out, &addr, 4);
    return true;
#else
    in_addr addr;
    if (inet_pton(AF_INET, s.data(), &addr) != 1)
        return false;
    std::memcpy(out, &addr, 4);
    return true;
#endif
}

bool parseIPv6(std::string_view s, uint8_t (&out)[16]) {
#ifdef _WIN32
    in6_addr addr;
    if (InetPtonA(AF_INET6, s.data(), &addr) != 1)
        return false;
    std::memcpy(out, &addr, 16);
    return true;
#else
    in6_addr addr;
    if (inet_pton(AF_INET6, s.data(), &addr) != 1)
        return false;
    std::memcpy(out, &addr, 16);
    return true;
#endif
}

std::string formatIPv4(const uint8_t (&bytes)[4]) {
    std::ostringstream os;
    os << static_cast<int>(bytes[0]) << '.'
       << static_cast<int>(bytes[1]) << '.'
       << static_cast<int>(bytes[2]) << '.'
       << static_cast<int>(bytes[3]);
    return os.str();
}

std::string formatIPv6(const uint8_t (&bytes)[16]) {
    char buf[INET6_ADDRSTRLEN];
    in6_addr addr;
    std::memcpy(&addr, bytes, 16);
    if (inet_ntop(AF_INET6, &addr, buf, sizeof(buf)))
        return buf;
    return "::";
}

bool bytesEqual(const uint8_t* a, const uint8_t* b, size_t n) {
    return std::memcmp(a, b, n) == 0;
}

int bytesCompare(const uint8_t* a, const uint8_t* b, size_t n) {
    return std::memcmp(a, b, n);
}

inline void hashCombine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // anonymous namespace

NetworkAddress::NetworkAddress(AddressFamily family, Data data, uint16_t port)
    : m_family(family)
    , m_data(std::move(data))
    , m_port(port) {}

Result<NetworkAddress> NetworkAddress::fromString(std::string_view address, uint16_t port) {
    IPv4Data v4;
    if (parseIPv4(address, v4.bytes)) {
        return NetworkAddress(AddressFamily::IPv4, std::move(v4), port);
    }

    IPv6Data v6;
    if (parseIPv6(address, v6.bytes)) {
        return NetworkAddress(AddressFamily::IPv6, std::move(v6), port);
    }

    if (address.empty()) {
        return ErrorCode::InvalidArgument;
    }

    HostnameData host;
    host.name = address;
    return NetworkAddress(AddressFamily::Hostname, std::move(host), port);
}

NetworkAddress NetworkAddress::loopback(AddressFamily family) {
    switch (family) {
    case AddressFamily::IPv4: {
        IPv4Data v4 = {{127, 0, 0, 1}};
        return NetworkAddress(AddressFamily::IPv4, std::move(v4), 0);
    }
    case AddressFamily::IPv6: {
        IPv6Data v6 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
        return NetworkAddress(AddressFamily::IPv6, std::move(v6), 0);
    }
    default:
        return loopback(AddressFamily::IPv4);
    }
}

NetworkAddress NetworkAddress::any(AddressFamily family) {
    switch (family) {
    case AddressFamily::IPv4: {
        IPv4Data v4 = {{0, 0, 0, 0}};
        return NetworkAddress(AddressFamily::IPv4, std::move(v4), 0);
    }
    case AddressFamily::IPv6: {
        IPv6Data v6 = {{0}};
        return NetworkAddress(AddressFamily::IPv6, std::move(v6), 0);
    }
    default:
        return any(AddressFamily::IPv4);
    }
}

std::string NetworkAddress::toString() const {
    std::string addrStr;
    switch (m_family) {
    case AddressFamily::IPv4:
        addrStr = formatIPv4(std::get<IPv4Data>(m_data).bytes);
        break;
    case AddressFamily::IPv6:
        addrStr = formatIPv6(std::get<IPv6Data>(m_data).bytes);
        break;
    case AddressFamily::Hostname:
        addrStr = std::get<HostnameData>(m_data).name;
        break;
    }
    if (m_port != 0) {
        addrStr += ':' + std::to_string(m_port);
    }
    return addrStr;
}

bool NetworkAddress::operator==(const NetworkAddress& other) const noexcept {
    if (m_family != other.m_family || m_port != other.m_port)
        return false;
    switch (m_family) {
    case AddressFamily::IPv4:
        return bytesEqual(std::get<IPv4Data>(m_data).bytes,
                          std::get<IPv4Data>(other.m_data).bytes, 4);
    case AddressFamily::IPv6:
        return bytesEqual(std::get<IPv6Data>(m_data).bytes,
                          std::get<IPv6Data>(other.m_data).bytes, 16);
    case AddressFamily::Hostname:
        return std::get<HostnameData>(m_data).name ==
               std::get<HostnameData>(other.m_data).name;
    }
    return false;
}

bool NetworkAddress::operator<(const NetworkAddress& other) const noexcept {
    if (m_family != other.m_family)
        return static_cast<uint8_t>(m_family) < static_cast<uint8_t>(other.m_family);
    if (m_port != other.m_port)
        return m_port < other.m_port;
    switch (m_family) {
    case AddressFamily::IPv4:
        return bytesCompare(std::get<IPv4Data>(m_data).bytes,
                            std::get<IPv4Data>(other.m_data).bytes, 4) < 0;
    case AddressFamily::IPv6:
        return bytesCompare(std::get<IPv6Data>(m_data).bytes,
                            std::get<IPv6Data>(other.m_data).bytes, 16) < 0;
    case AddressFamily::Hostname:
        return std::get<HostnameData>(m_data).name <
               std::get<HostnameData>(other.m_data).name;
    }
    return false;
}

std::size_t NetworkAddress::hash() const noexcept {
    auto h = std::hash<uint8_t>{}(static_cast<uint8_t>(m_family));
    hashCombine(h, std::hash<uint16_t>{}(m_port));
    switch (m_family) {
    case AddressFamily::IPv4: {
        auto& b = std::get<IPv4Data>(m_data).bytes;
        for (auto byte : b)
            hashCombine(h, std::hash<uint8_t>{}(byte));
        break;
    }
    case AddressFamily::IPv6: {
        auto& b = std::get<IPv6Data>(m_data).bytes;
        for (auto byte : b)
            hashCombine(h, std::hash<uint8_t>{}(byte));
        break;
    }
    case AddressFamily::Hostname:
        hashCombine(h, std::hash<std::string>{}(std::get<HostnameData>(m_data).name));
        break;
    }
    return h;
}

std::ostream& operator<<(std::ostream& os, const NetworkAddress& addr) {
    os << addr.toString();
    return os;
}

} // namespace network
} // namespace mbootcore
