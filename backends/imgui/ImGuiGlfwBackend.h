#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::Platform {
class Window;
}

namespace DirectorDesk::Backends {

class ImGuiGlfwBackend {
public:
    ImGuiGlfwBackend() = default;
    ImGuiGlfwBackend(const ImGuiGlfwBackend&) = delete;
    ImGuiGlfwBackend& operator=(const ImGuiGlfwBackend&) = delete;
    ~ImGuiGlfwBackend();

    Core::Result<void> Init(Platform::Window& window, const std::string& shaderDirectory,
                            std::uint8_t viewId);
    void Shutdown();
    void BeginFrame();
    void Submit(std::uint32_t framebufferWidth, std::uint32_t framebufferHeight);

private:
    Core::Result<void> CreateResources();
    void DestroyResources();

    bool m_initialized = false;
    std::string m_shaderDirectory;
    std::uint8_t m_viewId = 255;
    std::uint16_t m_program = 0xFFFFu;
    std::uint16_t m_textureSampler = 0xFFFFu;
};

} // namespace DirectorDesk::Backends
