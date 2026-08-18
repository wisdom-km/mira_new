#include "ImGuiGlfwBackend.h"

#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace DirectorDesk::Backends {

ImGuiGlfwBackend::~ImGuiGlfwBackend() {
    Shutdown();
}

Core::Result<void> ImGuiGlfwBackend::Init(Platform::Window& window) {
    if (m_initialized) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::AlreadyInitialized,
            "ImGuiGlfwBackend::Init called twice",
            "界面系统已经初始化"));
    }

    auto* glfwWindow = static_cast<GLFWwindow*>(window.NativeHandle());
    if (glfwWindow == nullptr) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::NotInitialized,
            "ImGui init requires a created window",
            "窗口尚未创建"));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true)) {
        ImGui::DestroyContext();
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal,
            "ImGui_ImplGlfw_InitForOpenGL failed",
            "无法初始化界面窗口后端"));
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal,
            "ImGui_ImplOpenGL3_Init failed",
            "无法初始化界面渲染后端"));
    }

    m_initialized = true;
    DD_LOG_INFO("ImGui docking backend initialized");
    return Core::Result<void>::Ok();
}

void ImGuiGlfwBackend::Shutdown() {
    if (!m_initialized) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiGlfwBackend::BeginFrame(unsigned framebufferWidth, unsigned framebufferHeight) {
    if (framebufferWidth > 0 && framebufferHeight > 0) {
        glViewport(
            0,
            0,
            static_cast<int>(framebufferWidth),
            static_cast<int>(framebufferHeight));
    }
    glClearColor(0.09f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiGlfwBackend::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace DirectorDesk::Backends
