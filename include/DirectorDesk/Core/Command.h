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

using Command = std::variant<QuitCommand, ViewportResizeCommand, OrbitDeltaCommand,
                             ExportTestPngCommand, ImportModelCommand, ImportModelFromPathCommand,
                             SelectNodeCommand, SetNodeTransformCommand, LoadScriptCommand,
                             LoadScriptFromPathCommand, SaveScriptCommand, SetScriptTextCommand,
                             InsertSceneCommand, InsertShotCommand, SelectShotCommand>;

} // namespace DirectorDesk::Core
