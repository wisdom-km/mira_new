#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

struct GpuVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float nx = 0.0f;
    float ny = 1.0f;
    float nz = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    std::uint32_t abgr = 0xffffffffu;
};

struct GpuPrimitive {
    std::vector<GpuVertex> vertices;
    std::vector<std::uint32_t> indices;
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    std::uint32_t textureWidth = 0;
    std::uint32_t textureHeight = 0;
    std::vector<std::uint8_t> rgba;
    glm::mat4 localTransform{1.0f};
};

struct GpuModelDesc {
    std::vector<GpuPrimitive> primitives;
};

struct RenderMeshInstance {
    std::uint32_t modelId = 0;
    glm::mat4 world{1.0f};
    bool visible = true;
};

struct RenderLight {
    glm::vec3 direction{0.35f, 0.80f, 0.45f};
    glm::vec3 color{1.0f, 0.98f, 0.94f};
};

struct RenderSceneView {
    bool showTestMesh = true;
    bool showGroundGrid = false;
    RenderLight light;
    std::vector<RenderMeshInstance> instances;
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
