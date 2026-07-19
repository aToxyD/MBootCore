#pragma once

#include "MessageMetadata.hpp"
#include "Payload.hpp"
#include "StatusCode.hpp"

namespace mbootcore::protocol {

class Response {
public:
    Response() = default;

    Response(MessageMetadata metadata, StatusCode status, Payload payload)
        : m_metadata(metadata)
        , m_status(status)
        , m_payload(std::move(payload))
    {}

    const MessageMetadata& metadata() const noexcept { return m_metadata; }
    StatusCode             status()   const noexcept { return m_status;   }
    const Payload&         payload()  const noexcept { return m_payload;  }

private:
    MessageMetadata m_metadata;
    StatusCode      m_status;
    Payload         m_payload;
};

} // namespace mbootcore::protocol
