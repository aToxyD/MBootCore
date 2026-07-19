#pragma once

#ifdef MBOOTCORE_ENABLE_CRYPTO

#include <memory>
#include <mbootcore/security/interfaces/ISecureStorage.hpp>

namespace mbootcore { namespace security {

std::unique_ptr<ISecureStorage> makeMbedTLSSecureStorage();

} }

#endif
