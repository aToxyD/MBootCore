// Fuzz harness for Sahara packet parser.
// Requires libFuzzer: clang++ -fsanitize=fuzzer,address -std=c++17 ...

#ifdef __has_include
#if __has_include(<fuzzer/FuzzedDataProvider.h>)
#include <fuzzer/FuzzedDataProvider.h>
#define MBOOTCORE_HAS_LIBFUZZER 1
#endif
#endif

#ifndef MBOOTCORE_HAS_LIBFUZZER
// Stub: compile succeeds but main() is a no-op
int main() { return 0; }
#else

#include "mbootcore/core/protocols/sahara/SaharaPacketParser.hpp"

static mbootcore::SaharaPacketParser g_parser;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 1024 * 1024) return 0;
    mbootcore::ByteBuffer buf(data, data + size);
    auto result = g_parser.parse(buf);
    (void)result;
    return 0;
}

#endif
