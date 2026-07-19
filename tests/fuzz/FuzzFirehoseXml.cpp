// Fuzz harness for Firehose XML engine.
// Requires libFuzzer: clang++ -fsanitize=fuzzer,address -std=c++17 ...

#ifdef __has_include
#if __has_include(<fuzzer/FuzzedDataProvider.h>)
#include <fuzzer/FuzzedDataProvider.h>
#define MBOOTCORE_HAS_LIBFUZZER 1
#endif
#endif

#ifndef MBOOTCORE_HAS_LIBFUZZER
int main() { return 0; }
#else

#include "mbootcore/core/protocols/firehose/FirehoseXmlEngine.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 1024 * 1024) return 0;
    std::string input(reinterpret_cast<const char*>(data), size);
    auto result = mbootcore::FirehoseXmlEngine::parse(input);
    (void)result;
    return 0;
}

#endif
