// NullServices: Implementation for the DirectorDesk AI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/AI/NullServices.h"

namespace DirectorDesk::AI {
namespace {

Core::Error Unavailable(const char* technical) {
    return Core::Error::Make(Core::ErrorCode::Unsupported, technical,
                             "当前版本未接入生成服务");
}

} // namespace

Core::Result<std::string> NullImageGenService::Submit(const ImageGenRequest&) {
    return Core::Result<std::string>::Fail(Unavailable("null image gen"));
}

Core::Result<void> NullImageGenService::Cancel(const std::string&) {
    return Core::Result<void>::Fail(Unavailable("null image gen cancel"));
}

Core::Result<GenProgress> NullImageGenService::Progress(const std::string&) const {
    return Core::Result<GenProgress>::Fail(Unavailable("null image gen progress"));
}

Core::Result<ImageGenResult> NullImageGenService::TakeResult(const std::string&) {
    return Core::Result<ImageGenResult>::Fail(Unavailable("null image gen result"));
}

Core::Result<std::string> NullVideoGenService::Submit(const VideoGenRequest&) {
    return Core::Result<std::string>::Fail(Unavailable("null video gen"));
}

Core::Result<void> NullVideoGenService::Cancel(const std::string&) {
    return Core::Result<void>::Fail(Unavailable("null video gen cancel"));
}

Core::Result<GenProgress> NullVideoGenService::Progress(const std::string&) const {
    return Core::Result<GenProgress>::Fail(Unavailable("null video gen progress"));
}

Core::Result<VideoGenResult> NullVideoGenService::TakeResult(const std::string&) {
    return Core::Result<VideoGenResult>::Fail(Unavailable("null video gen result"));
}

std::unique_ptr<IImageGenService> CreateNullImageGenService() {
    return std::make_unique<NullImageGenService>();
}

std::unique_ptr<IVideoGenService> CreateNullVideoGenService() {
    return std::make_unique<NullVideoGenService>();
}

} // namespace DirectorDesk::AI
