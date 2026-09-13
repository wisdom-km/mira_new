// WorkspacePanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/WorkspacePanel.h"

#include "UiChrome.h"
#include "UiFonts.h"

#include "DirectorDesk/Core/Command.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kAccent(0.847f, 0.604f, 0.290f, 1.0f);
constexpr ImVec4 kSuccess(0.310f, 0.749f, 0.498f, 1.0f);
constexpr ImVec4 kWarning(0.878f, 0.537f, 0.290f, 1.0f);
constexpr ImVec4 kMuted(0.604f, 0.604f, 0.635f, 1.0f);
constexpr ImVec4 kText(0.902f, 0.902f, 0.902f, 1.0f);
constexpr ImU32 kModalTextU32 = IM_COL32(0xE6, 0xE6, 0xE6, 0xFF);

const char* ModeId(const AppViewState& state) {
    return state.workspaceModeId != nullptr && state.workspaceModeId[0] != '\0'
               ? state.workspaceModeId
               : "shoot";
}

bool ModeIs(const AppViewState& state, const char* id) {
    return std::strcmp(ModeId(state), id) == 0;
}

bool ProjectIsEmpty(const AppViewState& state) {
    const bool noProject = state.projectPath == nullptr || state.projectPath[0] == '\0';
    const bool noNodes = state.nodes == nullptr || state.nodes->empty();
    return noProject && !state.scriptHasSnapshot && noNodes;
}

const char* KindId(const AppViewState& state) {
    return state.selectionKind != nullptr && state.selectionKind[0] != '\0' ? state.selectionKind
                                                                            : "none";
}

bool KindIs(const AppViewState& state, const char* id) {
    return std::strcmp(KindId(state), id) == 0;
}

const StoryboardCardView* FindShotCard(const AppViewState& state, const std::string& shotId) {
    if (state.storyboardCards == nullptr || shotId.empty()) {
        return nullptr;
    }
    for (const StoryboardCardView& card : *state.storyboardCards) {
        if (card.kind == "shot" && card.shotId == shotId) {
            return &card;
        }
    }
    return nullptr;
}

const ScriptShotView* FindSelectedShot(const AppViewState& state,
                                       const ScriptSceneView** sceneOut) {
    if (sceneOut != nullptr) {
        *sceneOut = nullptr;
    }
    if (state.scriptScenes == nullptr) {
        return nullptr;
    }
    for (const ScriptSceneView& scene : *state.scriptScenes) {
        for (const ScriptShotView& shot : scene.shots) {
            if (shot.selected) {
                if (sceneOut != nullptr) {
                    *sceneOut = &scene;
                }
                return &shot;
            }
        }
    }
    return nullptr;
}

const NodeView* FindSelectedNode(const AppViewState& state) {
    if (state.nodes == nullptr) {
        return nullptr;
    }
    for (const NodeView& node : *state.nodes) {
        if (node.selected) {
            return &node;
        }
    }
    return nullptr;
}

const CameraItemView* FindSelectedCamera(const AppViewState& state) {
    if (state.cameras == nullptr) {
        return nullptr;
    }
    for (const CameraItemView& camera : *state.cameras) {
        if (camera.selected) {
            return &camera;
        }
    }
    return nullptr;
}

const LibraryAssetView* FindSelectedAsset(const AppViewState& state) {
    if (state.libraryAssets == nullptr) {
        return nullptr;
    }
    for (const LibraryAssetView& asset : *state.libraryAssets) {
        if (asset.selected) {
            return &asset;
        }
    }
    return nullptr;
}

void CountShots(const AppViewState& state, int& shotCount, int& readyCount) {
    shotCount = 0;
    readyCount = 0;
    if (state.storyboardCards == nullptr) {
        return;
    }
    for (const StoryboardCardView& card : *state.storyboardCards) {
        if (card.kind != "shot") {
            continue;
        }
        ++shotCount;
        if (card.link != nullptr && std::strcmp(card.link, "已关联") == 0 &&
            card.preview != nullptr && std::strcmp(card.preview, "就绪") == 0) {
            ++readyCount;
        }
    }
}

void DrawStatusDots(const StoryboardCardView* card) {
    auto icon = [](bool on, const char* glyph, const char* tip) {
        ImGui::SameLine();
        ImGui::TextColored(on ? kSuccess : kMuted, "%s", glyph);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tip);
        }
    };
    const bool linked =
        card != nullptr && card->link != nullptr && std::strcmp(card->link, "已关联") == 0;
    const bool preview =
        card != nullptr && card->preview != nullptr && std::strcmp(card->preview, "就绪") == 0;
    const bool exported =
        card != nullptr && card->exported != nullptr && std::strcmp(card->exported, "已导出") == 0;
    icon(linked, Icon::Camera, linked ? "已关联机位" : "无机位");
    icon(preview, Icon::Image, preview ? "预览就绪" : "预览未就绪");
    icon(exported, Icon::Download, exported ? "已导出" : "未导出");
}

void PushModalColors() {
    ImGui::PushStyleColor(ImGuiCol_Text, kText);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, kText);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.102f, 0.102f, 0.114f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.102f, 0.102f, 0.114f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.071f, 0.071f, 0.078f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.071f, 0.071f, 0.078f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.165f, 0.165f, 0.184f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.212f, 0.212f, 0.235f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.847f, 0.604f, 0.290f, 0.18f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
}

void PopModalColors() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(9);
}

void ModalBodyText(const char* text) {
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::CalcTextSize(text);
    ImGui::GetWindowDrawList()->AddText(pos, kModalTextU32, text);
    ImGui::Dummy(size);
}

bool ModalButton(const char* label) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 labelSize = ImGui::CalcTextSize(label);
    const ImVec2 size(labelSize.x + style.FramePadding.x * 2.0f,
                      labelSize.y + style.FramePadding.y * 2.0f);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("##modal-btn", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    ImU32 bg = IM_COL32(0x2B, 0x33, 0x40, 0xFF);
    if (held) {
        bg = IM_COL32(0x31, 0x54, 0x7D, 0xFF);
    } else if (hovered) {
        bg = IM_COL32(0x46, 0x52, 0x65, 0xFF);
    }
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 max(pos.x + size.x, pos.y + size.y);
    draw->AddRectFilled(pos, max, bg, style.FrameRounding);
    draw->AddRect(pos, max, IM_COL32(0x4A, 0x4A, 0x52, 0xFF), style.FrameRounding);
    draw->AddText(ImVec2(pos.x + style.FramePadding.x, pos.y + style.FramePadding.y), kModalTextU32,
                  label);
    ImGui::PopID();
    return pressed;
}

void ApplyDockLayout(ImGuiID dockspaceId, const ImVec2& size, const char* modeId, bool force,
                    bool leftCollapsed, bool empty, bool iconBar) {
    if (!force && ImGui::DockBuilderGetNode(dockspaceId) != nullptr) {
        return;
    }

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, size);

    const bool script = std::strcmp(modeId, "script") == 0;
    const bool set = std::strcmp(modeId, "set") == 0;
    const bool review = std::strcmp(modeId, "review") == 0;

    ImGuiID dockCenter = dockspaceId;
    ImGuiID dockLeft = 0;
    ImGuiID dockRight = 0;
    if (!empty && !iconBar) {
        const float leftRatio =
            leftCollapsed ? std::min(0.08f, 72.0f / std::max(size.x, 1.0f)) : 0.18f;
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, leftRatio, &dockLeft, &dockCenter);
    }
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.22f, &dockRight, &dockCenter);

    if (dockLeft != 0) {
        if (script || review) {
            ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        } else {
            ImGuiID dockLibrary = 0;
            ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, set ? 0.70f : 0.42f, &dockLibrary,
                                        &dockLeft);
            ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
            ImGui::DockBuilderDockWindow("Library", dockLibrary);
        }
    }
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    if (script) {
        ImGui::DockBuilderDockWindow("Script", dockCenter);
    } else if (review) {
        ImGui::DockBuilderDockWindow("Storyboard", dockCenter);
    } else {
        ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    }
    ImGui::DockBuilderFinish(dockspaceId);
}

