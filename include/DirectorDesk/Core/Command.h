#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace DirectorDesk::Core {

struct QuitCommand {};

struct ViewportResizeCommand {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct OrbitDeltaCommand {
    float rotateYaw = 0.0f;
    float rotatePitch = 0.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 0.0f;
};

struct ExportTestPngCommand {};

struct ImportModelCommand {};

struct ImportModelFromPathCommand {
    std::string utf8Path;
};

struct SelectNodeCommand {
    std::string nodeId;
};

struct SetNodeTransformCommand {
    std::string nodeId;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float eulerDegrees[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
};

struct LoadScriptCommand {};

struct LoadScriptFromPathCommand {
    std::string utf8Path;
};

struct SaveScriptCommand {};

struct SetScriptTextCommand {
    std::string text;
};

struct InsertSceneCommand {};

struct InsertShotCommand {};

struct SelectShotCommand {
    std::string shotId;
};

struct ApplyCameraPresetCommand {
    std::string presetId;
};

struct AddCameraCommand {};

struct RemoveCameraCommand {
    std::string cameraId;
};

struct RenameCameraCommand {
    std::string cameraId;
    std::string name;
};

struct SelectCameraCommand {
    std::string cameraId;
};

struct SetLightPresetCommand {
    std::string presetId;
};

using Command = std::variant<
    QuitCommand, ViewportResizeCommand, OrbitDeltaCommand, ExportTestPngCommand, ImportModelCommand,
    ImportModelFromPathCommand, SelectNodeCommand, SetNodeTransformCommand, LoadScriptCommand,
    LoadScriptFromPathCommand, SaveScriptCommand, SetScriptTextCommand, InsertSceneCommand,
    InsertShotCommand, SelectShotCommand, ApplyCameraPresetCommand, AddCameraCommand,
    RemoveCameraCommand, RenameCameraCommand, SelectCameraCommand, SetLightPresetCommand>;

} // namespace DirectorDesk::Core
