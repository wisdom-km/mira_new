#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

namespace DirectorDesk::Renderer {

struct RendererInitDesc {
    void* nativeWindowHandle = nullptr;
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    std::string shaderDirectory;
};

struct CameraView {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
};

struct RenderSceneView {
    bool showTestMesh = true;
};

enum class RenderTargetKind {
    Viewport,
    Offscreen,
};

struct RenderTargetDesc {
    RenderTargetKind kind = RenderTargetKind::Viewport;
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool transparentBackground = false;
};

struct PixelBuffer {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

} // namespace DirectorDesk::Renderer