ImVec2 FitExportFrame(const ImVec2& available, const char* resolutionId) {
    if (available.x <= 1.0f || available.y <= 1.0f) {
        return ImVec2(1.0f, 1.0f);
    }
    const float aspect = (resolutionId != nullptr && std::strcmp(resolutionId, "2k") == 0)
                             ? (2560.0f / 1440.0f)
                             : (1920.0f / 1080.0f);
    const float availAspect = available.x / available.y;
    if (availAspect > aspect) {
        return ImVec2(available.y * aspect, available.y);
    }
    return ImVec2(available.x, available.x / aspect);
}

void ApplyLeftColumnWidth(ImGuiID dockspaceId, float columnWidth, bool collapsed) {
    ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspaceId);
    if (root == nullptr || !root->IsSplitNode() || root->ChildNodes[0] == nullptr) {
        return;
    }
    ImGuiDockNode* left = root->ChildNodes[0];
    const float target = collapsed ? 72.0f : std::max(220.0f, columnWidth * 0.18f);
    if (std::abs(left->Size.x - target) < 8.0f) {
        return;
    }
    ImGui::DockBuilderSetNodeSize(left->ID, ImVec2(target, left->Size.y));
}

void ApplyAutoHideTabBar(ImGuiDockNode* node) {
    if (node == nullptr) {
        return;
    }
    node->SetLocalFlags(node->LocalFlags | ImGuiDockNodeFlags_AutoHideTabBar);
    if (node->IsSplitNode()) {
        ApplyAutoHideTabBar(node->ChildNodes[0]);
        ApplyAutoHideTabBar(node->ChildNodes[1]);
    }
}

void DrawModeButton(const AppViewState& state, Core::CommandQueue& commands, const char* id,
                    const char* label, bool enabled) {
    const bool current = ModeIs(state, id);
    if (!enabled) {
        ImGui::BeginDisabled();
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (current) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.165f, 0.165f, 0.184f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.165f, 0.165f, 0.184f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.165f, 0.165f, 0.184f, 1.0f));
    }
    if (ImGui::Button(label, ImVec2(0.0f, 28.0f)) && !current && enabled) {
        commands.Push(Core::SetWorkspaceModeCommand{id});
    }
    if (current) {
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(min.x, max.y - 2.0f), max,
                                                  ImGui::GetColorU32(kAccent));
        ImGui::PopStyleColor(3);
    }
    ImGui::PopStyleVar();
    if (!enabled) {
        ImGui::EndDisabled();
    }
}

bool DrawPrimaryButton(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.922f, 0.698f, 0.373f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccent);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.071f, 0.071f, 0.078f, 1.0f));
    const bool pressed = ImGui::Button(label, ImVec2(0.0f, 32.0f));
    ImGui::PopStyleColor(4);
    return pressed;
}

const ExportLogView* LastOkExport(const AppViewState& state) {
    if (state.exportLog == nullptr) {
        return nullptr;
    }
    for (int i = static_cast<int>(state.exportLog->size()) - 1; i >= 0; --i) {
        const ExportLogView& entry = (*state.exportLog)[static_cast<std::size_t>(i)];
        if (entry.ok && !entry.path.empty()) {
            return &entry;
        }
    }
    return nullptr;
}

void CountSelectedShotOrdinal(const AppViewState& state, int& ordinal, int& total) {
    ordinal = 0;
    total = 0;
    if (state.scriptScenes == nullptr) {
        return;
    }
    for (const ScriptSceneView& scene : *state.scriptScenes) {
        for (const ScriptShotView& shot : scene.shots) {
            ++total;
            if (shot.selected) {
                ordinal = total;
            }
        }
    }
}

void DrawOnboardingStep(bool done, const char* label) {
    bool checked = done;
    ImGui::BeginDisabled();
    ImGui::Checkbox(label, &checked);
    ImGui::EndDisabled();
}

void DrawOnboarding(const AppViewState& state, Core::CommandQueue& commands) {
    ImGui::TextUnformatted("无选择");
    ImGui::SeparatorText("上手三步");
    const bool step1 = state.scriptHasSnapshot;
    const bool step2 = state.nodes != nullptr && !state.nodes->empty();
    const bool step3 =
        state.selectedShotLinkedCamera != nullptr && state.selectedShotLinkedCamera[0] != '\0';

    DrawOnboardingStep(step1, "打开剧本");
    if (ImGui::Button("打开剧本...", ImVec2(-1.0f, 0.0f))) {
        commands.Push(Core::LoadScriptCommand{});
    }
    if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
        if (ImGui::Button("示例剧本", ImVec2(-1.0f, 0.0f))) {
            commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
        }
    }

    DrawOnboardingStep(step2, "放一个对象");
    if (ImGui::Button("导入模型...", ImVec2(-1.0f, 0.0f))) {
        commands.Push(Core::ImportModelCommand{});
    }

    DrawOnboardingStep(step3, "给镜头绑机位");
    if (ImGui::Button("新建机位", ImVec2(-1.0f, 0.0f))) {
        commands.Push(Core::BindShotToNewCameraCommand{});
    }

    ImGui::SeparatorText("快捷键");
    if (ImGui::BeginTable("OnboardingKeys", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, 88.0f);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
        const char* rows[][2] = {{"Ctrl+N", "新建"}, {"Ctrl+O", "打开"},   {"Ctrl+S", "保存"},
                                 {"Ctrl+I", "导入"}, {"Ctrl+E", "导出"},   {"[ ] / ↑↓", "切镜"},
                                 {"左键", "旋转视口"}};
        for (const auto& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("%s", row[0]);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(row[1]);
        }
        ImGui::EndTable();
    }
}

void DrawLightPresets(const AppViewState& state, Core::CommandQueue& commands) {
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
}

void DrawSceneInspector(const AppViewState& state, Core::CommandQueue& commands) {
    ImGui::SeparatorText("灯光");
    DrawLightPresets(state, commands);
    ImGui::SeparatorText("背景");
    ImGui::TextDisabled("中性灰");
}

void DrawAssetInspector(const LibraryAssetView& asset, Core::CommandQueue& commands) {
    ImGui::TextUnformatted(asset.name.c_str());
    ImGui::TextDisabled("%s  ·  %s", asset.format.c_str(), asset.origin.c_str());
    if (!asset.description.empty()) {
        ImGui::TextWrapped("%s", asset.description.c_str());
    }
    if (!asset.license.empty()) {
        ImGui::Text("许可: %s", asset.license.c_str());
    }
    if (!asset.author.empty()) {
        ImGui::Text("作者: %s", asset.author.c_str());
    }
    ImGui::Text("状态: %s  %.0f%%", asset.status.c_str(), asset.progress * 100.0f);
    if (asset.canDownload && ImGui::Button("下载")) {
        commands.Push(Core::DownloadOfficialAssetCommand{asset.id});
    }
    if (asset.canCancel) {
        ImGui::SameLine();
        if (ImGui::Button("取消下载")) {
            commands.Push(Core::CancelOfficialDownloadCommand{asset.id});
        }
    }
    if (asset.canAddToScene && ImGui::Button("加入场景", ImVec2(-1.0f, 0.0f))) {
        commands.Push(Core::AddLibraryAssetToSceneCommand{asset.id});
    }
    if (!asset.canDownload && !asset.canCancel && asset.origin != "online" &&
        ImGui::Button("从资源库删除")) {
        commands.Push(Core::RemoveLibraryAssetCommand{asset.id});
    }
}

