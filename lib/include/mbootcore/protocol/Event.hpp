#pragma once

#include "MessageMetadata.hpp"
#include "Payload.hpp"

namespace mbootcore::protocol {

class Event {
public:
    Event() = default;

    Event(MessageMetadata metadata, Payload payload)
        : m_metadata(metadata)
        , m_payload(std::move(payload))
    {}

    const MessageMetadata& metadata() const noexcept { return m_metadata; }
    const Payload&         payload()  const noexcept { return m_payload;  }

private:
    MessageMetadata m_metadata;
    Payload         m_payload;
};

} // namespace mbootcore::protocol
