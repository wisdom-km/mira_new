// ShotExport: Public or internal interface for the DirectorDesk Export module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Renderer/Types.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Export {

enum class ShotResolution {
    Hd1080,
    Uhd2k,
};

struct ShotExportOptions {
    ShotResolution resolution = ShotResolution::Hd1080;
    bool transparentBackground = true;
    std::string outputPath;
};

struct ShotSize {
    std::uint32_t width = 1920;
    std::uint32_t height = 1080;
};

[[nodiscard]] ShotSize SizeFor(ShotResolution resolution);
[[nodiscard]] const char* ResolutionId(ShotResolution resolution);
[[nodiscard]] bool TryParseResolution(const std::string& id, ShotResolution& out);
[[nodiscard]] std::string DefaultShotFileName(const std::string& projectName,
                                              const std::string& shotId,
                                              ShotResolution resolution, bool transparent);
[[nodiscard]] Renderer::RenderTargetDesc MakeOffscreenTarget(ShotResolution resolution,
                                                             bool transparent);

struct ShotPackageMeta {
    std::string key;
    std::string value;
};

struct ShotPackageNode {
    std::string id;
    std::string name;
    std::string assetId;
    std::string assetName;
    std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> scale = {1.0f, 1.0f, 1.0f};
    bool visible = true;
};

struct ShotPackageInput {
    std::string generatedBy;
    std::string generatedAt;
    std::string projectName;
    std::string projectId;
    std::string sceneId;
    std::string sceneTitle;
    std::string sceneBody;
    std::string shotId;
    std::string shotTitle;
    std::string shotBody;
    std::vector<ShotPackageMeta> meta;
    std::string prompt;
    std::string negativePrompt;
    std::uint32_t imageWidth = 1920;
    std::uint32_t imageHeight = 1080;
    bool transparentBackground = false;
    std::string cameraId;
    std::string cameraName;
    std::array<float, 3> cameraPosition = {0.0f, 0.0f, 0.0f};
    std::array<float, 4> cameraRotation = {0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> cameraLookAt = {0.0f, 0.0f, 0.0f};
    float verticalFovDegrees = 45.0f;
    float aspect = 1.7778f;
    std::string cameraPreset;
    std::vector<ShotPackageNode> sceneNodes;
    std::string lightingPreset;
};

struct ShotPackagePaths {
    std::string pngPath;
    std::string jsonPath;
};

struct BoardIndexShot {
    std::string id;
    std::string title;
    std::string image;
    std::string metaLine;
};

[[nodiscard]] float VerticalFovToFocalLength35mm(float verticalFovDegrees);
[[nodiscard]] float HorizontalFovDegrees(float verticalFovDegrees, float aspect);
[[nodiscard]] Core::Result<ShotPackagePaths> WriteShotPackage(const std::string& directoryUtf8,
                                                              const ShotPackageInput& input,
                                                              const Renderer::PixelBuffer& pixels);
[[nodiscard]] Core::Result<void> WriteBoardIndex(const std::string& jsonPathUtf8,
                                                 const std::string& imageFileName,
                                                 const std::vector<BoardIndexShot>& shots);
[[nodiscard]] Core::Result<void> WriteBoardPdf(const std::string& utf8Path,
                                               const std::vector<Renderer::PixelBuffer>& pages);

} // namespace DirectorDesk::Export
