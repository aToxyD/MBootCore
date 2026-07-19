#pragma once

#ifdef MBOOTCORE_ENABLE_CRYPTO

#include <memory>
#include <mbootcore/security/interfaces/IHashProvider.hpp>

namespace mbootcore { namespace security {

std::unique_ptr<IHashProvider> makeMbedTLSHashProvider();

} }

#endif
