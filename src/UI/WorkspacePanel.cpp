#include "DirectorDesk/UI/WorkspacePanel.h"

#include "DirectorDesk/Core/Command.h"

#include <cstdint>
#include <cstring>
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
    ImGuiID dockLeft =
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.24f, nullptr, &dockMain);
    const ImGuiID dockRight =
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.32f, nullptr, &dockMain);
    ImGuiID dockLeftBottom = 0;
    const ImGuiID dockLeftTop =
        ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.46f, &dockLeftBottom, &dockLeft);
    ImGui::DockBuilderDockWindow("Workspace", dockLeftTop);
    ImGui::DockBuilderDockWindow("Library", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", dockMain);
    ImGui::DockBuilderDockWindow("Script", dockRight);
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
            if (ImGui::MenuItem("New Project")) {
                commands.Push(Core::NewProjectCommand{});
            }
            if (ImGui::MenuItem("Open Project...")) {
                commands.Push(Core::OpenProjectCommand{});
            }
            if (ImGui::MenuItem("Save Project", nullptr, false, true)) {
                commands.Push(Core::SaveProjectCommand{});
            }
            if (ImGui::MenuItem("Save Project As...")) {
                commands.Push(Core::SaveProjectAsCommand{});
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open Script...")) {
                commands.Push(Core::LoadScriptCommand{});
            }
            if (ImGui::MenuItem("Save Script")) {
                commands.Push(Core::SaveScriptCommand{});
            }
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
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DD_ASSET_ID")) {
                const char* assetId = static_cast<const char*>(payload->Data);
                if (assetId != nullptr) {
                    commands.Push(Core::AddLibraryAssetToSceneCommand{assetId});
                }
            }
            ImGui::EndDragDropTarget();
        }
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

    if (state.projectPromptVisible) {
        ImGui::OpenPopup("未保存的工程");
    }
    if (ImGui::BeginPopupModal("未保存的工程", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("当前工程有未保存的更改。");
        if (ImGui::Button("保存")) {
            commands.Push(Core::ConfirmSaveProjectCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("放弃")) {
            commands.Push(Core::DiscardProjectCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            commands.Push(Core::CancelProjectPromptCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Begin("Workspace");
    ImGui::TextUnformatted(state.appName);
    if (state.projectName != nullptr && state.projectName[0] != '\0') {
        ImGui::SameLine();
        ImGui::TextUnformatted(state.projectName);
        if (state.projectDirty) {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
        }
    }
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

    ImGui::Separator();
    ImGui::TextUnformatted("Cameras");
    if (ImGui::Button("Add Camera")) {
        commands.Push(Core::AddCameraCommand{});
    }
    if (state.cameras != nullptr) {
        for (const CameraItemView& camera : *state.cameras) {
            const std::string label = camera.name + "##" + camera.id;
            if (ImGui::Selectable(label.c_str(), camera.selected)) {
                commands.Push(Core::SelectCameraCommand{camera.id});
            }
            if (camera.selected) {
                if (m_cameraNameId != camera.id) {
                    m_cameraName = camera.name;
                    m_cameraNameId = camera.id;
                }
                char nameBuffer[128];
                const std::size_t copy = m_cameraName.size() < sizeof(nameBuffer) - 1
                                             ? m_cameraName.size()
                                             : sizeof(nameBuffer) - 1;
                std::memcpy(nameBuffer, m_cameraName.data(), copy);
                nameBuffer[copy] = '\0';
                if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                    m_cameraName = nameBuffer;
                    commands.Push(Core::RenameCameraCommand{camera.id, m_cameraName});
                }
                if (ImGui::Button("Remove Camera")) {
                    commands.Push(Core::RemoveCameraCommand{camera.id});
                }
            }
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("镜头关联");
    if (state.selectedShotLinkedCamera != nullptr && state.selectedShotLinkedCamera[0] != '\0') {
        ImGui::Text("当前镜头相机: %s", state.selectedShotLinkedCamera);
    } else {
        ImGui::TextUnformatted("当前镜头未关联相机");
    }
    if (ImGui::Button("关联到当前相机")) {
        commands.Push(Core::LinkShotToCameraCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("取消关联")) {
        commands.Push(Core::UnlinkShotCommand{});
    }

    ImGui::TextUnformatted("Presets");
    if (ImGui::Button("Front")) {
        commands.Push(Core::ApplyCameraPresetCommand{"front"});
    }
    ImGui::SameLine();
    if (ImGui::Button("Side")) {
        commands.Push(Core::ApplyCameraPresetCommand{"side"});
    }
    ImGui::SameLine();
    if (ImGui::Button("Over Shoulder")) {
        commands.Push(Core::ApplyCameraPresetCommand{"over-shoulder"});
    }
    if (ImGui::Button("Top")) {
        commands.Push(Core::ApplyCameraPresetCommand{"top"});
    }
    ImGui::SameLine();
    if (ImGui::Button("Close-up")) {
        commands.Push(Core::ApplyCameraPresetCommand{"close-up"});
    }

    ImGui::TextUnformatted("Light");
    if (ImGui::RadioButton("Neutral", state.lightPresetId != nullptr &&
                                          std::strcmp(state.lightPresetId, "neutral") == 0)) {
        commands.Push(Core::SetLightPresetCommand{"neutral"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Warm", state.lightPresetId != nullptr &&
                                       std::strcmp(state.lightPresetId, "warm") == 0)) {
        commands.Push(Core::SetLightPresetCommand{"warm"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Cool", state.lightPresetId != nullptr &&
                                       std::strcmp(state.lightPresetId, "cool") == 0)) {
        commands.Push(Core::SetLightPresetCommand{"cool"});
    }

    if (state.statusText != nullptr && state.statusText[0] != '\0') {
        ImGui::Separator();
        ImGui::TextUnformatted(state.statusText);
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
