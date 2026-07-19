#pragma once

#include "CommandId.hpp"

#include <string_view>

namespace mbootcore::protocol {

class Command {
public:
    CommandId       id;
    std::string_view name;
    std::string_view description;
};

} // namespace mbootcore::protocol
