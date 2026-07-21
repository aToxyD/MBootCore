# Golden Vector Policy

**Document Version:** 1.0
**Last Updated:** 2026-07-11

> Policy governing golden vector creation, maintenance, and validation.

---

## Definition

A **golden vector** is a known-good input/output pair for a protocol operation.
It consists of:

1. A serialized byte sequence (the wire-format representation)
2. The expected deserialized fields
3. The expected response (if applicable)

## Sources

| Source | Quality | Example |
|--------|---------|---------|
| Real device capture | Highest | Packet captured via USB sniffing during actual flashing |
| Spec document | High | Bytes defined by vendor protocol specification |
| Synthetic construction | Lower | Bytes constructed to match known wire format rules |

## Requirements by Maturity State

| State | Min Vectors | Source | Byte-exact? |
|-------|-------------|--------|-------------|
| Experimental | 1 | Any | No |
| Scaffold | 5 | Synthetic or spec | Recommended |
| Preview | 10 | Real device + synthetic | Yes for real captures |
| Production | 20 | Real device captures | Yes |

## Test Pattern

Every golden vector test must:

1. Construct the expected packet from known fields
2. Serialize to bytes
3. Verify byte-exact match (for real captures)
4. Parse the bytes back
5. Verify all fields match the original construction
6. Report timing information

```cpp
SECTION("testHelloRoundTrip") {
    // 1. Construct
    HelloPacket original(2, 2, 4096, 0);

    // 2. Serialize
    auto buffer = serializer.serialize(original);

    // 3. Byte verification (if applicable)
    REQUIRE(buffer.size() == expectedBytes.size());
    REQUIRE(buffer == expectedBytes);

    // 4. Parse back
    auto parsed = parser.parse(buffer);

    // 5. Verify fields
    REQUIRE(parsed->version == 2);
    REQUIRE(parsed->versionSupported == 2);
    REQUIRE(parsed->cmdPacketLength == 4096);
    REQUIRE(parsed->mode == 0);
}
```

## Golden Vector Framework

The `GoldenVectorTest` helper framework in
`tests/virtual/GoldenVectorFramework.hpp` provides:

- `GoldenVector<T>` — typed vector with input, expected output, metadata
- `GoldenVectorSuite<T>` — collection of vectors with batch execution
- `GoldenVectorResult` — execution result with timing and pass/fail
- `runGoldenVectorSuite()` — runs all vectors, reports summary

See `tests/virtual/GoldenVectorFramework.hpp` for the full API.

## Maintenance

- New vectors must be added when real device captures become available
- Vectors must not be removed without architectural review
- Synthetic vectors must be clearly annotated as synthetic
- Timing information is informational only (not asserted)
