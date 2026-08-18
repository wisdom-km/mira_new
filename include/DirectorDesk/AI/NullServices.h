#pragma once

#include "DirectorDesk/AI/IImageGenService.h"
#include "DirectorDesk/AI/IVideoGenService.h"

#include <memory>

namespace DirectorDesk::AI {

class NullImageGenService final : public IImageGenService {
public:
    Core::Result<std::string> Submit(const ImageGenRequest& request) override;
    Core::Result<void> Cancel(const std::string& jobId) override;
    Core::Result<GenProgress> Progress(const std::string& jobId) const override;
    Core::Result<ImageGenResult> TakeResult(const std::string& jobId) override;
};

class NullVideoGenService final : public IVideoGenService {
public:
    Core::Result<std::string> Submit(const VideoGenRequest& request) override;
    Core::Result<void> Cancel(const std::string& jobId) override;
    Core::Result<GenProgress> Progress(const std::string& jobId) const override;
    Core::Result<VideoGenResult> TakeResult(const std::string& jobId) override;
};

[[nodiscard]] std::unique_ptr<IImageGenService> CreateNullImageGenService();
[[nodiscard]] std::unique_ptr<IVideoGenService> CreateNullVideoGenService();

} // namespace DirectorDesk::AI
