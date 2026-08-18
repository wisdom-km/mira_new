// Window: Public or internal interface for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

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
    void CancelClose();
    [[nodiscard]] FramebufferSize GetFramebufferSize() const;

    // GLFW window pointer. Phase 1 bgfx uses NativeOsHandle() instead.
    [[nodiscard]] void* NativeHandle() const;
    [[nodiscard]] void* NativeOsHandle() const;

private:
    void* m_handle = nullptr;
};

} // namespace DirectorDesk::Platform
