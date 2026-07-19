#pragma once

#include <mbootcore/domain/Types.hpp>

#include <cstddef>

namespace mbootcore::protocol {

class Packet {
public:
    Packet() = default;

    explicit Packet(ByteBuffer data)
        : m_data(std::move(data))
    {}

    const ByteBuffer& data() const noexcept { return m_data; }
    ByteBuffer&       data() noexcept       { return m_data; }

    bool     empty() const noexcept { return m_data.empty(); }
    std::size_t size() const noexcept { return m_data.size(); }
    void     clear()                { m_data.clear(); }

    bool operator==(const Packet& other) const { return m_data == other.m_data; }
    bool operator!=(const Packet& other) const { return m_data != other.m_data; }

private:
    ByteBuffer m_data;
};

} // namespace mbootcore::protocol
