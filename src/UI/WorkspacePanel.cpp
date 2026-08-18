#include "DirectorDesk/UI/WorkspacePanel.h"

#include "DirectorDesk/Core/Command.h"

#include <cstdint>
#include <imgui.h>
#include <imgui_internal.h>

namespace DirectorDesk::UI {
namespace {

void ApplyDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size) {
    if (ImGui::DockBuilderGetNode(dockspaceId) != nullptr) {
        return;
    }

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, size);

    ImGuiID dockMain = dockspaceId;
    const ImGuiID dockLeft =
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.28f, nullptr, &dockMain);
    ImGui::DockBuilderDockWindow("Workspace", dockLeft);
    ImGui::DockBuilderDockWindow("Viewport", dockMain);
    ImGui::DockBuilderFinish(dockspaceId);
}

} // namespace

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
            if (ImGui::MenuItem("Import Model...")) {
                commands.Push(Core::ImportModelCommand{});
            }
            if (ImGui::MenuItem("Export Test PNG")) {
                commands.Push(Core::ExportTestPngCommand{});
            }
            if (ImGui::MenuItem("Exit")) {
                commands.Push(Core::QuitCommand{});
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("DirectorDeskMainDockSpace");
    ApplyDefaultDockLayout(dockspaceId, viewport->WorkSize);
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::Begin("Viewport");
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const std::uint32_t width = available.x > 1.0f ? static_cast<std::uint32_t>(available.x) : 1;
    const std::uint32_t height = available.y > 1.0f ? static_cast<std::uint32_t>(available.y) : 1;
    commands.Push(Core::ViewportResizeCommand{width, height});
    if (state.viewportTextureIndex != 0xFFFFu) {
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)), available);
        if (ImGui::IsItemHovered()) {
            const ImGuiIO& io = ImGui::GetIO();
            Core::OrbitDeltaCommand orbit;
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
                orbit.rotateYaw = io.MouseDelta.x * 0.4f;
                orbit.rotatePitch = io.MouseDelta.y * 0.4f;
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
                orbit.panX = io.MouseDelta.x;
                orbit.panY = io.MouseDelta.y;
            }
            orbit.zoom = io.MouseWheel;
            if (orbit.rotateYaw != 0.0f || orbit.rotatePitch != 0.0f || orbit.panX != 0.0f ||
                orbit.panY != 0.0f || orbit.zoom != 0.0f) {
                commands.Push(orbit);
            }
        }
    } else {
        ImGui::TextUnformatted("Viewport texture is not ready.");
    }
    ImGui::End();

    ImGui::Begin("Workspace");
    ImGui::TextUnformatted(state.appName);
    ImGui::Text("Window: %u x %u", state.windowWidth, state.windowHeight);
    ImGui::Text("Viewport: %u x %u", state.viewportTextureWidth, state.viewportTextureHeight);
    ImGui::Separator();
    ImGui::TextUnformatted("Left drag: orbit  Right drag: pan  Wheel: zoom");
    if (ImGui::Button("Import Model...")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Test PNG")) {
        commands.Push(Core::ExportTestPngCommand{});
    }
    if (state.exampleObjPath != nullptr && state.exampleObjPath[0] != '\0') {
        if (ImGui::Button("Import Example OBJ")) {
            commands.Push(Core::ImportModelFromPathCommand{state.exampleObjPath});
        }
    }
    if (state.exampleGlbPath != nullptr && state.exampleGlbPath[0] != '\0') {
        if (ImGui::Button("Import Example GLB")) {
            commands.Push(Core::ImportModelFromPathCommand{state.exampleGlbPath});
        }
    }
    if (state.importInProgress) {
        ImGui::TextUnformatted("Loading model...");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Scene");
    if (state.nodes != nullptr) {
        for (const NodeView& node : *state.nodes) {
            if (ImGui::Selectable(node.name.c_str(), node.selected)) {
                commands.Push(Core::SelectNodeCommand{node.id});
            }
            if (node.selected) {
                float position[3] = {node.position[0], node.position[1], node.position[2]};
                float euler[3] = {node.eulerDegrees[0], node.eulerDegrees[1], node.eulerDegrees[2]};
                float scale[3] = {node.scale[0], node.scale[1], node.scale[2]};
                bool changed = false;
                changed = ImGui::DragFloat3("Position", position, 0.01f) || changed;
                changed = ImGui::DragFloat3("Rotation", euler, 0.5f) || changed;
                changed = ImGui::DragFloat3("Scale", scale, 0.01f) || changed;
                if (changed) {
                    Core::SetNodeTransformCommand transform;
                    transform.nodeId = node.id;
                    transform.position[0] = position[0];
                    transform.position[1] = position[1];
                    transform.position[2] = position[2];
                    transform.eulerDegrees[0] = euler[0];
                    transform.eulerDegrees[1] = euler[1];
                    transform.eulerDegrees[2] = euler[2];
                    transform.scale[0] = scale[0];
                    transform.scale[1] = scale[1];
                    transform.scale[2] = scale[2];
                    commands.Push(transform);
                }
            }
        }
    }

    if (state.statusText != nullptr && state.statusText[0] != '\0') {
        ImGui::Separator();
        ImGui::TextUnformatted(state.statusText);
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