void DrawShotInspector(const AppViewState& state, Core::CommandQueue& commands,
                       const ScriptShotView& shot, const ScriptSceneView* scene) {
    ImGui::PushID(shot.id.c_str());
    ImGui::TextUnformatted(shot.title.c_str());
    if (scene != nullptr) {
        ImGui::TextDisabled("%s", scene->title.c_str());
    }
    ImGui::SeparatorText("相机");
    if (!shot.linkedCameraName.empty()) {
        ImGui::TextColored(kSuccess, "● %s%s", shot.linkedCameraName.c_str(),
                           shot.linkedMissing ? " (缺失)" : "");
    } else {
        ImGui::TextDisabled("无机位");
    }
    const char* cameraPreview =
        shot.linkedCameraName.empty() ? "选择已有相机" : shot.linkedCameraName.c_str();
    if (state.cameras != nullptr && !state.cameras->empty()) {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##shot-camera", cameraPreview)) {
            for (const CameraItemView& camera : *state.cameras) {
                const bool selected = camera.id == shot.linkedCameraId;
                if (ImGui::Selectable(camera.name.c_str(), selected)) {
                    commands.Push(Core::LinkShotToCameraCommand{shot.id, camera.id});
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::TextDisabled("还没有相机，可先建机位");
    }
    if (ImGui::Button("新建机位")) {
        commands.Push(Core::BindShotToNewCameraCommand{shot.id});
    }
    if (!shot.linkedCameraName.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("解绑")) {
            commands.Push(Core::UnlinkShotCommand{shot.id});
        }
    }
    ImGui::TextDisabled("机位预设");
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float presetWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;
    static constexpr const char* kPresetIds[] = {"front",         "side",     "over-shoulder",
                                                 "top",           "close-up", "eye-level"};
    static constexpr const char* kPresetLabels[] = {"正视", "侧面", "过肩", "俯视", "特写", "平视"};
    for (int i = 0; i < 6; ++i) {
        if (i % 3 != 0) {
            ImGui::SameLine();
        }
        if (ImGui::Button(kPresetLabels[i], ImVec2(presetWidth, 0.0f))) {
            commands.Push(Core::ApplyCameraPresetCommand{kPresetIds[i]});
        }
    }
    const StoryboardCardView* card = FindShotCard(state, shot.id);
    ImGui::SeparatorText("预览");
    const char* previewLabel = "无";
    if (card != nullptr && card->preview != nullptr) {
        if (std::strcmp(card->preview, "就绪") == 0) {
            previewLabel = "最新";
        } else if (std::strcmp(card->preview, "过期") == 0) {
            previewLabel = "需刷新";
        } else if (std::strcmp(card->preview, "失败") == 0) {
            previewLabel = "失败";
        } else if (std::strcmp(card->preview, "缺失") == 0) {
            previewLabel = "无";
        } else {
            previewLabel = card->preview;
        }
    }
    ImGui::Text("预览: %s", previewLabel);
    ImGui::SameLine();
    if (ImGui::SmallButton("刷新预览")) {
        commands.Push(Core::RefreshStoryboardThumbnailCommand{shot.id});
    }
    ImGui::PopID();
}

void PushNodeTransform(Core::CommandQueue& commands, const std::string& nodeId, const float position[3],
                       const float euler[3], const float scale[3]) {
    Core::SetNodeTransformCommand transform;
    transform.nodeId = nodeId;
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

void DrawNodeInspector(const AppViewState& state, Core::CommandQueue& commands, const NodeView& node) {
    ImGui::PushID(node.id.c_str());
    ImGui::TextUnformatted(node.name.c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("对象为所有镜头共享");
    }
    float position[3] = {node.position[0], node.position[1], node.position[2]};
    float euler[3] = {node.eulerDegrees[0], node.eulerDegrees[1], node.eulerDegrees[2]};
    float scale[3] = {node.scale[0], node.scale[1], node.scale[2]};
    bool changed = false;
    if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("位置");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        changed = ImGui::DragFloat3("##position", position, 0.01f) || changed;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("旋转");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        changed = ImGui::DragFloat3("##rotation", euler, 0.5f) || changed;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("缩放");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        changed = ImGui::DragFloat3("##scale", scale, 0.01f) || changed;
        ImGui::EndTable();
    }
    if (changed) {
        PushNodeTransform(commands, node.id, position, euler, scale);
    }
    if (ImGui::Button("对齐地面")) {
        position[1] = 0.0f;
        PushNodeTransform(commands, node.id, position, euler, scale);
    }
    ImGui::SameLine();
    if (ImGui::Button("面向相机")) {
        const float dx = state.selectedCameraPosition[0] - position[0];
        const float dy = state.selectedCameraPosition[1] - position[1];
        const float dz = state.selectedCameraPosition[2] - position[2];
        const float horiz = std::sqrt(dx * dx + dz * dz);
        if (horiz > 1.0e-5f || std::fabs(dy) > 1.0e-5f) {
            euler[0] = std::atan2(-dy, std::max(horiz, 1.0e-5f)) * (180.0f / 3.14159265f);
            euler[1] = std::atan2(dx, dz) * (180.0f / 3.14159265f);
            euler[2] = 0.0f;
            PushNodeTransform(commands, node.id, position, euler, scale);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("复位")) {
        const float origin[3] = {0.0f, 0.0f, 0.0f};
        const float identityEuler[3] = {0.0f, 0.0f, 0.0f};
        const float identityScale[3] = {1.0f, 1.0f, 1.0f};
        PushNodeTransform(commands, node.id, origin, identityEuler, identityScale);
    }
    if (ImGui::SmallButton(node.visible ? "隐藏" : "显示")) {
        commands.Push(Core::SetNodeVisibleCommand{node.id, !node.visible});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("复制")) {
        commands.Push(Core::DuplicateNodeCommand{node.id});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("删除")) {
        commands.Push(Core::DeleteNodeCommand{node.id});
    }
    ImGui::PopID();
}

void DrawCameraInspector(const AppViewState& state, Core::CommandQueue& commands,
                         const CameraItemView& camera, std::string& cameraName,
                         std::string& cameraNameId) {
    ImGui::PushID(camera.id.c_str());
    if (cameraNameId != camera.id) {
        cameraName = camera.name;
        cameraNameId = camera.id;
    }
    char nameBuffer[128];
    const std::size_t copy =
        cameraName.size() < sizeof(nameBuffer) - 1 ? cameraName.size() : sizeof(nameBuffer) - 1;
    std::memcpy(nameBuffer, cameraName.data(), copy);
    nameBuffer[copy] = '\0';
    if (ImGui::InputText("名称", nameBuffer, sizeof(nameBuffer))) {
        cameraName = nameBuffer;
        commands.Push(Core::RenameCameraCommand{camera.id, cameraName});
    }
    ImGui::SeparatorText("占用此相机的镜头");
    const ScriptShotView* liveShot = FindSelectedShot(state, nullptr);
    const char* liveShotId = liveShot != nullptr ? liveShot->id.c_str() : "";
    bool anyLinked = false;
    bool anyLinkable = false;
    if (state.scriptScenes != nullptr) {
        for (const ScriptSceneView& scene : *state.scriptScenes) {
            for (const ScriptShotView& shot : scene.shots) {
                if (shot.id == liveShotId) {
                    continue;
                }
                if (shot.linkedCameraId == camera.id) {
                    anyLinked = true;
                    if (ImGui::Selectable(shot.title.c_str())) {
                        commands.Push(Core::SelectShotCommand{shot.id});
                    }
                }
            }
        }
        for (const ScriptSceneView& scene : *state.scriptScenes) {
            for (const ScriptShotView& shot : scene.shots) {
                if (shot.id == liveShotId || shot.linkedCameraId == camera.id) {
                    continue;
                }
                anyLinkable = true;
                ImGui::PushID(shot.id.c_str());
                ImGui::TextUnformatted(shot.title.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("关联")) {
                    commands.Push(Core::LinkShotToCameraCommand{shot.id, camera.id});
                }
                ImGui::PopID();
            }
        }
    }
    if (liveShot != nullptr && liveShot->linkedCameraId == camera.id) {
        ImGui::TextDisabled("当前画面：%s（不参与关联）", liveShot->title.c_str());
    }
    if (!anyLinked && !anyLinkable) {
        ImGui::TextDisabled("没有可关联的其他镜头");
    }
    ImGui::PopID();
}

void DrawDeliveryInspector(const AppViewState& state, Core::CommandQueue& commands) {
    if (state.viewportTextureIndex != 0xFFFFu) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const float monitorW = avail.x;
        const float monitorH = std::min(monitorW * 9.0f / 16.0f, 160.0f);
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(monitorW, monitorH));
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddImage(ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)), cursor,
                       ImVec2(cursor.x + monitorW, cursor.y + monitorH));
        draw->AddRect(cursor, ImVec2(cursor.x + monitorW, cursor.y + monitorH),
                      ImGui::GetColorU32(ImGuiCol_Border));
    }
    ImGui::SeparatorText("交付");
    bool exportTransparent = state.exportTransparent;
    if (ImGui::Checkbox("透明背景", &exportTransparent)) {
        commands.Push(Core::SetExportTransparentCommand{exportTransparent});
    }
    const bool res2k =
        state.exportResolutionId != nullptr && std::strcmp(state.exportResolutionId, "2k") == 0;
    const char* resolutionPreview = res2k ? "2K" : "1080p";
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##export-resolution", resolutionPreview)) {
        if (ImGui::Selectable("1080p", !res2k)) {
            commands.Push(Core::SelectExportResolutionCommand{"1080p"});
        }
        if (ImGui::Selectable("2K", res2k)) {
            commands.Push(Core::SelectExportResolutionCommand{"2k"});
        }
        ImGui::EndCombo();
    }
    if (state.exportIssues != nullptr && !state.exportIssues->empty()) {
        ImGui::SeparatorText("未就绪");
        for (const ExportIssueView& issue : *state.exportIssues) {
            ImGui::PushID(issue.shotId.c_str());
            const char* reasonText = "";
            if (issue.reason != nullptr) {
                if (std::strcmp(issue.reason, "未关联相机") == 0) {
                    reasonText = "无机位";
                } else if (std::strcmp(issue.reason, "缩略图过期") == 0) {
                    reasonText = "预览需刷新";
                } else if (std::strcmp(issue.reason, "缺少缩略图") == 0) {
                    reasonText = "预览无";
                } else if (std::strcmp(issue.reason, "渲染失败") == 0) {
                    reasonText = "预览失败";
                } else {
                    reasonText = issue.reason;
                }
            }
            ImGui::TextWrapped("%s · %s", issue.shotTitle.c_str(), reasonText);
            if (issue.reason != nullptr && std::strcmp(issue.reason, "未关联相机") == 0) {
                if (ImGui::SmallButton("新建机位")) {
                    commands.Push(Core::BindShotToNewCameraCommand{issue.shotId});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("关联当前")) {
                    commands.Push(Core::LinkShotToCameraCommand{issue.shotId, ""});
                }
            } else if (ImGui::SmallButton("刷新预览")) {
                commands.Push(Core::RefreshStoryboardThumbnailCommand{issue.shotId});
            }
            ImGui::PopID();
        }
    }
    if (ImGui::Button("导出总览", ImVec2(-1.0f, 0.0f))) {
        commands.Push(Core::ExportStoryboardBoardCommand{});
    }
}

