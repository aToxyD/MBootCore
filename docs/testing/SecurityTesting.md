# Security & Robustness Testing

This document describes MBootCore's security testing infrastructure,
including sanitizer support, parser regression corpus, fuzz harnesses,
and robustness regression tests.

**Security Audit Complete (2026-07-16):** Full arithmetic hardening audit
of all binary parsers. See `docs/internal/SecurityAuditReport.md` for
detailed findings, fixes, and exclusions.

---

## Philosophy

All parsers in MBootCore handle untrusted, potentially hostile input from
external devices. A malformed packet must never:

- Crash the application
- Cause undefined behavior (buffer overflow, use-after-free, etc.)
- Leak memory
- Hang indefinitely

Every parser returns `Result<T>` — errors are values, not exceptions.
Security tests verify this contract under adversarial conditions.

---

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MBOOTCORE_BUILD_SECURITY_TESTS` | ON | Build robustness regression tests |
| `MBOOTCORE_BUILD_FUZZERS` | OFF | Build coverage-guided fuzz harnesses |
| `MBOOTCORE_ENABLE_ASAN` | OFF | Enable AddressSanitizer (Debug only) |
| `MBOOTCORE_ENABLE_UBSAN` | OFF | Enable UBSan (Debug only) |
| `MBOOTCORE_ENABLE_TSAN` | OFF | Enable ThreadSanitizer (Debug only) |

### CMake Usage

```bash
# Standard build with security tests
cmake -B build -DMBOOTCORE_BUILD_SECURITY_TESTS=ON
cmake --build build
ctest --test-dir build -R "security|robustness"

# Sanitizer build (ASan + UBSan)
cmake -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMBOOTCORE_ENABLE_ASAN=ON \
    -DMBOOTCORE_ENABLE_UBSAN=ON
cmake --build build-asan
ctest --test-dir build-asan

# ThreadSanitizer build
cmake -B build-tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMBOOTCORE_ENABLE_TSAN=ON
cmake --build build-tsan
ctest --test-dir build-tsan

# Fuzz targets (requires Clang with libFuzzer)
cmake -B build-fuzz \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DMBOOTCORE_BUILD_FUZZERS=ON \
    -DMBOOTCORE_ENABLE_ASAN=ON
cmake --build build-fuzz
```

---

## Sanitizers

### AddressSanitizer (ASan)

Detects:
- Heap buffer overflow/underflow
- Stack buffer overflow
- Use-after-free
- Use-after-return
- Double-free
- Memory leaks

Options set for sanitizer runs:
```
ASAN_OPTIONS=detect_stack_use_after_return=1:strict_init_order=1:halt_on_error=1
```

### UndefinedBehaviorSanitizer (UBSan)

Detects:
- Signed integer overflow
- Unsigned integer wraparound
- Null pointer dereference
- Misaligned pointer access
- Out-of-bounds array index
- Invalid shift exponent
- Float-to-integer truncation

Options set for sanitizer runs:
```
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

### ThreadSanitizer (TSan)

Detects:
- Data races between threads
- Deadlocks
- Misuse of POSIX threads primitives
- Thread-unsafe standard library usage

Options set for sanitizer runs:
```
TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1
```

### Compiler Support

| Compiler | ASan | UBSan | TSan |
|----------|------|-------|------|
| GCC 11+  | Yes  | Yes   | Yes  |
| Clang 15+| Yes  | Yes   | Yes  |
| MSVC     | ASan only | No | No |

Sanitizers are silently ignored on unsupported compilers.

### Mutual Exclusion

| Combination | Status |
|-------------|--------|
| ASan + UBSan | Supported — default sanitizer configuration |
| ASan + TSan | **Forbidden** — CMake FATAL_ERROR |
| TSan + UBSan | Warning — not officially tested, may vary |

### Sanitizer Propagation Policy

When sanitizer instrumentation is enabled, compile options are applied with
`PRIVATE` scope to each target.

For `STATIC_LIBRARY` targets, sanitizer link options are applied with `PUBLIC`
scope. A static library does not perform the final link step, so executables
that consume the library must also link the corresponding sanitizer runtime.
Using `PUBLIC` propagates the required linker options automatically during
sanitizer-enabled development builds.

`OBJECT_LIBRARY` targets receive compile options only, because they do not
participate in the link stage.

---

## Parser Regression Corpus

Location: `tests/corpus/`

### Structure

```
corpus/
  sahara/      {valid,invalid,truncated,corrupted,boundary}/
  firehose/    {valid,invalid,truncated,corrupted,boundary}/
  gpt/         {valid,invalid,truncated,corrupted,boundary}/
  elf/         {valid,invalid,truncated,corrupted,boundary}/
  firmware/    {valid,invalid,truncated,corrupted,boundary}/
```

### Sample Categories

| Category | Expected Result | Description |
|----------|-----------------|-------------|
| `valid/` | `Result::isOk()` | Correctly formed input, valid fields |
| `invalid/` | `Result::isError()` | Malformed structure, wrong magic, bad fields |
| `truncated/` | `Result::isError()` | Truncated mid-packet or mid-field |
| `corrupted/` | `Result::isError()` | Valid structure with corrupted bytes |
| `boundary/` | `Result` (any) | Edge cases: zero-length, max values, empty |

### Adding Samples

1. Place binary/XML sample in the correct subdirectory
2. Name descriptively: `<parser>-<description>.bin`
3. Write a corresponding test in `ParserRobustnessTest.cpp`
4. Verify sample passes under ASan + UBSan

---

## Fuzz Targets

