# Security Architecture

## Current State

MBootCore provides interface-based security operations. When
`MBOOTCORE_ENABLE_CRYPTO=ON` (default) with the MbedTLS provider, SHA-256,
SHA-512, RSA, and ECDSA operations are available. When
`MBOOTCORE_ENABLE_CRYPTO=OFF`, all cryptographic operations
return `NotSupported`.

## Design

### Dependency Injection

Security operations use interface-based dependency injection:

```txt
Consumer → ISignatureVerifier → NotSupportedSignatureVerifier (default)
                                → MbedTLSSignatureVerifier

Consumer → IHashProvider → NotSupportedHashProvider (default)
                          → MbedTLSHashProvider

Consumer → ISecureStorage → NotSupportedSecureStorage (default)
                           → MbedTLSSecureStorage

Consumer → IIntegrityVerifier → NotSupportedIntegrityVerifier (default)
                               → MbedTLSIntegrityVerifier
```

### CMake Feature Gate

`MBOOTCORE_ENABLE_CRYPTO` (default ON):

- **ON**: Mbed TLS provider is the primary production implementation.
  SHA-256, SHA-512, RSA (2048/4096), and ECDSA (NIST P-256/P-384)
  operations are available. Auto-managed, no host dependencies.
- **OFF**: All provider implementations return `NotSupported`. No crypto
  library dependencies required.

## Interfaces

Located at `lib/include/mbootcore/security/interfaces/`:

| Interface | Method | Default Implementation |
|-----------|--------|----------------------|
| `IHashProvider` | `hash(data, algo)` => `Result<vector<uint8_t>>` | `NotSupportedHashProvider` |
| `ISignatureVerifier` | `verify(data, signature)` => `Result<SignatureVerificationResult>` | `NotSupportedSignatureVerifier` |
| | `verifyFile(path)` => `Result<SignatureVerificationResult>` | |
| `ISecureStorage` | `store(key, value)` => `Result<void>` | `NotSupportedSecureStorage` |
| | `retrieve(key)` => `Result<string>` | |
| | `remove(key)` => `Result<void>` | |
| | `exists(key)` => `Result<bool>` | |
| | `clear()` => `Result<void>` | |
| `IIntegrityVerifier` | `isTampered(path)` => `Result<bool>` | `NotSupportedIntegrityVerifier` |
| | `verifyIntegrity(path)` => `Result<bool>` | |

## Related Components

| Component | Status |
|-----------|--------|
| `SecurityManager` | Policy management only. Crypto/verify methods return NotSupported. |
| `PermissionSet` | Permission flag container. No runtime enforcement. |
| `InMemoryStorage` | `testing` namespace. Plain `std::map` with mutex. Not secure. |
| `SecurityPolicy` | Defaults set to `None`/`false`/`NotSupported`. |

## Threat Model

Since no security operations are functional when crypto is disabled, the
current threat model treats the library as **untrusted for security-sensitive
contexts**:

- No signature verification => package authenticity is not guaranteed.
- No hash verification => data integrity is not guaranteed.
- No secure storage => secrets stored via InMemoryStorage are plaintext in
  process memory.
- No tamper detection => file system contents are not validated.

## Building with Crypto

```bash
# Crypto enabled (default) — MbedTLS auto-managed
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Crypto disabled — all operations return NotSupported
cmake .. -DMBOOTCORE_ENABLE_CRYPTO=OFF
```

Mbed TLS v4.2 is auto-downloaded by the dependency manager, SHA256-verified,
and statically built. No host Perl or system crypto library installation required.
