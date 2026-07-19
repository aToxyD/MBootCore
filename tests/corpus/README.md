# Parser Regression Corpus

Binary test samples organized by parser for regression testing.
Each subdirectory corresponds to a parser module.

## Directory Structure

```
corpus/
  sahara/      Sahara binary packet parser
  firehose/    Firehose XML engine
  gpt/         GPT partition table parser
  elf/         ELF binary parser
  firmware/    Firmware package reader
```

## Subdirectory Conventions

| Directory   | Expected parser behavior                              |
|-------------|-------------------------------------------------------|
| `valid/`    | Parser returns `Result::isOk()` with correct output   |
| `invalid/`  | Parser returns `Result::isError()` — no crash         |
| `truncated/`| Parser returns `Result::isError()` — no crash         |
| `corrupted/`| Parser returns `Result::isError()` — no crash         |
| `boundary/` | Parser returns `Result` — handles edge case safely    |

## Adding New Samples

1. Place the binary sample in the correct subdirectory.
2. Name it descriptively: `<description>.bin` for binary, `<description>.xml` for XML.
3. Every sample **must** be exercised by a corresponding test in
   `tests/security/ParserRobustnessTest.cpp`.
4. A parser must **never crash, throw, or produce undefined behavior** on any
   sample in this corpus.
5. Valid samples must parse successfully and match known-good output.

## Usage

The corpus is consumed by:
- `ParserRobustnessTest` — deterministic regression tests
- Fuzz target seed inputs (fuzzers load corpus files as starting seeds)
- Sanitizer validation runs — all samples run under ASan/UBSan

## Expected Runtime

Full corpus execution: < 5 seconds (all parsers combined).
Individual sample: < 50ms.
