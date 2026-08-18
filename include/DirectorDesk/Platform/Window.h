#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::Platform {

struct WindowDesc {
    std::string title = "DirectorDesk";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
};

struct FramebufferSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

class Window {
public:
    Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();

    Core::Result<void> Create(const WindowDesc& desc);
    void Destroy();
    void PollEvents();
    void SwapBuffers();
    [[nodiscard]] bool ShouldClose() const;
    void RequestClose();
    [[nodiscard]] FramebufferSize GetFramebufferSize() const;

    // Opaque native window pointer. Phase 0 is GLFWwindow*; Phase 1 passes it to bgfx.
    [[nodiscard]] void* NativeHandle() const;

private:
    void* m_handle = nullptr;
};

} // namespace DirectorDesk::Platform
