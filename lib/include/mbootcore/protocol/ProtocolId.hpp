#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace mbootcore::protocol {

class ProtocolId {
public:
    explicit ProtocolId(std::string name)
        : m_name(std::move(name))
    {}

    const std::string& name() const noexcept { return m_name; }

    bool operator==(const ProtocolId& other) const { return m_name == other.m_name; }
    bool operator!=(const ProtocolId& other) const { return m_name != other.m_name; }
    bool operator<(const ProtocolId& other) const  { return m_name < other.m_name; }

private:
    std::string m_name;
};

} // namespace mbootcore::protocol

namespace std {

template<>
struct hash<mbootcore::protocol::ProtocolId> {
    size_t operator()(const mbootcore::protocol::ProtocolId& id) const noexcept {
        return hash<std::string>{}(id.name());
    }
};

} // namespace std
