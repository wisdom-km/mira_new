#pragma once

#include <cstdint>
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

using Command =
    std::variant<QuitCommand, ViewportResizeCommand, OrbitDeltaCommand, ExportTestPngCommand>;

} // namespace DirectorDesk::Core
