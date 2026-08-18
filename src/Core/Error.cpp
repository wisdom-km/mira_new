// Error: Implementation for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Core/Error.h"

namespace DirectorDesk::Core {

Error Error::Make(ErrorCode code, std::string technicalMessage, std::string userMessage) {
    Error error;
    error.code = code;
    error.technicalMessage = std::move(technicalMessage);
    error.userMessage = std::move(userMessage);
    return error;
}

} // namespace DirectorDesk::Core
