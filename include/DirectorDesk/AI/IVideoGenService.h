#pragma once

#include "DirectorDesk/AI/Types.h"
#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::AI {

class IVideoGenService {
public:
    virtual ~IVideoGenService() = default;
    virtual Core::Result<std::string> Submit(const VideoGenRequest& request) = 0;
    virtual Core::Result<void> Cancel(const std::string& jobId) = 0;
    virtual Core::Result<GenProgress> Progress(const std::string& jobId) const = 0;
    virtual Core::Result<VideoGenResult> TakeResult(const std::string& jobId) = 0;
};

} // namespace DirectorDesk::AI
