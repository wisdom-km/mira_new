#pragma once

#include "DirectorDesk/Core/Error.h"

#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::AI {

enum class GenJobStatus {
    Queued,
    Running,
    Succeeded,
    Failed,
    Cancelled,
};

struct ShotGenContext {
    std::string shotId;
    std::string sceneId;
    std::string shotTitle;
    std::string shotText;
    std::string cameraId;
    std::string cameraPresetId;
    std::string projectName;
};

struct ReferenceImage {
    std::string path;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

struct GenProgress {
    std::string jobId;
    GenJobStatus status = GenJobStatus::Queued;
    float ratio = 0.0f;
    std::string message;
    Core::Error error;
};

struct ImageGenRequest {
    std::string prompt;
    ShotGenContext shot;
    std::vector<ReferenceImage> references;
    std::uint32_t width = 1920;
    std::uint32_t height = 1080;
};

struct VideoGenRequest {
    std::string prompt;
    ShotGenContext shot;
    std::vector<ReferenceImage> references;
    std::uint32_t width = 1920;
    std::uint32_t height = 1080;
    std::uint32_t durationMs = 4000;
    std::uint32_t frameRate = 24;
};

struct ImageGenResult {
    std::string jobId;
    std::string outputPath;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    ShotGenContext shot;
    std::vector<ReferenceImage> references;
};

struct VideoGenResult {
    std::string jobId;
    std::string outputPath;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t durationMs = 0;
    std::uint32_t frameRate = 0;
    ShotGenContext shot;
    std::vector<ReferenceImage> references;
};

[[nodiscard]] const char* JobStatusId(GenJobStatus status);
[[nodiscard]] bool IsTerminal(GenJobStatus status);
[[nodiscard]] bool IsSafeReferencePath(const std::string& path);

} // namespace DirectorDesk::AI
