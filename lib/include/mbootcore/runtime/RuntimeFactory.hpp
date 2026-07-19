#pragma once

#include <mbootcore/runtime/Runtime.hpp>
#include <mbootcore/runtime/RuntimeBuilder.hpp>

namespace mbootcore {
namespace runtime {

class RuntimeFactory {
public:
    static Runtime createDefault();
    static Runtime createTesting();
    static Runtime createMinimal();
    static Runtime createCLI();
    static Runtime createGUI();
    static Runtime createEmbedded();
};

} // namespace runtime
} // namespace mbootcore
