// WorkspacePanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

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
    ImGuiID dockBottom = 0;
    const ImGuiID dockCenter =
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.36f, &dockBottom, &dockMain);
    (void)dockCenter;
    ImGui::DockBuilderDockWindow("Workspace", dockLeftTop);
    ImGui::DockBuilderDockWindow("Library", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Viewport", dockMain);
    ImGui::DockBuilderDockWindow("Storyboard", dockBottom);
    ImGui::DockBuilderDockWindow("Script", dockRight);
    ImGui::DockBuilderFinish(dockspaceId);
}

} // namespace

void WorkspacePanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    // Panels are deliberately write-only toward the application: they render the immutable
    // snapshot and enqueue commands; state changes are applied on the next app tick.
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
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("新建工程", "Ctrl+N")) {
                commands.Push(Core::NewProjectCommand{});
            }
            if (ImGui::MenuItem("打开工程...", "Ctrl+O")) {
                commands.Push(Core::OpenProjectCommand{});
            }
            if (ImGui::MenuItem("保存工程", "Ctrl+S")) {
                commands.Push(Core::SaveProjectCommand{});
            }
            if (ImGui::MenuItem("工程另存为...", "Ctrl+Shift+S")) {
                commands.Push(Core::SaveProjectAsCommand{});
            }
            ImGui::Separator();
            if (ImGui::MenuItem("打开剧本...")) {
                commands.Push(Core::LoadScriptCommand{});
            }
            if (ImGui::MenuItem("保存剧本")) {
                commands.Push(Core::SaveScriptCommand{});
            }
            if (ImGui::MenuItem("导入模型...", "Ctrl+I")) {
                commands.Push(Core::ImportModelCommand{});
            }
            ImGui::Separator();
            if (ImGui::MenuItem("导出当前镜头 1080p", "Ctrl+E")) {
                commands.Push(Core::ExportCurrentShotCommand{"1080p"});
            }
            if (ImGui::MenuItem("导出当前镜头 2K")) {
                commands.Push(Core::ExportCurrentShotCommand{"2k"});
            }
            if (ImGui::MenuItem("导出分镜总览")) {
                commands.Push(Core::ExportStoryboardBoardCommand{});
            }
            ImGui::Separator();
            if (ImGui::MenuItem("退出")) {
                commands.Push(Core::QuitCommand{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("帮助")) {
            ImGui::TextUnformatted("Ctrl+N  新建工程");
            ImGui::TextUnformatted("Ctrl+O  打开工程");
            ImGui::TextUnformatted("Ctrl+S  保存工程");
            ImGui::TextUnformatted("Ctrl+Shift+S  工程另存为");
            ImGui::TextUnformatted("Ctrl+I  导入模型");
            ImGui::TextUnformatted("Ctrl+E  导出当前镜头 1080p");
            ImGui::Separator();
            ImGui::TextUnformatted("视口：左键旋转  右键平移  滚轮缩放");
            ImGui::TextUnformatted("分镜：右键平移  滚轮缩放");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (!ImGui::GetIO().WantTextInput) {
        const ImGuiIO& io = ImGui::GetIO();
        const bool ctrl = io.KeyCtrl && !io.KeyAlt;
        if (ctrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            commands.Push(Core::SaveProjectAsCommand{});
        } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            commands.Push(Core::SaveProjectCommand{});
        } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
            commands.Push(Core::NewProjectCommand{});
        } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            commands.Push(Core::OpenProjectCommand{});
        } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
            commands.Push(Core::ImportModelCommand{});
        } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            commands.Push(Core::ExportCurrentShotCommand{"1080p"});
        }
    }

    const ImGuiID dockspaceId = ImGui::GetID("DirectorDeskMainDockSpace");
    ApplyDefaultDockLayout(dockspaceId, viewport->WorkSize);
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::Begin("视口###Viewport", nullptr,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGuiWindow* window = ImGui::GetCurrentWindow()) {
        window->Scroll = ImVec2(0.0f, 0.0f);
    }
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const std::uint32_t width = available.x > 1.0f ? static_cast<std::uint32_t>(available.x) : 1;
    const std::uint32_t height = available.y > 1.0f ? static_cast<std::uint32_t>(available.y) : 1;
    const int dw = static_cast<int>(width) - static_cast<int>(m_lastViewportW);
    const int dh = static_cast<int>(height) - static_cast<int>(m_lastViewportH);
    if (m_lastViewportW == 0 || m_lastViewportH == 0 || dw * dw + dh * dh >= 4) {
        m_lastViewportW = width;
        m_lastViewportH = height;
    }
    commands.Push(Core::ViewportResizeCommand{m_lastViewportW, m_lastViewportH});
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    // Keep input separate from the renderer texture. This prevents ImGui scrolling from
    // changing content size and accidentally forcing a render-target rebuild.
    ImGui::InvisibleButton("viewport_input", available);
    if (state.viewportTextureIndex != 0xFFFFu) {
        ImGui::GetWindowDrawList()->AddImage(
            ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)), cursor,
            ImVec2(cursor.x + available.x, cursor.y + available.y));
    } else {
        ImGui::GetWindowDrawList()->AddText(cursor, IM_COL32(200, 200, 200, 255), "视口尚未就绪。");
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DD_ASSET_ID")) {
            const char* assetId = static_cast<const char*>(payload->Data);
            if (assetId != nullptr) {
                commands.Push(Core::AddLibraryAssetToSceneCommand{assetId});
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::IsItemHovered() || ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        Core::OrbitDeltaCommand orbit;
        if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
                orbit.rotateYaw = io.MouseDelta.x * 0.4f;
                orbit.rotatePitch = io.MouseDelta.y * 0.4f;
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
                orbit.panX = io.MouseDelta.x;
                orbit.panY = io.MouseDelta.y;
            }
        }
        orbit.zoom = io.MouseWheel;
        io.MouseWheel = 0.0f;
        io.MouseWheelH = 0.0f;
        if (orbit.rotateYaw != 0.0f || orbit.rotatePitch != 0.0f || orbit.panX != 0.0f ||
            orbit.panY != 0.0f || orbit.zoom != 0.0f) {
            commands.Push(orbit);
        }
    }
    ImGui::End();

    if (state.exportOverwritePrompt) {
        ImGui::OpenPopup("覆盖导出文件");
    }
    if (ImGui::BeginPopupModal("覆盖导出文件", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("目标 PNG 已存在，是否覆盖？");
        if (ImGui::Button("覆盖")) {
            commands.Push(Core::ConfirmExportOverwriteCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            commands.Push(Core::CancelExportOverwriteCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.exportStalePrompt) {
        ImGui::OpenPopup("缩略图未就绪");
    }
    if (ImGui::BeginPopupModal("缩略图未就绪", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("有 %d 个镜头缩略图缺失、过期或失败。", state.exportStaleCount);
        ImGui::TextUnformatted("继续导出将使用占位状态，是否继续？");
        if (ImGui::Button("继续导出")) {
            commands.Push(Core::ConfirmStoryboardStaleExportCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            commands.Push(Core::CancelStoryboardStaleExportCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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

    ImGui::Begin("导演台###Workspace");
    ImGui::TextUnformatted(state.appName);
    if (state.projectName != nullptr && state.projectName[0] != '\0') {
        ImGui::SameLine();
        ImGui::TextUnformatted(state.projectName);
        if (state.projectDirty) {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
        }
    }
    ImGui::TextDisabled("窗口 %u x %u  视口 %u x %u", state.windowWidth, state.windowHeight,
                        state.viewportTextureWidth, state.viewportTextureHeight);
    ImGui::Separator();
    ImGui::TextUnformatted("视口：左键旋转  右键平移  滚轮缩放");
    if (ImGui::Button("导入模型...")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::SameLine();
    bool exportTransparent = state.exportTransparent;
    if (ImGui::Checkbox("透明导出", &exportTransparent)) {
        commands.Push(Core::SetExportTransparentCommand{exportTransparent});
    }
    if (state.exampleProjectPath != nullptr && state.exampleProjectPath[0] != '\0') {
        if (ImGui::Button("打开示例工程")) {
            commands.Push(Core::OpenProjectFromPathCommand{state.exampleProjectPath});
        }
    }
    if (state.exampleObjPath != nullptr && state.exampleObjPath[0] != '\0') {
        if (ImGui::Button("导入示例 OBJ")) {
            commands.Push(Core::ImportModelFromPathCommand{state.exampleObjPath});
        }
    }
    if (state.exampleGlbPath != nullptr && state.exampleGlbPath[0] != '\0') {
        if (ImGui::Button("导入示例 GLB")) {
            commands.Push(Core::ImportModelFromPathCommand{state.exampleGlbPath});
        }
    }
    if (state.importInProgress) {
        ImGui::TextUnformatted("正在加载模型...");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("场景");
    if (state.nodes != nullptr && state.nodes->empty()) {
        ImGui::TextUnformatted("场景为空。从资源库拖入模型，或导入 OBJ/GLB。");
    }
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
                changed = ImGui::DragFloat3("位置", position, 0.01f) || changed;
                changed = ImGui::DragFloat3("旋转", euler, 0.5f) || changed;
                changed = ImGui::DragFloat3("缩放", scale, 0.01f) || changed;
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
    ImGui::TextUnformatted("相机");
    if (ImGui::Button("添加相机")) {
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
                if (ImGui::InputText("名称", nameBuffer, sizeof(nameBuffer))) {
                    m_cameraName = nameBuffer;
                    commands.Push(Core::RenameCameraCommand{camera.id, m_cameraName});
                }
                if (ImGui::Button("删除相机")) {
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

    ImGui::TextUnformatted("机位预设");
    if (ImGui::Button("正视")) {
        commands.Push(Core::ApplyCameraPresetCommand{"front"});
    }
    ImGui::SameLine();
    if (ImGui::Button("侧面")) {
        commands.Push(Core::ApplyCameraPresetCommand{"side"});
    }
    ImGui::SameLine();
    if (ImGui::Button("过肩")) {
        commands.Push(Core::ApplyCameraPresetCommand{"over-shoulder"});
    }
    if (ImGui::Button("俯视")) {
        commands.Push(Core::ApplyCameraPresetCommand{"top"});
    }
    ImGui::SameLine();
    if (ImGui::Button("特写")) {
        commands.Push(Core::ApplyCameraPresetCommand{"close-up"});
    }

    ImGui::TextUnformatted("灯光");
    if (ImGui::RadioButton("中性", state.lightPresetId != nullptr &&
                                       std::strcmp(state.lightPresetId, "neutral") == 0)) {
        commands.Push(Core::SetLightPresetCommand{"neutral"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("暖光", state.lightPresetId != nullptr &&
                                       std::strcmp(state.lightPresetId, "warm") == 0)) {
        commands.Push(Core::SetLightPresetCommand{"warm"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("冷光", state.lightPresetId != nullptr &&
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
