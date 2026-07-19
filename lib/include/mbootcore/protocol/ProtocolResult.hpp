#pragma once

/// \file
/// Semantic alias for protocol-layer result types.
///
/// \see Architecture Note "Semantic Aliases" below.

#include <mbootcore/domain/Error.hpp>

namespace mbootcore::protocol {

// ---------------------------------------------------------------------------
// ProtocolResult<T>
// ---------------------------------------------------------------------------
/// Semantic alias for Result<T> at the protocol layer.
///
/// \tparam T  The success type carried by the result.
///
/// **Purpose**
///
/// `ProtocolResult<T>` communicates architectural intent: a value of this type
/// belongs to the protocol layer.  It makes the calling code self-documenting
/// and distinguishes protocol results from results in other layers (domain,
/// transport, application) without relying on namespace qualifiers alone.
///
/// **Current definition**
///
///     template<typename T>
///     using ProtocolResult = Result<T>;
///
/// At present the alias adds no protocol-specific logic, zero runtime overhead,
/// and zero ABI impact.  Its role is exclusively semantic.
///
/// **Future stability**
///
/// Because every protocol API already returns `ProtocolResult<T>`, any future
/// protocol-wide error policy, wrapping, or instrumentation can be introduced
/// by changing this single alias — without modifying a single protocol
/// implementation.  In this sense `ProtocolResult<T>` serves as a stable
/// abstraction point reserved for the protocol platform.
///
/// **Guidelines**
///
/// - Protocol interface methods MUST return `ProtocolResult<T>`, not `Result<T>`.
/// - Protocol implementation methods SHOULD return `ProtocolResult<T>` for
///   consistency.
/// - Callers outside the protocol layer receive `ProtocolResult<T>` and can
///   treat it as `Result<T>` (the alias is transparent).

template<typename T>
using ProtocolResult = Result<T>;

// ===========================================================================
// Architecture Note: Semantic Aliases
// ===========================================================================
//
// MBootCore uses `using` aliases to express architectural intent — these are
// *semantic aliases*.  They are part of the project's ubiquitous language and
// document which subsystem a type belongs to.
//
// Examples:
//
//   ProtocolResult<T>     — result at the protocol layer
//   (future) TransportResult<T>  — result at the transport layer
//
// A semantic alias is NOT a "temporary compatibility shim".  It should not be
// removed simply because it currently resolves to an existing type.  Removing
// it would erase the intent signal and make the code harder to navigate.
//
// Guidelines:
//
// 1. Use a semantic alias when it improves readability of a subsystem's
//    public API (e.g., `ProtocolResult` inside `protocol/` headers).
// 2. Do NOT replace occurrences of the base type with the alias outside the
//    subsystem — cross-boundary usage should remain explicit.
// 3. If the alias ever needs to diverge from the base type (e.g., adding
//    layer-specific error enrichment), the change is confined to one line.
// 4. Never remove a semantic alias solely because it is "just an alias".
//    Evaluate removal only when the alias no longer communicates meaningful
//    architectural intent.
//
// ===========================================================================

} // namespace mbootcore::protocol
