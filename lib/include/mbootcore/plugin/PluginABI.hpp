#pragma once

#include <cstdint>

#if defined(_WIN32)
  #if defined(MBOOTCORE_PLUGIN_BUILD)
    #define MBOOTCORE_PLUGIN_EXPORT __declspec(dllexport)
  #else
    #define MBOOTCORE_PLUGIN_EXPORT __declspec(dllimport)
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define MBOOTCORE_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
  #define MBOOTCORE_PLUGIN_EXPORT
#endif

namespace mbootcore {
namespace plugin {

constexpr uint32_t PluginABIVersion = 1;

using PluginCreateFunc  = class IPlugin* (*)();
using PluginDestroyFunc = void (*)(class IPlugin*);
using PluginVersionFunc = uint32_t (*)();

constexpr const char* PluginCreateSymbol  = "mbootcore_plugin_create";
constexpr const char* PluginDestroySymbol = "mbootcore_plugin_destroy";
constexpr const char* PluginVersionSymbol = "mbootcore_plugin_abi_version";

} // namespace plugin
} // namespace mbootcore
