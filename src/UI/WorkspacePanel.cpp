#include "DirectorDesk/UI/WorkspacePanel.h"

#include "DirectorDesk/Core/Command.h"

#include <imgui.h>

namespace DirectorDesk::UI {

void WorkspacePanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("DirectorDeskDockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                commands.Push(Core::QuitCommand{});
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("DirectorDeskMainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();

    ImGui::Begin("Workspace");
    ImGui::TextUnformatted(state.appName);
    ImGui::Text("Window: %u x %u", state.windowWidth, state.windowHeight);
    ImGui::Separator();
    ImGui::TextUnformatted("Phase 0 skeleton. Dock this panel or open additional views later.");
    ImGui::End();
}

} // namespace DirectorDesk::UI
