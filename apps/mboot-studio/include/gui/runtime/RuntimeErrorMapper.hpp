#pragma once

#include "gui/runtime/RuntimeTypes.hpp"

#include <mbootcore/domain/Error.hpp>

#include <string>

namespace gui {
namespace runtime {

struct RuntimeError {
    mbootcore::ErrorCode coreCode{mbootcore::ErrorCode::Success};
    ErrorSeverity severity{ErrorSeverity::Error};
    std::string title;
    std::string message;
    std::string details;
    std::string source;

    bool isSuccess() const noexcept {
        return coreCode == mbootcore::ErrorCode::Success;
    }
};

class RuntimeErrorMapper {
public:
    static RuntimeError map(mbootcore::ErrorCode code);
    static RuntimeError map(const mbootcore::ErrorInfo& info);
    static RuntimeError map(const std::string& context, mbootcore::ErrorCode code);

    static std::string toString(mbootcore::ErrorCode code);
    static ErrorSeverity severityFor(mbootcore::ErrorCode code);
};

} // namespace runtime
} // namespace gui
