# MBootCore Security

## Current Capabilities

The security module (`lib/include/mbootcore/security/`) provides:

- **SecurityManager**: Policy management and trusted key storage.
  When `MBOOTCORE_ENABLE_CRYPTO=ON` (default), cryptographic operations
  are available through the MbedTLS provider interfaces. When explicitly
  disabled via `MBOOTCORE_ENABLE_CRYPTO=OFF`, all cryptographic operations
  return `NotSupported`.

- **SecureHash**: Data structure for hash values.

- **PermissionSet**: Enum-based permission flag container.

- **InMemoryStorage**: Thread-safe `std::map<std::string, std::string>` in
  `mbootcore::security::testing` namespace. No encryption.

- **Security interfaces** (`interfaces/`):
  - `IHashProvider` — Default: `NotSupportedHashProvider`
  - `ISignatureVerifier` — Default: `NotSupportedSignatureVerifier`
  - `ISecureStorage` — Default: `NotSupportedSecureStorage`
  - `IIntegrityVerifier` — Default: `NotSupportedIntegrityVerifier`

## MbedTLS Backend (Default)

When `MBOOTCORE_ENABLE_CRYPTO=ON` (default) and `CRYPTO_PROVIDER=mbedtls`,
a real cryptographic backend is available via Mbed TLS v4.2.

### Supported Algorithms

| Algorithm | Usage | Status |
|-----------|-------|--------|
| SHA-256 | Hash computation, integrity verification | Production |
| SHA-512 | Hash computation, integrity verification | Production |
| RSA-2048 | Signature verification (PKCS#1 v1.5) | Production |
| RSA-4096 | Signature verification (PKCS#1 v1.5) | Production |
| ECDSA | Signature verification (NIST P-256/P-384) | Production |

### Provider Architecture

```
Runtime / FirmwareValidator
        |
        v
Security Interfaces (IHashProvider, ISignatureVerifier, IIntegrityVerifier)
        |
        v
MbedTLS Provider (lib/src/security/mbedtls/MbedTLS*.cpp)
        |
        v
Mbed TLS v4.2 Library (auto-managed via DependencyManager)
```

Mbed TLS is an implementation detail. No Mbed TLS types appear in public
headers. All Mbed TLS resources use RAII wrappers. All errors are converted
to `Result<T>` / `ErrorCode`. No exceptions cross library boundaries.

### Factory Functions

```cpp
// Create an MbedTLS-backed hash provider
std::unique_ptr<IHashProvider> makeMbedTLSHashProvider();

// Create an MbedTLS-backed signature verifier
std::unique_ptr<ISignatureVerifier> makeMbedTLSSignatureVerifier();

// Create an MbedTLS-backed integrity verifier
std::unique_ptr<IIntegrityVerifier> makeMbedTLSIntegrityVerifier();
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `MBOOTCORE_ENABLE_CRYPTO` | `ON` | Enable cryptographic backend |
| `CRYPTO_PROVIDER` | `mbedtls` | Cryptographic provider name |

### Build Commands

```bash
# With crypto (default, auto-managed)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Without crypto
cmake .. -DMBOOTCORE_ENABLE_CRYPTO=OFF
```

### Configure Output

When crypto is enabled, the configure summary shows:

```
Crypto support: ENABLED (mbedtls-4.2.0)
```

When disabled:

```
Crypto support: DISABLED (MBOOTCORE_ENABLE_CRYPTO=OFF)
```

## Current Limitations

- Signature verification requires a pre-configured public key (PEM or DER).
  There is no certificate chain validation.
- No private key operations (signing). Verification only.
- No hardware security module (HSM) or TPM integration.
- No encrypted secure storage. `MbedTLSSecureStorage` is in-memory only.
- No certificate validation or PKI infrastructure.
- No runtime access control enforcement.
- No audit logging.

## Behaviour When Crypto Is Disabled

When `MBOOTCORE_ENABLE_CRYPTO=OFF`:

- All cryptographic operations return `ErrorCode::NotSupported`.
- `FirmwareValidator::verifyHash()` uses a lightweight XOR checksum
  for firmware integrity checks. This checksum detects accidental data
  corruption (e.g., transmission errors) but provides **no protection
  against intentional malicious modification**. It must not be relied
  upon as a security measure in production environments.
- A runtime warning is emitted (once per process) through the logging
  system when firmware integrity validation is reached, to ensure that
  integrators are aware of the reduced protection level.
- No Mbed TLS library is linked.
- Zero runtime overhead from crypto code.
- The `NotSupported*` default implementations remain in place.

Production deployments **must** use `MBOOTCORE_ENABLE_CRYPTO=ON` (the
default) to obtain SHA-256/SHA-512 hash verification and optional
signature verification via Mbed TLS.

## Security Model

- Mbed TLS is only called from `lib/src/security/mbedtls/*.cpp`.
- Runtime, Workflow, Transport, Protocol, Firmware layers never call
  Mbed TLS directly.
- Dependency direction:
  `Runtime → FirmwareValidator → Security Interfaces → MbedTLS Provider`
- `Result<T>` boundary is preserved: all Mbed TLS failures are converted
  to error codes, never exceptions.
