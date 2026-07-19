#pragma once

#include "CapabilityId.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace mbootcore::protocol {

class CapabilitySet {
public:
    using value_type      = CapabilityId;
    using const_iterator  = std::vector<CapabilityId>::const_iterator;
    using const_reference = const CapabilityId&;
    using size_type       = std::size_t;

    void add(CapabilityId id)
    {
        if (!contains(id))
            m_capabilities.push_back(id);
    }

    bool contains(CapabilityId id) const
    {
        return std::find(m_capabilities.begin(), m_capabilities.end(), id)
               != m_capabilities.end();
    }

    const_iterator begin() const noexcept { return m_capabilities.begin(); }
    const_iterator end()   const noexcept { return m_capabilities.end(); }

    size_type size()  const noexcept { return m_capabilities.size(); }
    bool      empty() const noexcept { return m_capabilities.empty(); }

    bool operator==(const CapabilitySet& other) const { return m_capabilities == other.m_capabilities; }
    bool operator!=(const CapabilitySet& other) const { return !(*this == other); }

private:
    std::vector<CapabilityId> m_capabilities;
};

} // namespace mbootcore::protocol
