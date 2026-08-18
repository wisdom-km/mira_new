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
};

struct Error {
    ErrorCode code = ErrorCode::Internal;
    std::string technicalMessage;
    std::string userMessage;

    static Error Make(ErrorCode code, std::string technicalMessage, std::string userMessage);
};

} // namespace DirectorDesk::Core
