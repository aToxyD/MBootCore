#pragma once

#ifdef MBOOTCORE_ENABLE_CRYPTO

#include <memory>
#include <mbootcore/security/interfaces/IIntegrityVerifier.hpp>

namespace mbootcore { namespace security {

std::unique_ptr<IIntegrityVerifier> makeMbedTLSIntegrityVerifier();

} }

#endif