void DrawScriptInspector(const AppViewState& state) {
    int scenes = 0;
    int shots = 0;
    if (state.scriptScenes != nullptr) {
        scenes = static_cast<int>(state.scriptScenes->size());
        for (const ScriptSceneView& scene : *state.scriptScenes) {
            shots += static_cast<int>(scene.shots.size());
        }
    }
    const int diagnostics =
        state.scriptDiagnostics != nullptr ? static_cast<int>(state.scriptDiagnostics->size()) : 0;
    ImGui::Text("场次 %d  ·  镜头 %d  ·  诊断 %d", scenes, shots, diagnostics);
    ImGui::SeparatorText("诊断");
    if (state.scriptDiagnostics == nullptr || state.scriptDiagnostics->empty()) {
        ImGui::TextUnformatted("无");
        return;
    }
    for (const ScriptDiagnosticView& diagnostic : *state.scriptDiagnostics) {
        ImGui::TextColored(kAccent, "%s  L%d  [%s]",
                           diagnostic.severity != nullptr && diagnostic.severity[0] != '\0'
                               ? diagnostic.severity
                               : "提示",
                           diagnostic.line, diagnostic.code != nullptr ? diagnostic.code : "");
        ImGui::TextWrapped("%s", diagnostic.message != nullptr ? diagnostic.message : "");
    }
}

bool ComputeLeftIconBar(const AppViewState& state, bool empty, bool foldExplicit, bool folded) {
    if (empty) {
        return false;
    }
    if (foldExplicit) {
        return folded;
    }
    const float windowW = state.windowWidth > 0 ? static_cast<float>(state.windowWidth)
                                                : ImGui::GetMainViewport()->Size.x;
    return ModeIs(state, "review") || windowW < 1180.0f;
}

void DrawLeftIconBar(const AppViewState& state, LeftRailState& rail) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiWindowFlags sideFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 8.0f));
    if (ImGui::BeginViewportSideBar("##LeftIconBar", viewport, ImGuiDir_Left, 48.0f, sideFlags)) {
        const float btn = 40.0f;
        auto railBtn = [&](const char* icon, const char* tip, LeftRailOverlay id) {
            const bool on = rail.overlay == id;
            if (on) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.847f, 0.604f, 0.290f, 0.18f));
            }
            ImGui::PushID(tip);
            if (ImGui::Button(icon, ImVec2(btn, btn))) {
                rail.overlay = on ? LeftRailOverlay::None : id;
            }
            ImGui::PopID();
            if (on) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", tip);
            }
        };
        railBtn(Icon::Clapperboard, "镜头表", LeftRailOverlay::Shots);
        railBtn(Icon::Box, "场景", LeftRailOverlay::Scene);
        if (ModeIs(state, "set") || ModeIs(state, "shoot")) {
            railBtn(Icon::Package, "资源库", LeftRailOverlay::Library);
        } else if (rail.overlay == LeftRailOverlay::Library) {
            rail.overlay = LeftRailOverlay::None;
        }
        const ImVec2 barPos = ImGui::GetWindowPos();
        const ImVec2 barSize = ImGui::GetWindowSize();
        rail.overlayPos = ImVec2(barPos.x + barSize.x, viewport->WorkPos.y);
        rail.overlaySize = ImVec2(280.0f, viewport->WorkSize.y);
        ImGui::End();
    }
    ImGui::PopStyleVar(2);
}

