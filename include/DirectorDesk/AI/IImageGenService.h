// IImageGenService: Public or internal interface for the DirectorDesk AI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/AI/Types.h"
#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::AI {

class IImageGenService {
public:
    virtual ~IImageGenService() = default;
    virtual Core::Result<std::string> Submit(const ImageGenRequest& request) = 0;
    virtual Core::Result<void> Cancel(const std::string& jobId) = 0;
    virtual Core::Result<GenProgress> Progress(const std::string& jobId) const = 0;
    virtual Core::Result<ImageGenResult> TakeResult(const std::string& jobId) = 0;
};

} // namespace DirectorDesk::AI
