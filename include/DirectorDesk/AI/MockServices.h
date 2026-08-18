#pragma once

#include "DirectorDesk/AI/IImageGenService.h"
#include "DirectorDesk/AI/IVideoGenService.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace DirectorDesk::AI {

class MockImageGenService final : public IImageGenService {
public:
    void FailNext(Core::Error error);
    void Pump();

    Core::Result<std::string> Submit(const ImageGenRequest& request) override;
    Core::Result<void> Cancel(const std::string& jobId) override;
    Core::Result<GenProgress> Progress(const std::string& jobId) const override;
    Core::Result<ImageGenResult> TakeResult(const std::string& jobId) override;

private:
    struct Job {
        ImageGenRequest request;
        GenProgress progress;
        ImageGenResult result;
        bool fail = false;
        Core::Error failError;
    };

    std::unordered_map<std::string, Job> m_jobs;
    std::uint32_t m_nextId = 0;
    bool m_failNext = false;
    Core::Error m_nextError;
};

class MockVideoGenService final : public IVideoGenService {
public:
    void FailNext(Core::Error error);
    void Pump();

    Core::Result<std::string> Submit(const VideoGenRequest& request) override;
    Core::Result<void> Cancel(const std::string& jobId) override;
    Core::Result<GenProgress> Progress(const std::string& jobId) const override;
    Core::Result<VideoGenResult> TakeResult(const std::string& jobId) override;

private:
    struct Job {
        VideoGenRequest request;
        GenProgress progress;
        VideoGenResult result;
        bool fail = false;
        Core::Error failError;
    };

    std::unordered_map<std::string, Job> m_jobs;
    std::uint32_t m_nextId = 0;
    bool m_failNext = false;
    Core::Error m_nextError;
};

} // namespace DirectorDesk::AI
