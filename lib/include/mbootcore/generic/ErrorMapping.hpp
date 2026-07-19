#pragma once

#include "mbootcore/domain/Error.hpp"

#include <cstdint>
#include <string_view>

namespace mbootcore {

inline constexpr ErrorCode genericErrorFromCode(uint32_t protocolCode) noexcept {
    switch (protocolCode) {
        case 0:  return ErrorCode::Success;
        case 1:  return ErrorCode::InvalidArgument;
        case 2:  return ErrorCode::ProtocolMismatch;
        case 3:  return ErrorCode::InvalidPacket;
        case 4:  return ErrorCode::UnexpectedPacket;
        case 5:  return ErrorCode::InvalidState;
        case 6:  return ErrorCode::TransportTimeout;
        case 7:  return ErrorCode::TransportDisconnected;
        case 8:  return ErrorCode::NotSupported;
        case 9:  return ErrorCode::Cancelled;
        default: return ErrorCode::Unknown;
    }
}

} // namespace mbootcore
