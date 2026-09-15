// ShotExport: Implementation for the DirectorDesk Export module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Export/ShotExport.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Renderer/PngWriter.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <utility>

#ifndef DD_PROJECT_VERSION
#define DD_PROJECT_VERSION "0.1.3"
#endif

namespace DirectorDesk::Export {
namespace {

constexpr float kPi = 3.14159265358979323846f;

float DegreesToRadians(float degrees) {
    return degrees * (kPi / 180.0f);
}

float RadiansToDegrees(float radians) {
    return radians * (180.0f / kPi);
}

nlohmann::json Vec3Json(const std::array<float, 3>& value) {
    return nlohmann::json::array({value[0], value[1], value[2]});
}

nlohmann::json Vec4Json(const std::array<float, 4>& value) {
    return nlohmann::json::array({value[0], value[1], value[2], value[3]});
}

} // namespace

ShotSize SizeFor(ShotResolution resolution) {
    if (resolution == ShotResolution::Uhd2k) {
        return ShotSize{2560, 1440};
    }
    return ShotSize{1920, 1080};
}

const char* ResolutionId(ShotResolution resolution) {
    return resolution == ShotResolution::Uhd2k ? "2k" : "1080p";
}

bool TryParseResolution(const std::string& id, ShotResolution& out) {
    if (id == "2k" || id == "2560x1440") {
        out = ShotResolution::Uhd2k;
        return true;
    }
    if (id == "1080p" || id == "1920x1080") {
        out = ShotResolution::Hd1080;
        return true;
    }
    return false;
}

std::string DefaultShotFileName(const std::string& projectName, const std::string& shotId,
                                ShotResolution resolution, bool transparent) {
    const ShotSize size = SizeFor(resolution);
    std::string stem = projectName.empty() ? "DirectorDesk" : projectName;
    for (char& ch : stem) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' ||
            ch == '<' || ch == '>' || ch == '|') {
            ch = '_';
        }
    }
    return stem + "-" + (shotId.empty() ? "shot" : shotId) + "-" +
           std::to_string(size.width) + "x" + std::to_string(size.height) +
           (transparent ? "-alpha.png" : ".png");
}

Renderer::RenderTargetDesc MakeOffscreenTarget(ShotResolution resolution, bool transparent) {
    const ShotSize size = SizeFor(resolution);
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = size.width;
    target.height = size.height;
    target.transparentBackground = transparent;
    return target;
}

float VerticalFovToFocalLength35mm(float verticalFovDegrees) {
    const float half = DegreesToRadians(verticalFovDegrees) * 0.5f;
    const float tangent = std::tan(half);
    if (tangent <= 1.0e-6f) {
        return 0.0f;
    }
    return 12.0f / tangent;
}

float HorizontalFovDegrees(float verticalFovDegrees, float aspect) {
    const float half = DegreesToRadians(verticalFovDegrees) * 0.5f;
    return RadiansToDegrees(2.0f * std::atan(aspect * std::tan(half)));
}

Core::Result<ShotPackagePaths> WriteShotPackage(const std::string& directoryUtf8,
                                                const ShotPackageInput& input,
                                                const Renderer::PixelBuffer& pixels) {
    if (directoryUtf8.empty() || input.shotId.empty()) {
        return Core::Result<ShotPackagePaths>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Shot package needs directory and shot id",
            "无法导出镜头包"));
    }
    auto created = Platform::Paths::CreateDirectories(directoryUtf8);
    if (!created.IsOk()) {
        return Core::Result<ShotPackagePaths>::Fail(created.GetError());
    }
    ShotPackagePaths paths;
    paths.pngPath = Platform::Paths::Join(directoryUtf8, input.shotId + ".png");
    paths.jsonPath = Platform::Paths::Join(directoryUtf8, input.shotId + ".shot.json");
    auto writtenPng = Renderer::WritePng(pixels, paths.pngPath);
    if (!writtenPng.IsOk()) {
        return Core::Result<ShotPackagePaths>::Fail(writtenPng.GetError());
    }

    nlohmann::json meta = nlohmann::json::array();
    for (const ShotPackageMeta& item : input.meta) {
        meta.push_back({{"key", item.key}, {"value", item.value}});
    }
    nlohmann::json nodes = nlohmann::json::array();
    for (const ShotPackageNode& node : input.sceneNodes) {
        if (!node.visible) {
            continue;
        }
        nodes.push_back({{"id", node.id},
                         {"name", node.name},
                         {"assetId", node.assetId},
                         {"assetName", node.assetName},
                         {"position", Vec3Json(node.position)},
                         {"rotation", Vec4Json(node.rotation)},
                         {"scale", Vec3Json(node.scale)},
                         {"visible", true}});
    }
    const std::string generatedBy =
        input.generatedBy.empty() ? std::string("DirectorDesk ") + DD_PROJECT_VERSION
                                  : input.generatedBy;
    nlohmann::json root = {{"format", "DirectorDeskShotPackage"},
                           {"formatVersion", 1},
                           {"generatedBy", generatedBy},
                           {"generatedAt", input.generatedAt},
                           {"project", {{"name", input.projectName}, {"projectId", input.projectId}}},
                           {"scene",
                            {{"id", input.sceneId},
                             {"title", input.sceneTitle},
                             {"body", input.sceneBody}}},
                           {"shot",
                            {{"id", input.shotId},
                             {"title", input.shotTitle},
                             {"body", input.shotBody},
                             {"meta", std::move(meta)},
                             {"prompt", input.prompt},
                             {"negativePrompt", input.negativePrompt}}},
                           {"image",
                            {{"color", input.shotId + ".png"},
                             {"width", input.imageWidth},
                             {"height", input.imageHeight},
                             {"transparentBackground", input.transparentBackground}}},
                           {"camera",
                            {{"id", input.cameraId},
                             {"name", input.cameraName},
                             {"position", Vec3Json(input.cameraPosition)},
                             {"rotation", Vec4Json(input.cameraRotation)},
                             {"lookAt", Vec3Json(input.cameraLookAt)},
                             {"verticalFovDegrees", input.verticalFovDegrees},
                             {"horizontalFovDegrees",
                              HorizontalFovDegrees(input.verticalFovDegrees, input.aspect)},
                             {"focalLength35mmEquivalent",
                              VerticalFovToFocalLength35mm(input.verticalFovDegrees)},
                             {"aspect", input.aspect},
                             {"preset", input.cameraPreset}}},
                           {"sceneNodes", std::move(nodes)},
                           {"lighting", {{"preset", input.lightingPreset}}}};
    auto writtenJson = Platform::Paths::WriteTextFile(paths.jsonPath, root.dump(2));
    if (!writtenJson.IsOk()) {
        return Core::Result<ShotPackagePaths>::Fail(writtenJson.GetError());
    }
    return Core::Result<ShotPackagePaths>::Ok(std::move(paths));
}

Core::Result<void> WriteBoardIndex(const std::string& jsonPathUtf8, const std::string& imageFileName,
                                   const std::vector<BoardIndexShot>& shots) {
    nlohmann::json items = nlohmann::json::array();
    for (const BoardIndexShot& shot : shots) {
        items.push_back({{"id", shot.id},
                         {"title", shot.title},
                         {"image", shot.image.empty() ? imageFileName : shot.image},
                         {"metaLine", shot.metaLine}});
    }
    nlohmann::json root = {{"format", "DirectorDeskBoardIndex"},
                           {"formatVersion", 1},
                           {"image", imageFileName},
                           {"shots", std::move(items)}};
    return Platform::Paths::WriteTextFile(jsonPathUtf8, root.dump(2));
}

} // namespace DirectorDesk::Export
