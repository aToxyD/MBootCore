#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

namespace mbootcore::protocol {

class Payload {
public:
    using value_type = std::byte;

    Payload() = default;

    static Payload copy(const void* data, std::size_t size)
    {
        Payload p;
        if (data && size > 0) {
            p.m_data.resize(size);
            std::memcpy(p.m_data.data(), data, size);
        }
        return p;
    }

    const value_type* data()  const noexcept { return m_data.data(); }
    std::size_t       size()  const noexcept { return m_data.size(); }
    bool              empty() const noexcept { return m_data.empty(); }

    const value_type* begin() const noexcept { return m_data.data(); }
    const value_type* end()   const noexcept { return m_data.data() + m_data.size(); }

    bool operator==(const Payload& other) const { return m_data == other.m_data; }
    bool operator!=(const Payload& other) const { return m_data != other.m_data; }

private:
    std::vector<value_type> m_data;
};

} // namespace mbootcore::protocol