void DrawHierarchyBody(const AppViewState& state, Core::CommandQueue& commands,
                       bool* sceneHeaderClicked) {
    bool shotAddClicked = false;
    if (BeginSection("镜头表", true, nullptr, &shotAddClicked)) {
        if (state.scriptScenes == nullptr || state.scriptScenes->empty()) {
            ImGui::TextDisabled("还没有镜头。打开或新建剧本");
            if (ImGui::SmallButton("打开剧本")) {
                commands.Push(Core::LoadScriptCommand{});
            }
            if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
                ImGui::SameLine();
                if (ImGui::SmallButton("示例")) {
                    commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
                }
            }
        } else {
            for (const ScriptSceneView& scene : *state.scriptScenes) {
                if (ImGui::TreeNodeEx(scene.id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s",
                                      scene.title.c_str())) {
                    for (const ScriptShotView& shot : scene.shots) {
                        const std::string label = shot.title + "##" + shot.id;
                        if (ImGui::Selectable(label.c_str(), shot.selected)) {
                            commands.Push(Core::SelectShotCommand{shot.id});
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("在此后插入镜头")) {
                                commands.Push(Core::InsertShotCommand{shot.id});
                            }
                            if (ImGui::MenuItem("删除镜头")) {
                                commands.Push(Core::DeleteShotCommand{shot.id});
                            }
                            ImGui::EndPopup();
                        }
                        DrawStatusDots(FindShotCard(state, shot.id));
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    bool sceneAddClicked = false;
    if (BeginSection("场景", !ModeIs(state, "script"), sceneHeaderClicked, &sceneAddClicked)) {
        if (state.nodes == nullptr || state.nodes->empty()) {
            ImGui::TextDisabled("场景是空的。把资源库里的模型拖到画面里");
            if (ImGui::SmallButton("导入模型")) {
                commands.Push(Core::ImportModelCommand{});
            }
        } else {
            for (const NodeView& node : *state.nodes) {
                ImGui::PushID(node.id.c_str());
                const std::string nodeLabel =
                    std::string(Icon::Box) + "  " + node.name + "##sel";
                if (ImGui::Selectable(nodeLabel.c_str(), node.selected,
                                    ImGuiSelectableFlags_AllowOverlap, ImVec2(-28.0f, 0.0f))) {
                    commands.Push(Core::SelectNodeCommand{node.id});
                }
                if (ImGui::BeginPopupContextItem("##node-menu")) {
                    if (ImGui::MenuItem("复制")) {
                        commands.Push(Core::DuplicateNodeCommand{node.id});
                    }
                    if (ImGui::MenuItem(node.visible ? "隐藏" : "显示")) {
                        commands.Push(Core::SetNodeVisibleCommand{node.id, !node.visible});
                    }
                    if (ImGui::MenuItem("删除")) {
                        commands.Push(Core::DeleteNodeCommand{node.id});
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                const char* eye = node.visible ? Icon::Eye : Icon::EyeOff;
                if (ImGui::SmallButton(eye)) {
                    commands.Push(Core::SetNodeVisibleCommand{node.id, !node.visible});
                }
                ImGui::PopID();
            }
        }
        if (ImGui::TreeNodeEx("##cameras", ImGuiTreeNodeFlags_DefaultOpen, "相机")) {
            if (state.cameras != nullptr) {
                for (const CameraItemView& camera : *state.cameras) {
                    const std::string label =
                        std::string(Icon::Camera) + "  " + camera.name + "##" + camera.id;
                    if (ImGui::Selectable(label.c_str(), camera.selected)) {
                        commands.Push(Core::SelectCameraCommand{camera.id});
                    }
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("删除相机")) {
                            commands.Push(Core::RemoveCameraCommand{camera.id});
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::TreePop();
        }
    }
    if (shotAddClicked) {
        ImGui::OpenPopup("##hierarchy-shots-add");
    }
    if (ImGui::BeginPopup("##hierarchy-shots-add")) {
        if (ImGui::MenuItem("场次")) {
            commands.Push(Core::InsertSceneCommand{});
        }
        if (ImGui::MenuItem("镜头")) {
            commands.Push(Core::InsertShotCommand{});
        }
        ImGui::EndPopup();
    }
    if (sceneAddClicked) {
        ImGui::OpenPopup("##hierarchy-scene-add");
    }
    if (ImGui::BeginPopup("##hierarchy-scene-add")) {
        if (ImGui::MenuItem("添加相机")) {
            commands.Push(Core::AddCameraCommand{});
        }
        ImGui::EndPopup();
    }
}

void DrawViewportGuides(const ImVec2& frameMin, const ImVec2& frameMax, bool thirds, bool safe) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 color = IM_COL32(0xE6, 0xE6, 0xE6, 48);
    const float width = frameMax.x - frameMin.x;
    const float height = frameMax.y - frameMin.y;
    if (thirds && width > 2.0f && height > 2.0f) {
        for (int i = 1; i <= 2; ++i) {
            const float x = frameMin.x + width * (static_cast<float>(i) / 3.0f);
            const float y = frameMin.y + height * (static_cast<float>(i) / 3.0f);
            draw->AddLine(ImVec2(x, frameMin.y), ImVec2(x, frameMax.y), color);
            draw->AddLine(ImVec2(frameMin.x, y), ImVec2(frameMax.x, y), color);
        }
    }
    if (safe && width > 2.0f && height > 2.0f) {
        const float insetX = width * 0.05f;
        const float insetY = height * 0.05f;
        draw->AddRect(ImVec2(frameMin.x + insetX, frameMin.y + insetY),
                      ImVec2(frameMax.x - insetX, frameMax.y - insetY), color);
    }
}

} // namespace

void WorkspacePanel::ApplyPreferences(const UiPreferences& preferences) {
    m_lockExportAspect = preferences.lockExportAspect;
    m_leftFoldExplicit = preferences.leftFoldExplicit;
    m_leftFolded = preferences.leftFolded;
    m_leftFoldDirty = true;
    m_showGroundGrid = preferences.showGroundGrid;
    m_showGroundAxes = preferences.showGroundAxes;
    m_showThirds = preferences.showThirds;
    m_showSafeFrame = preferences.showSafeFrame;
    m_viewportBackground =
        preferences.viewportBackground == "dark" ? "dark" : "neutral";
    m_preferencesDirty = false;
}

UiPreferences WorkspacePanel::Preferences() const {
    UiPreferences preferences;
    preferences.lockExportAspect = m_lockExportAspect;
    preferences.leftFoldExplicit = m_leftFoldExplicit;
    preferences.leftFolded = m_leftFolded;
    preferences.showGroundGrid = m_showGroundGrid;
    preferences.showGroundAxes = m_showGroundAxes;
    preferences.showThirds = m_showThirds;
    preferences.showSafeFrame = m_showSafeFrame;
    preferences.viewportBackground = m_viewportBackground;
    return preferences;
}

bool WorkspacePanel::ConsumePreferencesDirty() {
    const bool dirty = m_preferencesDirty;
    m_preferencesDirty = false;
    return dirty;
}

void WorkspacePanel::MarkPreferencesDirty() {
    m_preferencesDirty = true;
}

void WorkspacePanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool empty = ProjectIsEmpty(state);
    const char* modeId = ModeId(state);
    bool sceneHeaderClicked = false;
    LeftRailState& rail = CurrentLeftRail();
    const bool iconBar = ComputeLeftIconBar(state, empty, m_leftFoldExplicit, m_leftFolded);
    if (iconBar != m_lastIconBar) {
        m_leftFoldDirty = true;
        m_lastIconBar = iconBar;
        if (!iconBar) {
            rail.overlay = LeftRailOverlay::None;
        }
    }
    rail.iconBar = iconBar;

    const ImGuiWindowFlags menuFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
    if (ImGui::BeginViewportSideBar("##MenuBar", viewport, ImGuiDir_Up, 26.0f, menuFlags)) {
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
                if (ImGui::MenuItem("按当前选择导出", "Ctrl+E")) {
                    commands.Push(Core::ExportCurrentShotCommand{});
                }
                if (ImGui::MenuItem("导出镜头 1080p")) {
                    commands.Push(Core::ExportCurrentShotCommand{"1080p"});
                }
                if (ImGui::MenuItem("导出镜头 2K")) {
                    commands.Push(Core::ExportCurrentShotCommand{"2k"});
                }
                if (ImGui::MenuItem("导出总览")) {
                    commands.Push(Core::ExportStoryboardBoardCommand{});
                }
                ImGui::Separator();
                if (ImGui::MenuItem("退出")) {
                    commands.Push(Core::QuitCommand{});
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("视图")) {
                if (ImGui::MenuItem("编剧", nullptr, ModeIs(state, "script"), !empty)) {
                    commands.Push(Core::SetWorkspaceModeCommand{"script"});
                }
                if (ImGui::MenuItem("置景", nullptr, ModeIs(state, "set"), !empty)) {
                    commands.Push(Core::SetWorkspaceModeCommand{"set"});
                }
                if (ImGui::MenuItem("掌机", nullptr, ModeIs(state, "shoot"))) {
                    commands.Push(Core::SetWorkspaceModeCommand{"shoot"});
                }
                if (ImGui::MenuItem("审片", nullptr, ModeIs(state, "review"), !empty)) {
                    commands.Push(Core::SetWorkspaceModeCommand{"review"});
                }
                ImGui::Separator();
                if (ImGui::MenuItem("锁定导出比例", nullptr, m_lockExportAspect)) {
                    m_lockExportAspect = !m_lockExportAspect;
                    MarkPreferencesDirty();
                }
                if (ImGui::MenuItem("地面网格", "G", m_showGroundGrid)) {
                    m_showGroundGrid = !m_showGroundGrid;
                    MarkPreferencesDirty();
                }
                if (ImGui::MenuItem("坐标轴", nullptr, m_showGroundAxes)) {
                    m_showGroundAxes = !m_showGroundAxes;
                    MarkPreferencesDirty();
                }
                if (ImGui::MenuItem("三分线", "Shift+G", m_showThirds)) {
                    m_showThirds = !m_showThirds;
                    MarkPreferencesDirty();
                }
                if (ImGui::MenuItem("安全框", nullptr, m_showSafeFrame)) {
                    m_showSafeFrame = !m_showSafeFrame;
                    MarkPreferencesDirty();
                }
                if (ImGui::BeginMenu("视口背景")) {
                    if (ImGui::MenuItem("中性灰", nullptr, m_viewportBackground != "dark")) {
                        m_viewportBackground = "neutral";
                        MarkPreferencesDirty();
                    }
                    if (ImGui::MenuItem("深灰", nullptr, m_viewportBackground == "dark")) {
                        m_viewportBackground = "dark";
                        MarkPreferencesDirty();
                    }
                    ImGui::EndMenu();
                }
                const bool leftCollapsed =
                    ComputeLeftIconBar(state, empty, m_leftFoldExplicit, m_leftFolded);
                if (ImGui::MenuItem("折叠左栏", nullptr, leftCollapsed)) {
                    m_leftFoldExplicit = true;
                    m_leftFolded = !leftCollapsed;
                    m_leftFoldDirty = true;
                    MarkPreferencesDirty();
                }
                if (ImGui::MenuItem("重置布局")) {
                    commands.Push(Core::ResetLayoutCommand{});
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("镜头")) {
                if (ImGui::MenuItem("正视机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"front"});
                }
                if (ImGui::MenuItem("侧面机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"side"});
                }
                if (ImGui::MenuItem("过肩机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"over-shoulder"});
                }
                if (ImGui::MenuItem("俯视机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"top"});
                }
                if (ImGui::MenuItem("特写机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"close-up"});
                }
                if (ImGui::MenuItem("平视机位")) {
                    commands.Push(Core::ApplyCameraPresetCommand{"eye-level"});
                }
                ImGui::Separator();
                if (ImGui::MenuItem("新建机位")) {
                    commands.Push(Core::BindShotToNewCameraCommand{});
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助")) {
                ImGui::TextUnformatted("Ctrl+N  新建工程");
                ImGui::TextUnformatted("Ctrl+O  打开工程");
                ImGui::TextUnformatted("Ctrl+S  保存工程");
                ImGui::TextUnformatted("Ctrl+Shift+S  工程另存为");
                ImGui::TextUnformatted("Ctrl+I  导入模型");
                ImGui::TextUnformatted("Ctrl+E  按当前选择导出");
                ImGui::TextUnformatted("[ ] / ↑↓  上一镜 / 下一镜");
                ImGui::Separator();
                ImGui::TextUnformatted("视口：左键旋转  右键平移  滚轮缩放");
                ImGui::TextUnformatted("G  地面网格    Shift+G  三分线");
                ImGui::TextUnformatted("分镜：右键平移  滚轮缩放");
                ImGui::EndMenu();
            }
            const char* projectName = state.projectName != nullptr && state.projectName[0] != '\0'
                                          ? state.projectName
                                          : "未命名工程";
            const std::string projectStatus =
                std::string(state.projectDirty ? "●  " : "") + projectName;
            const float statusWidth = ImGui::CalcTextSize(projectStatus.c_str()).x + 14.0f;
            const float rightAligned = ImGui::GetWindowWidth() - statusWidth;
            if (rightAligned > ImGui::GetCursorPosX()) {
                ImGui::SetCursorPosX(rightAligned);
            }
            ImGui::TextColored(state.projectDirty ? kAccent : kMuted, "%s", projectStatus.c_str());
            if (ImGui::IsItemHovered() && state.projectPath != nullptr &&
                state.projectPath[0] != '\0') {
                ImGui::SetTooltip("%s", state.projectPath);
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    const ImGuiWindowFlags sideFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 4.0f));
    if (ImGui::BeginViewportSideBar("##ToolStrip", viewport, ImGuiDir_Up, 40.0f, sideFlags)) {
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
        char scriptMode[32];
        char setMode[32];
        char shootMode[32];
        char reviewMode[32];
        std::snprintf(scriptMode, sizeof(scriptMode), "%s  编剧", Icon::BookOpen);
        std::snprintf(setMode, sizeof(setMode), "%s  置景", Icon::Box);
        std::snprintf(shootMode, sizeof(shootMode), "%s  掌机", Icon::Video);
        std::snprintf(reviewMode, sizeof(reviewMode), "%s  审片", Icon::LayoutGrid);
        DrawModeButton(state, commands, "script", scriptMode, !empty);
        ImGui::SameLine();
        DrawModeButton(state, commands, "set", setMode, !empty);
        ImGui::SameLine();
        DrawModeButton(state, commands, "shoot", shootMode, true);
        ImGui::SameLine();
        DrawModeButton(state, commands, "review", reviewMode, !empty);
        ImGui::PopStyleVar();

        const ScriptSceneView* selectedScene = nullptr;
        const ScriptShotView* selectedShot = FindSelectedShot(state, &selectedScene);
        int shotOrdinal = 0;
        int shotTotal = 0;
        CountSelectedShotOrdinal(state, shotOrdinal, shotTotal);
        char centerText[256];
        if (selectedShot != nullptr && selectedScene != nullptr) {
            std::snprintf(centerText, sizeof(centerText), "%s · %s (%d/%d)",
                          selectedShot->title.c_str(), selectedScene->title.c_str(), shotOrdinal,
                          shotTotal);
        } else {
            std::snprintf(centerText, sizeof(centerText), "%s", "未选镜头");
        }
        const float stepperW = ImGui::GetFrameHeight();
        const float centerWidth = stepperW * 2.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f +
                                  ImGui::CalcTextSize(centerText).x;
        const float centerX = (ImGui::GetWindowWidth() - centerWidth) * 0.5f;
        if (centerX > ImGui::GetCursorPosX() + 16.0f) {
            ImGui::SameLine(centerX);
        } else {
            ImGui::SameLine(0.0f, 16.0f);
        }
        ImGui::BeginDisabled(empty);
        char prevShot[24];
        std::snprintf(prevShot, sizeof(prevShot), "%s##prevShot", Icon::ChevronLeft);
        if (ImGui::SmallButton(prevShot)) {
            commands.Push(Core::SelectAdjacentShotCommand{-1});
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("上一镜  [  ↑");
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(centerText);
        ImGui::SameLine();
        char nextShot[24];
        std::snprintf(nextShot, sizeof(nextShot), "%s##nextShot", Icon::ChevronRight);
        if (ImGui::SmallButton(nextShot)) {
            commands.Push(Core::SelectAdjacentShotCommand{1});
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("下一镜  ]  ↓");
        }
        ImGui::EndDisabled();

        if (selectedShot != nullptr) {
            ImGui::SameLine(0.0f, 12.0f);
            ImGui::BeginGroup();
            DrawStatusDots(FindShotCard(state, selectedShot->id));
            ImGui::EndGroup();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("机位 · 预览 · 导出");
            }
        }

        const char* primaryLabel = "导出镜头 ▾";
        if (ModeIs(state, "script")) {
            primaryLabel = "保存剧本";
        } else if (ModeIs(state, "set")) {
            primaryLabel = "导入模型";
        } else if (ModeIs(state, "review")) {
            primaryLabel = "导出总览";
        }
        const float primaryWidth = ImGui::CalcTextSize(primaryLabel).x + 28.0f;
        const float primaryX = ImGui::GetWindowWidth() - primaryWidth - 12.0f;
        if (primaryX > ImGui::GetCursorPosX()) {
            ImGui::SameLine(primaryX);
        } else {
            ImGui::SameLine();
        }
        if (ModeIs(state, "script")) {
            if (DrawPrimaryButton(primaryLabel)) {
                commands.Push(Core::SaveScriptCommand{});
            }
        } else if (ModeIs(state, "set")) {
            if (DrawPrimaryButton(primaryLabel)) {
                commands.Push(Core::ImportModelCommand{});
            }
        } else if (ModeIs(state, "review")) {
            if (DrawPrimaryButton(primaryLabel)) {
                commands.Push(Core::ExportStoryboardBoardCommand{});
            }
        } else {
            if (DrawPrimaryButton(primaryLabel)) {
                ImGui::OpenPopup("##PrimaryExportShot");
            }
            if (ImGui::BeginPopup("##PrimaryExportShot")) {
                if (ImGui::MenuItem("1080p")) {
                    commands.Push(Core::ExportCurrentShotCommand{"1080p"});
                }
                if (ImGui::MenuItem("2K")) {
                    commands.Push(Core::ExportCurrentShotCommand{"2k"});
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
    ImGui::PopStyleVar();

    if (ImGui::BeginViewportSideBar("##StatusBar", viewport, ImGuiDir_Down, 28.0f, sideFlags)) {
        ImGui::AlignTextToFramePadding();
        const bool hasStatus = state.statusText != nullptr && state.statusText[0] != '\0';
        const bool hasExportIssues = ModeIs(state, "review") && state.exportIssues != nullptr &&
                                     !state.exportIssues->empty();
        const bool loadingScene = state.sceneLoadPending > 0 && state.sceneLoadTotal > 0;
        const bool warning =
            state.importInProgress || hasExportIssues || loadingScene || state.projectSaveInProgress;
        const ImVec4& statusDot = warning ? kWarning : (hasStatus ? kMuted : kSuccess);
        ImGui::TextColored(statusDot, "%s", warning ? Icon::TriangleAlert : Icon::CircleCheck);
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::TextUnformatted(hasStatus ? state.statusText : "就绪");
        if (const ExportLogView* exported = LastOkExport(state)) {
            ImGui::SameLine(0.0f, 10.0f);
            char openFile[32];
            char openFolder[32];
            std::snprintf(openFile, sizeof(openFile), "%s 打开", Icon::FileInput);
            std::snprintf(openFolder, sizeof(openFolder), "%s 文件夹", Icon::FolderOpen);
            if (ImGui::SmallButton(openFile)) {
                commands.Push(Core::RevealPathCommand{exported->path, false});
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(openFolder)) {
                commands.Push(Core::RevealPathCommand{exported->path, true});
            }
        }
        if (state.importInProgress) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kWarning, "导入中");
        }
        if (state.projectSaveInProgress) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kWarning, "正在校验资产");
        }
        if (state.sceneLoadPending > 0 && state.sceneLoadTotal > 0) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kWarning, "加载模型 %u/%u", state.sceneLoadPending,
                               state.sceneLoadTotal);
        }
        if (state.officialCatalogStatus != nullptr && state.officialCatalogStatus[0] != '\0') {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextDisabled("%s", state.officialCatalogStatus);
        }
        if (hasExportIssues) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kWarning, "%d 项未就绪",
                               static_cast<int>(state.exportIssues->size()));
        }
        int shotCount = 0;
        int readyCount = 0;
        CountShots(state, shotCount, readyCount);
        char rightText[64];
        std::snprintf(rightText, sizeof(rightText), "%d 镜 · %d 就绪", shotCount, readyCount);
        const float dirtyW = state.projectDirty ? ImGui::CalcTextSize("  ●").x : 0.0f;
        const float rightWidth = ImGui::CalcTextSize(rightText).x + dirtyW + 12.0f;
        const float rightX = ImGui::GetWindowWidth() - rightWidth;
        if (rightX > ImGui::GetCursorPosX()) {
            ImGui::SameLine(rightX);
            ImGui::TextDisabled("%s", rightText);
            if (state.projectDirty) {
                ImGui::SameLine(0.0f, 8.0f);
                ImGui::TextColored(kAccent, "●");
            }
        }
        ImGui::End();
    }

    if (!empty && !ModeIs(state, "script")) {
        if (ImGui::BeginViewportSideBar("镜头条###ShotStrip", viewport, ImGuiDir_Down, 132.0f,
                                        sideFlags)) {
            ImGui::End();
        }
    }

    if (iconBar) {
        DrawLeftIconBar(state, rail);
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
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
            commands.Push(Core::ExportCurrentShotCommand{});
        } else if (!ctrl && !io.KeyAlt && !io.KeySuper) {
            if (ImGui::IsKeyPressed(ImGuiKey_1, false) && !empty) {
                commands.Push(Core::SetWorkspaceModeCommand{"script"});
            } else if (ImGui::IsKeyPressed(ImGuiKey_2, false) && !empty) {
                commands.Push(Core::SetWorkspaceModeCommand{"set"});
            } else if (ImGui::IsKeyPressed(ImGuiKey_3, false)) {
                commands.Push(Core::SetWorkspaceModeCommand{"shoot"});
            } else if (ImGui::IsKeyPressed(ImGuiKey_4, false) && !empty) {
                commands.Push(Core::SetWorkspaceModeCommand{"review"});
            } else if (!empty && (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false) ||
                                  ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))) {
                commands.Push(Core::SelectAdjacentShotCommand{-1});
            } else if (!empty && (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false) ||
                                  ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))) {
                commands.Push(Core::SelectAdjacentShotCommand{1});
            } else if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
                if (io.KeyShift) {
                    m_showThirds = !m_showThirds;
                } else {
                    m_showGroundGrid = !m_showGroundGrid;
                }
                MarkPreferencesDirty();
            }
        }
    }

    const ImGuiID dockspaceId = ImGui::GetID("DirectorDeskMainDockSpace.uipro");
    const bool leftCollapsed = iconBar;
    const ImVec2 dockSize = ImGui::GetContentRegionAvail();
    const bool emptyChanged = empty != m_lastEmpty;
    m_lastEmpty = empty;
    const bool rebuildLeft = state.layoutRebuildRequested || m_leftFoldDirty || emptyChanged;
    ApplyDockLayout(dockspaceId, dockSize, modeId, rebuildLeft, leftCollapsed, empty, iconBar);
    m_leftFoldDirty = false;
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_AutoHideTabBar);
    ApplyAutoHideTabBar(ImGui::DockBuilderGetNode(dockspaceId));
    if (rebuildLeft && !empty && !iconBar) {
        ApplyLeftColumnWidth(dockspaceId, dockSize.x, leftCollapsed);
    }
    ImGui::End();

    if (ModeIs(state, "set") || ModeIs(state, "shoot")) {
        ImGui::Begin("视口###Viewport", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (ImGuiWindow* window = ImGui::GetCurrentWindow()) {
            window->Scroll = ImVec2(0.0f, 0.0f);
        }
        if (empty) {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y * 0.22f));
            const float width = std::min(420.0f, ImGui::GetContentRegionAvail().x - 24.0f);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);
            ImGui::BeginGroup();
            ImGui::PushItemWidth(width);
            PushUiFont(kUiDisplay);
            ImGui::TextColored(kAccent, "开始一块新的分镜");
            PopUiFont();
            ImGui::TextDisabled("建档 → 编剧 → 置景 → 掌机 → 审片 → 交付");
            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            if (state.exampleProjectPath != nullptr && state.exampleProjectPath[0] != '\0') {
                if (ImGui::Button("打开示例工程", ImVec2(width, 0.0f))) {
                    commands.Push(Core::OpenProjectFromPathCommand{state.exampleProjectPath});
                }
            }
            if (ImGui::Button("新建工程", ImVec2(width, 0.0f))) {
                commands.Push(Core::NewProjectCommand{});
            }
            if (ImGui::Button("打开工程...", ImVec2(width, 0.0f))) {
                commands.Push(Core::OpenProjectCommand{});
            }
            ImGui::PopItemWidth();
            ImGui::EndGroup();
        } else {
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const ImVec2 frame =
                m_lockExportAspect ? FitExportFrame(available, state.exportResolutionId) : available;
            const std::uint32_t width =
                frame.x > 1.0f ? static_cast<std::uint32_t>(frame.x) : 1;
            const std::uint32_t height =
                frame.y > 1.0f ? static_cast<std::uint32_t>(frame.y) : 1;
            const int dw = static_cast<int>(width) - static_cast<int>(m_lastViewportW);
            const int dh = static_cast<int>(height) - static_cast<int>(m_lastViewportH);
            if (m_lastViewportW == 0 || m_lastViewportH == 0 || dw * dw + dh * dh >= 4) {
                m_lastViewportW = width;
                m_lastViewportH = height;
                commands.Push(Core::ViewportResizeCommand{m_lastViewportW, m_lastViewportH});
            }
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            const ImVec2 frameMin(cursor.x + (available.x - frame.x) * 0.5f,
                                  cursor.y + (available.y - frame.y) * 0.5f);
            const ImVec2 frameMax(frameMin.x + frame.x, frameMin.y + frame.y);
            ImGui::InvisibleButton("viewport_input", available);
            const bool viewportHovered = ImGui::IsItemHovered();
            const bool viewportActive = ImGui::IsItemActive();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(cursor, ImVec2(cursor.x + available.x, cursor.y + available.y),
                                IM_COL32(0x14, 0x14, 0x14, 230));
            if (state.viewportTextureIndex != 0xFFFFu) {
                draw->AddImage(ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)),
                               frameMin, frameMax);
                draw->AddRect(frameMin, frameMax, ImGui::GetColorU32(ImGuiCol_Border));
                DrawViewportGuides(frameMin, frameMax, m_showThirds, m_showSafeFrame);
            }
            const ScriptShotView* hudShot = FindSelectedShot(state, nullptr);
            char hud[256];
            if (hudShot != nullptr) {
                std::snprintf(hud, sizeof(hud), "%s · %s", hudShot->title.c_str(),
                              hudShot->linkedCameraName.empty() ? "无机位"
                                                                : hudShot->linkedCameraName.c_str());
            } else {
                std::snprintf(hud, sizeof(hud), "%s", "未选镜头");
            }
            PushUiFont(kUiCaption);
            const ImVec2 hudSize = ImGui::CalcTextSize(hud);
            const ImVec2 hudPos(frameMin.x + 10.0f,
                                frameMax.y - hudSize.y - 8.0f);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), hudPos,
                          ImGui::GetColorU32(ImGuiCol_Text), hud);
            PopUiFont();
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DD_ASSET_ID")) {
                    const char* assetId = static_cast<const char*>(payload->Data);
                    if (assetId != nullptr) {
                        commands.Push(Core::AddLibraryAssetToSceneCommand{assetId});
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (viewportHovered) {
                ImGuiIO& io = ImGui::GetIO();
                Core::OrbitDeltaCommand orbit;
                if (viewportActive || viewportHovered) {
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
        }
        ImGui::End();
    }

    if (!empty && !iconBar) {
        ImGui::Begin("层级###Hierarchy");
        DrawPanelCaption("层级");
        DrawHierarchyBody(state, commands, &sceneHeaderClicked);
        ImGui::End();
    } else if (!empty && iconBar && (rail.overlay == LeftRailOverlay::Shots ||
                                     rail.overlay == LeftRailOverlay::Scene)) {
        ImGui::SetNextWindowPos(rail.overlayPos);
        ImGui::SetNextWindowSize(rail.overlaySize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::Begin("层级###Hierarchy", nullptr,
                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar);
        DrawPanelCaption("层级");
        DrawHierarchyBody(state, commands, &sceneHeaderClicked);
        ImGui::End();
    }

    ImGui::Begin("检查器###Inspector");
    DrawPanelCaption("检查器");
    const char* selectionId = state.selectionId != nullptr ? state.selectionId : "";
    const std::string selectionKey = std::string(KindId(state)) + "\n" + selectionId;
    if (selectionKey != m_lastSelectionKey) {
        m_lastSelectionKey = selectionKey;
        if (!sceneHeaderClicked) {
            m_sceneFacePinned = false;
        }
    }
    if (sceneHeaderClicked) {
        m_sceneFacePinned = true;
    }
    if (ModeIs(state, "script")) {
        DrawScriptInspector(state);
    } else if (ModeIs(state, "review")) {
        DrawDeliveryInspector(state, commands);
    } else if (m_sceneFacePinned || (ModeIs(state, "set") && KindIs(state, "none"))) {
        DrawSceneInspector(state, commands);
    } else if (KindIs(state, "asset")) {
        if (const LibraryAssetView* asset = FindSelectedAsset(state)) {
            DrawAssetInspector(*asset, commands);
        } else {
            DrawOnboarding(state, commands);
        }
    } else if (KindIs(state, "shot")) {
        const ScriptSceneView* scene = nullptr;
        if (const ScriptShotView* shot = FindSelectedShot(state, &scene)) {
            DrawShotInspector(state, commands, *shot, scene);
        } else {
            DrawOnboarding(state, commands);
        }
    } else if (KindIs(state, "node")) {
        if (const NodeView* node = FindSelectedNode(state)) {
            DrawNodeInspector(state, commands, *node);
        } else {
            DrawOnboarding(state, commands);
        }
    } else if (KindIs(state, "camera")) {
        if (const CameraItemView* camera = FindSelectedCamera(state)) {
            DrawCameraInspector(state, commands, *camera, m_cameraName, m_cameraNameId);
        } else {
            DrawOnboarding(state, commands);
        }
    } else {
        DrawOnboarding(state, commands);
    }
    ImGui::End();

    if (state.exportOverwritePrompt) {
        ImGui::OpenPopup("覆盖导出文件");
    }
    PushModalColors();
    ImGui::SetNextWindowBgAlpha(1.0f);
    if (ImGui::BeginPopupModal("覆盖导出文件", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ModalBodyText("目标 PNG 已存在，是否覆盖？");
        if (ModalButton("覆盖")) {
            commands.Push(Core::ConfirmExportOverwriteCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ModalButton("取消")) {
            commands.Push(Core::CancelExportOverwriteCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    PopModalColors();

    if (state.exportStalePrompt) {
        ImGui::OpenPopup("预览未就绪");
    }
    PushModalColors();
    ImGui::SetNextWindowBgAlpha(1.0f);
    if (ImGui::BeginPopupModal("预览未就绪", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char staleText[128];
        std::snprintf(staleText, sizeof(staleText), "有 %d 个镜头预览缺失、需刷新或失败。",
                      state.exportStaleCount);
        ModalBodyText(staleText);
        ModalBodyText("继续导出将使用占位状态，是否继续？");
        if (ModalButton("继续导出")) {
            commands.Push(Core::ConfirmStoryboardStaleExportCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ModalButton("取消")) {
            commands.Push(Core::CancelStoryboardStaleExportCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    PopModalColors();

    if (state.projectPromptVisible) {
        ImGui::OpenPopup("未保存的工程");
    }
    PushModalColors();
    ImGui::SetNextWindowBgAlpha(1.0f);
    if (ImGui::BeginPopupModal("未保存的工程", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ModalBodyText("当前工程有未保存的更改。");
        if (ModalButton("保存")) {
            commands.Push(Core::ConfirmSaveProjectCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ModalButton("放弃")) {
            commands.Push(Core::DiscardProjectCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ModalButton("取消")) {
            commands.Push(Core::CancelProjectPromptCommand{});
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    PopModalColors();
}

} // namespace DirectorDesk::UI
