#include "DirectorDesk/App/Application.h"

#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/CommandQueue.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Startup.h"
#include "DirectorDesk/Platform/Window.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include "ImGuiGlfwBackend.h"

#include <type_traits>
#include <variant>

namespace DirectorDesk::App {

int Application::Run() {
    Platform::InitializeProcess();

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

    DD_LOG_INFO("DirectorDesk starting (Phase 0 skeleton)");

    Platform::Window window;
    auto windowResult = window.Create(Platform::WindowDesc{});
    if (!windowResult.IsOk()) {
        DD_LOG_ERROR("{}", windowResult.GetError().technicalMessage);
        Core::Log::Shutdown();
        return 1;
    }

    Backends::ImGuiGlfwBackend imgui;
    auto imguiResult = imgui.Init(window);
    if (!imguiResult.IsOk()) {
        DD_LOG_ERROR("{}", imguiResult.GetError().technicalMessage);
        window.Destroy();
        Core::Log::Shutdown();
        return 1;
    }

    UI::WorkspacePanel workspace;
    Core::CommandQueue commands;
    UI::AppViewState viewState;

    while (!window.ShouldClose()) {
        window.PollEvents();

        Core::Command command;
        while (commands.TryPop(command)) {
            std::visit(
                [&](const auto& typed) {
                    using T = std::decay_t<decltype(typed)>;
                    if constexpr (std::is_same_v<T, Core::QuitCommand>) {
                        window.RequestClose();
                    }
                },
                command);
        }

        const auto size = window.GetFramebufferSize();
        viewState.windowWidth = size.width;
        viewState.windowHeight = size.height;

        imgui.BeginFrame(size.width, size.height);
        workspace.Draw(viewState, commands);
        imgui.EndFrame();
        window.SwapBuffers();
    }

    imgui.Shutdown();
    window.Destroy();
    DD_LOG_INFO("DirectorDesk exiting");
    Core::Log::Shutdown();
    return 0;
}

} // namespace DirectorDesk::App
