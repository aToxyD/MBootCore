#pragma once

#ifdef MBOOTCORE_ENABLE_CRYPTO

#include <string>
#include <mbootcore/security/interfaces/ISignatureVerifier.hpp>

namespace mbootcore { namespace security {

void setPublicKeyPem(ISignatureVerifier& verifier, const std::string& pem);

} }

#endif
