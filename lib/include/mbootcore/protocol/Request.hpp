#pragma once

#include "Command.hpp"
#include "MessageMetadata.hpp"
#include "Payload.hpp"

namespace mbootcore::protocol {

class Request {
public:
    Request() = default;

    Request(MessageMetadata metadata, Command command, Payload payload)
        : m_metadata(metadata)
        , m_command(command)
        , m_payload(std::move(payload))
    {}

    const MessageMetadata& metadata() const noexcept { return m_metadata; }
    const Command&         command()  const noexcept { return m_command;  }
    const Payload&         payload()  const noexcept { return m_payload;  }

private:
    MessageMetadata m_metadata;
    Command         m_command;
    Payload         m_payload;
};

} // namespace mbootcore::protocol
