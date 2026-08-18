#include "DirectorDesk/Platform/Window.h"

#include "DirectorDesk/Core/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace DirectorDesk::Platform {
namespace {

int g_glfwUsers = 0;

void GlfwErrorCallback(int code, const char* description) {
    DD_LOG_ERROR("GLFW error {}: {}", code, description != nullptr ? description : "");
}

Core::Result<void> RetainGlfw() {
    if (g_glfwUsers == 0) {
        glfwSetErrorCallback(GlfwErrorCallback);
        if (glfwInit() == GLFW_FALSE) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::Internal,
                "glfwInit failed",
                "无法初始化窗口系统"));
        }
    }
    ++g_glfwUsers;
    return Core::Result<void>::Ok();
}

void ReleaseGlfw() {
    if (g_glfwUsers == 0) {
        return;
    }
    --g_glfwUsers;
    if (g_glfwUsers == 0) {
        glfwTerminate();
    }
}

} // namespace

Window::Window() = default;

Window::~Window() {
    Destroy();
}

Core::Result<void> Window::Create(const WindowDesc& desc) {
    if (m_handle != nullptr) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::AlreadyInitialized,
            "Window::Create called twice",
            "窗口已经创建"));
    }

    auto glfw = RetainGlfw();
    if (!glfw.IsOk()) {
        return glfw;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        static_cast<int>(desc.width),
        static_cast<int>(desc.height),
        desc.title.c_str(),
        nullptr,
        nullptr);
    if (window == nullptr) {
        ReleaseGlfw();
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal,
            "glfwCreateWindow returned null",
            "无法创建窗口"));
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(window);
        ReleaseGlfw();
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal,
            "gladLoadGLLoader failed",
            "无法初始化图形接口"));
    }

    m_handle = window;
    DD_LOG_INFO("Window created {}x{}", desc.width, desc.height);
    return Core::Result<void>::Ok();
}

void Window::Destroy() {
    if (m_handle == nullptr) {
        return;
    }
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_handle));
    m_handle = nullptr;
    ReleaseGlfw();
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_handle != nullptr) {
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_handle));
    }
}

bool Window::ShouldClose() const {
    if (m_handle == nullptr) {
        return true;
    }
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_handle)) == GLFW_TRUE;
}

void Window::RequestClose() {
    if (m_handle != nullptr) {
        glfwSetWindowShouldClose(static_cast<GLFWwindow*>(m_handle), GLFW_TRUE);
    }
}

FramebufferSize Window::GetFramebufferSize() const {
    FramebufferSize size;
    if (m_handle == nullptr) {
        return size;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_handle), &width, &height);
    size.width = width < 0 ? 0 : static_cast<std::uint32_t>(width);
    size.height = height < 0 ? 0 : static_cast<std::uint32_t>(height);
    return size;
}

void* Window::NativeHandle() const {
    return m_handle;
}

} // namespace DirectorDesk::Platform
