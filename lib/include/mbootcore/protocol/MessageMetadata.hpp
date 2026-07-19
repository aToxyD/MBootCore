#pragma once

#include "MessageId.hpp"
#include "TransactionId.hpp"

namespace mbootcore::protocol {

struct MessageMetadata {
    MessageId       messageId;
    TransactionId   transactionId{0};

    bool operator==(const MessageMetadata& other) const
    {
        return messageId == other.messageId && transactionId == other.transactionId;
    }
    bool operator!=(const MessageMetadata& other) const
    {
        return !(*this == other);
    }
};

} // namespace mbootcore::protocol
