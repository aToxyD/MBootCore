#pragma once

/**
 * @brief Re-exports transport type definitions from the domain layer.
 *
 * All core transport types (TransportType, TransportState,
 * TransportConfig, TransportStatistics, etc.) are defined in
 * mbootcore::domain::TransportTypes.hpp and re-exported here
 * for convenience under the mbootcore::transport namespace.
 */

#include <mbootcore/domain/TransportTypes.hpp>
