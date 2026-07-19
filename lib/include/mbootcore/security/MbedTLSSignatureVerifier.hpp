#pragma once

#ifdef MBOOTCORE_ENABLE_CRYPTO

#include <memory>
#include <mbootcore/security/interfaces/ISignatureVerifier.hpp>

namespace mbootcore { namespace security {

std::unique_ptr<ISignatureVerifier> makeMbedTLSSignatureVerifier();

} }

#endif
