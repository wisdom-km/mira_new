#pragma once

#include "DirectorDesk/Core/Result.h"

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

    Core::Result<void> Init(Platform::Window& window);
    void Shutdown();
    void BeginFrame(unsigned framebufferWidth, unsigned framebufferHeight);
    void EndFrame();

private:
    bool m_initialized = false;
};

} // namespace DirectorDesk::Backends
