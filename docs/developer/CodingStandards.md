# Coding Standards

This document provides a summary of coding standards for MBootCore. The
authoritative source is [AGENTS.md §6](../internal/AGENTS.md).

## Key Rules

- **Language:** C++17, no third-party libraries beyond the approved set
- **Compiler:** Zero warnings with `-Wall -Wextra -Wpedantic`
- **Naming:** Classes `PascalCase`, methods `camelCase`, constants `SCREAMING_SNAKE_CASE`, files `PascalCase.hpp`
- **Memory:** Smart pointers only, no raw `new`/`delete`
- **Errors:** Return `Result<T>`, no exceptions in library code
- **Thread safety:** Document thread-safety properties, use lock hierarchy

## Full Specification

See [AGENTS.md §6 — Coding Standards](../internal/AGENTS.md) for the complete
specification including memory management rules, error handling patterns,
platform abstraction guidelines, performance targets, and documentation
requirements.
