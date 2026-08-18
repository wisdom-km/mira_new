// Error: Public or internal interface for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <string>

namespace DirectorDesk::Core {

enum class ErrorCode {
    Ok = 0,
    InvalidArgument,
    NotFound,
    IoFailure,
    AlreadyInitialized,
    NotInitialized,
    Internal,
    ParseFailure,
    Unsupported,
    Cancelled,
};

struct Error {
    ErrorCode code = ErrorCode::Internal;
    std::string technicalMessage;
    std::string userMessage;

    static Error Make(ErrorCode code, std::string technicalMessage, std::string userMessage);
};

} // namespace DirectorDesk::Core