Located in `tests/fuzz/`. Built only when `MBOOTCORE_BUILD_FUZZERS=ON`.

| Target | Parser | Input Type |
|--------|--------|------------|
| `fuzz_sahara_parser` | `SaharaPacketParser` | Binary |
| `fuzz_firehose_xml` | `FirehoseXmlEngine` | String/XML |
| `fuzz_elf_parser` | `ElfParser` | Binary |

### Requirements

- Clang with `-fsanitize=fuzzer` (libFuzzer)
- AddressSanitizer recommended alongside fuzzer

### Running Fuzzers

```bash
# Build fuzzer
cmake -B build-fuzz \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DMBOOTCORE_BUILD_FUZZERS=ON \
    -DMBOOTCORE_ENABLE_ASAN=ON
cmake --build build-fuzz --target fuzz_sahara_parser

# Run with corpus seed directory
./build-fuzz/fuzz_sahara_parser tests/corpus/sahara/valid/

# Run for 60 seconds
./build-fuzz/fuzz_sahara_parser -max_total_time=60
```

### Fuzzer Contract

Each harness must:
1. Accept arbitrary input without crashing
2. Never allocate more than 1 MB for test data
3. Return 0 on all paths
4. Produce no memory errors under ASan

---

## Robustness Regression Tests

Location: `tests/security/ParserRobustnessTest.cpp`

### Test Categories

For each parser, tests cover:

| Test | Description |
|------|-------------|
| Empty input | Zero bytes / empty string |
| Single byte | Minimal possible input |
| Truncated data | Valid start, incomplete middle |
| Oversized data | 64KB+ random payload |
| Random payloads | Various sizes, all-zero, all-0xFF |
| Corrupted headers | Flip each bit in critical fields |
| Integer overflow | Near-UINT32_MAX length/count fields |
| Unknown commands | Out-of-range command IDs |
| Deep nesting | XML: 200 levels deep |
| Special characters | Invalid UTF-8, null bytes, injection |
| State leakage | Sequential parse calls don't affect each other |

### Expected Behavior

Every test asserts:
```cpp
REQUIRE((result.isOk() || result.isError()));
```

This verifies the parser returns `Result<T>` on all paths — no crash,
no throw, no UB.

---

## Release Integration

### Local Verification (before tagging)

All security tests, sanitizer runs, and fuzzing are performed locally
before creating a release tag. The tag is the approval gate.

### What Runs Where

| Stage | Tests | Sanitizers | Fuzzers |
|-------|-------|------------|---------|
| Local (pre-tag) | All unit + security tests | ASan + UBSan + TSan | Fuzz harnesses |
| GitHub Release | Package build only | None | None |

---

## Reproducing Crashes

### From local runs

1. Check local logs for the failed test name and sanitizer output
2. Reproduce locally:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug \
       -DMBOOTCORE_ENABLE_ASAN=ON -DMBOOTCORE_ENABLE_UBSAN=ON
   cmake --build build
   ./build/parser_robustness_test  # or the specific test
   # For TSan issues:
   cmake -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DMBOOTCORE_ENABLE_TSAN=ON
   cmake --build build-tsan
   ./build-tsan/parser_robustness_test
   ```

### From Fuzz Target

1. Save the crashing input to a file (e.g., `crash.bin`)
2. Reproduce:
   ```bash
   ./build-fuzz/fuzz_sahara_parser crash.bin
   ```

### Minimizing Corpus

Use libFuzzer's `minimize` flag:
```bash
./fuzz_target minimized_crash.bin -minimize_crash=1
```

---

## Expected Runtime

| Component | Time |
|-----------|------|
| Full robustness test suite | < 5 seconds |
| Parser corpus execution | < 2 seconds |
| Individual test case | < 50ms |
| Fuzzer (1 hour run) | 3600 seconds |

---

## Security Testing Checklist

- [x] All parsers return `Result<T>` on adversarial input
- [ ] No crashes under ASan
- [ ] No UB under UBSan
- [ ] No memory leaks under LSan
- [x] All corpus samples execute successfully
- [ ] Fuzz harnesses compile on Clang
- [x] Security tests run locally before release tagging
- [x] Documentation is current
- [x] Security audit completed (2026-07-16) — see `docs/internal/SecurityAuditReport.md`

---

## Crypto Fallback Warning

When `MBOOTCORE_ENABLE_CRYPTO=OFF`, `FirmwareValidator` falls back to a
lightweight XOR checksum for firmware integrity verification. This checksum
detects accidental data corruption but provides **no protection against
intentional malicious modification**.

A runtime warning is emitted (once per process) through the logging system
the first time firmware integrity validation is performed, to ensure
integrators are aware of the reduced protection level.

### What the XOR checksum covers

| Threat | Detected? |
|--------|-----------|
| Accidental bit-flip during transfer | Yes (some cases) |
| Truncated firmware image | Yes |
| Intentional byte replacement | No (high collision rate) |
| Targeted payload injection | No |
| Replay of modified firmware | No |

### Testing the warning

The `crypto_fallback_test` in `tests/security/CryptoFallbackTest.cpp`
verifies:

- **Crypto enabled:** No warning is emitted during firmware validation.
- **Crypto disabled:** Warning is emitted exactly once per process.
  Repeated `validateIntegrity()` calls do not produce duplicate warnings.
  The warning is absent when no logger is attached.

### Local coverage

| Stage | Crypto | Warning test |
|-------|--------|-------------|
| Local (pre-tag) | ON | Verifies no warning emitted |
| Local (sanitizers) | ON | Verifies no warning under ASan+UBSan |
