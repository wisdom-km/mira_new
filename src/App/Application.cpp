#include "DirectorDesk/App/Application.h"

#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/CommandQueue.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Startup.h"
#include "DirectorDesk/Platform/Window.h"
#include "DirectorDesk/Renderer/IRenderer.h"
#include "DirectorDesk/Renderer/PngWriter.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include "CreateBgfxRenderer.h"
#include "ImGuiGlfwBackend.h"

#include <cstring>
#include <string>
#include <type_traits>
#include <variant>

namespace DirectorDesk::App {
namespace {

bool WantsExportAndQuit(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (argv != nullptr && argv[i] != nullptr &&
            std::strcmp(argv[i], "--export-test-png") == 0) {
            return true;
        }
    }
    return false;
}

bool HandleExportTestPng(Renderer::IRenderer& renderer, const Camera::OrbitCamera& camera,
                         std::string& status) {
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = 1280;
    target.height = 720;
    target.transparentBackground = true;

    const float aspect = static_cast<float>(target.width) / static_cast<float>(target.height);
    renderer.BeginFrame(target.width, target.height);
    renderer.RenderScene(Renderer::RenderSceneView{}, camera.BuildView(aspect), target);
    auto pixels = renderer.ReadbackTarget(target);
    if (!pixels.IsOk()) {
        status = pixels.GetError().userMessage;
        DD_LOG_ERROR("{}", pixels.GetError().technicalMessage);
        return false;
    }

    auto userData = Platform::Paths::UserDataDirectory();
    if (!userData.IsOk()) {
        status = userData.GetError().userMessage;
        return false;
    }
    const std::string path = Platform::Paths::Join(
        Platform::Paths::Join(userData.Value(), "exports"), "phase1-offscreen.png");
    auto written = Renderer::WritePng(pixels.Value(), path);
    if (!written.IsOk()) {
        status = written.GetError().userMessage;
        DD_LOG_ERROR("{}", written.GetError().technicalMessage);
        return false;
    }

    bool hasOpaque = false;
    bool hasTransparent = false;
    for (std::size_t i = 3; i < pixels.Value().rgba.size(); i += 4) {
        const std::uint8_t alpha = pixels.Value().rgba[i];
        hasOpaque = hasOpaque || alpha > 200;
        hasTransparent = hasTransparent || alpha < 20;
    }
    const bool alphaOk = hasOpaque && hasTransparent;
    status = alphaOk ? "Exported transparent PNG (alpha OK)"
                     : "Exported PNG, but alpha coverage looks unexpected";
    DD_LOG_INFO("Exported test PNG to {} opaque={} transparent={}", path, hasOpaque,
                hasTransparent);
    return alphaOk;
}

} // namespace

int Application::Run(int argc, char** argv) {
    Platform::InitializeProcess();
    const bool exportAndQuit = WantsExportAndQuit(argc, argv);

    auto logDir = Platform::Paths::LogDirectory();
    if (!logDir.IsOk()) {
        return 1;
    }
    const auto created = Platform::Paths::CreateDirectories(logDir.Value());
    if (!created.IsOk()) {
        return 1;
    }
    auto logInit = Core::Log::Init(logDir.Value());
    if (!logInit.IsOk()) {
        return 1;
    }

    DD_LOG_INFO("DirectorDesk starting (Phase 1 render/camera)");

    auto exeDir = Platform::Paths::ExecutableDirectory();
    if (!exeDir.IsOk()) {
        DD_LOG_ERROR("{}", exeDir.GetError().technicalMessage);
        Core::Log::Shutdown();
        return 1;
    }
    const std::string shaderDirectory = Platform::Paths::Join(exeDir.Value(), "shaders");

    Platform::Window window;
    auto windowResult = window.Create(Platform::WindowDesc{});
    if (!windowResult.IsOk()) {
        DD_LOG_ERROR("{}", windowResult.GetError().technicalMessage);
        Core::Log::Shutdown();
        return 1;
    }

    auto renderer = Backends::CreateBgfxRenderer();
    const auto framebuffer = window.GetFramebufferSize();
    Renderer::RendererInitDesc rendererDesc;
    rendererDesc.nativeWindowHandle = window.NativeOsHandle();
    rendererDesc.width = framebuffer.width;
    rendererDesc.height = framebuffer.height;
    rendererDesc.shaderDirectory = shaderDirectory;
    auto rendererInit = renderer->Init(rendererDesc);
    if (!rendererInit.IsOk()) {
        DD_LOG_ERROR("{}", rendererInit.GetError().technicalMessage);
        window.Destroy();
        Core::Log::Shutdown();
        return 1;
    }

    Camera::OrbitCamera camera;
    if (exportAndQuit) {
        std::string status;
        const bool ok = HandleExportTestPng(*renderer, camera, status);
        renderer->Shutdown();
        window.Destroy();
        DD_LOG_INFO("DirectorDesk exiting");
        Core::Log::Shutdown();
        return ok ? 0 : 1;
    }

    Backends::ImGuiGlfwBackend imgui;
    auto imguiResult = imgui.Init(window, shaderDirectory, 255);
    if (!imguiResult.IsOk()) {
        DD_LOG_ERROR("{}", imguiResult.GetError().technicalMessage);
        renderer->Shutdown();
        window.Destroy();
        Core::Log::Shutdown();
        return 1;
    }

    UI::WorkspacePanel workspace;
    Core::CommandQueue commands;
    UI::AppViewState viewState;
    std::string status;

    while (!window.ShouldClose()) {
        window.PollEvents();

        Core::Command command;
        while (commands.TryPop(command)) {
            std::visit(
                [&](const auto& typed) {
                    using T = std::decay_t<decltype(typed)>;
                    if constexpr (std::is_same_v<T, Core::QuitCommand>) {
                        window.RequestClose();
                    } else if constexpr (std::is_same_v<T, Core::ViewportResizeCommand>) {
                        renderer->SetViewportSize(typed.width, typed.height);
                    } else if constexpr (std::is_same_v<T, Core::OrbitDeltaCommand>) {
                        camera.Rotate(typed.rotateYaw, typed.rotatePitch);
                        camera.Pan(typed.panX, typed.panY);
                        camera.Zoom(typed.zoom);
                    } else if constexpr (std::is_same_v<T, Core::ExportTestPngCommand>) {
                        HandleExportTestPng(*renderer, camera, status);
                    }
                },
                command);
        }

        const auto size = window.GetFramebufferSize();
        viewState.windowWidth = size.width;
        viewState.windowHeight = size.height;
        viewState.viewportTextureIndex = renderer->ViewportTextureIndex();
        viewState.viewportTextureWidth = renderer->ViewportWidth();
        viewState.viewportTextureHeight = renderer->ViewportHeight();
        viewState.statusText = status.c_str();

        const float aspect = renderer->ViewportHeight() == 0
                                 ? 1.0f
                                 : static_cast<float>(renderer->ViewportWidth()) /
                                       static_cast<float>(renderer->ViewportHeight());
        renderer->BeginFrame(size.width, size.height);
        renderer->RenderScene(Renderer::RenderSceneView{}, camera.BuildView(aspect),
                              Renderer::RenderTargetDesc{});
        imgui.BeginFrame();
        workspace.Draw(viewState, commands);
        imgui.Submit(size.width, size.height);
        renderer->EndFrame();
    }

    imgui.Shutdown();
    renderer->Shutdown();
    window.Destroy();
    DD_LOG_INFO("DirectorDesk exiting");
    Core::Log::Shutdown();
    return 0;
}

} // namespace DirectorDesk::App
