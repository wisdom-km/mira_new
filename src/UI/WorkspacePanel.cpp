// WorkspacePanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/WorkspacePanel.h"

#include "DirectorDesk/Core/Command.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kAccent(0.85f, 0.60f, 0.29f, 1.0f);
constexpr ImVec4 kLive(0.31f, 0.82f, 0.58f, 1.0f);
constexpr ImVec4 kMuted(0.56f, 0.61f, 0.68f, 1.0f);
constexpr ImVec4 kText(0.914f, 0.929f, 0.957f, 1.0f);
constexpr ImU32 kModalTextU32 = IM_COL32(0xE9, 0xED, 0xF4, 0xFF);

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
    auto dot = [](bool on) {
        ImGui::SameLine();
        ImGui::TextColored(on ? kLive : kMuted, "%s", on ? "●" : "○");
    };
    const bool linked =
        card != nullptr && card->link != nullptr && std::strcmp(card->link, "已关联") == 0;
    const bool preview =
        card != nullptr && card->preview != nullptr && std::strcmp(card->preview, "就绪") == 0;
    const bool exported =
        card != nullptr && card->exported != nullptr && std::strcmp(card->exported, "已导出") == 0;
    dot(linked);
    dot(preview);
    dot(exported);
}

void PushModalColors() {
    ImGui::PushStyleColor(ImGuiCol_Text, kText);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, kText);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.063f, 0.075f, 0.098f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.082f, 0.094f, 0.118f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.043f, 0.051f, 0.071f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.043f, 0.051f, 0.071f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.169f, 0.200f, 0.251f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.275f, 0.322f, 0.396f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.192f, 0.329f, 0.490f, 1.0f));
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
    draw->AddRect(pos, max, IM_COL32(0x46, 0x52, 0x65, 0xFF), style.FrameRounding);
    draw->AddText(ImVec2(pos.x + style.FramePadding.x, pos.y + style.FramePadding.y), kModalTextU32,
                  label);
    ImGui::PopID();
    return pressed;
}

void ApplyDockLayout(ImGuiID dockspaceId, const ImVec2& size, const char* modeId, bool force) {
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
    ImGuiID dockBottom = 0;
    if (!review) {
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenter);
    }
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.24f, &dockRight, &dockCenter);
    const float bottomRatio = set ? 0.12f : (script ? 0.16f : 0.22f);
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, bottomRatio, &dockBottom, &dockCenter);

    if (dockLeft != 0) {
        if (script) {
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
    ImGui::DockBuilderDockWindow("ShotStrip", dockBottom);
    if (script) {
        ImGui::DockBuilderDockWindow("Script", dockCenter);
    } else if (review) {
        ImGui::DockBuilderDockWindow("Storyboard", dockCenter);
    } else {
        ImGui::DockBuilderDockWindow("Viewport", dockCenter);
    }
    ImGui::DockBuilderFinish(dockspaceId);
}

void DrawModeButton(const AppViewState& state, Core::CommandQueue& commands, const char* id,
                    const char* label, bool enabled) {
    const bool current = ModeIs(state, id);
    if (!enabled) {
        ImGui::BeginDisabled();
    }
    if (current) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }
    if (ImGui::SmallButton(label) && !current && enabled) {
        commands.Push(Core::SetWorkspaceModeCommand{id});
    }
    if (current) {
        ImGui::PopStyleColor();
    }
    if (!enabled) {
        ImGui::EndDisabled();
    }
}

void DrawOnboarding(const AppViewState& state, Core::CommandQueue& commands) {
    ImGui::TextUnformatted("无选择");
    ImGui::TextDisabled("上手三步");
    ImGui::Separator();
    const bool step1 = state.scriptHasSnapshot;
    const bool step2 = state.nodes != nullptr && !state.nodes->empty();
    const bool step3 =
        state.selectedShotLinkedCamera != nullptr && state.selectedShotLinkedCamera[0] != '\0';
    ImGui::Text("%s 打开剧本", step1 ? "[x]" : "[ ]");
    ImGui::SameLine();
    if (ImGui::SmallButton("打开剧本...")) {
        commands.Push(Core::LoadScriptCommand{});
    }
    if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::SmallButton("示例剧本")) {
            commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
        }
    }
    ImGui::Text("%s 放一个场景对象", step2 ? "[x]" : "[ ]");
    ImGui::SameLine();
    if (ImGui::SmallButton("导入模型...")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::Text("%s 给镜头绑机位", step3 ? "[x]" : "[ ]");
    ImGui::SameLine();
    if (ImGui::SmallButton("为该镜头建机位")) {
        commands.Push(Core::BindShotToNewCameraCommand{});
    }
    ImGui::Separator();
    ImGui::TextDisabled("Ctrl+N 新建  Ctrl+O 打开  Ctrl+S 保存");
    ImGui::TextDisabled("Ctrl+I 导入  Ctrl+E 导出  视口：左键旋转");
}

void DrawAssetInspector(const LibraryAssetView& asset, Core::CommandQueue& commands) {
    ImGui::SeparatorText("资产");
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
    if (asset.canAddToScene && ImGui::Button("加入场景")) {
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
        ImGui::TextColored(kLive, "● %s%s", shot.linkedCameraName.c_str(),
                           shot.linkedMissing ? " (缺失)" : "");
    } else {
        ImGui::TextDisabled("未关联");
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
    if (ImGui::Button("为该镜头建机位")) {
        commands.Push(Core::BindShotToNewCameraCommand{shot.id});
    }
    if (!shot.linkedCameraName.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("取消关联")) {
            commands.Push(Core::UnlinkShotCommand{shot.id});
        }
    }
    ImGui::TextDisabled("机位预设");
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float presetWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;
    if (ImGui::Button("正视", ImVec2(presetWidth, 0.0f))) {
        commands.Push(Core::ApplyCameraPresetCommand{"front"});
    }
    ImGui::SameLine();
    if (ImGui::Button("侧面", ImVec2(presetWidth, 0.0f))) {
        commands.Push(Core::ApplyCameraPresetCommand{"side"});
    }
    ImGui::SameLine();
    if (ImGui::Button("过肩", ImVec2(presetWidth, 0.0f))) {
        commands.Push(Core::ApplyCameraPresetCommand{"over-shoulder"});
    }
    if (ImGui::Button("俯视", ImVec2(presetWidth, 0.0f))) {
        commands.Push(Core::ApplyCameraPresetCommand{"top"});
    }
    ImGui::SameLine();
    if (ImGui::Button("特写", ImVec2(presetWidth, 0.0f))) {
        commands.Push(Core::ApplyCameraPresetCommand{"close-up"});
    }
    ImGui::SeparatorText("画面");
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
    const StoryboardCardView* card = FindShotCard(state, shot.id);
    ImGui::Text("缩略图: %s", card != nullptr && card->preview != nullptr ? card->preview : "—");
    ImGui::SameLine();
    if (ImGui::SmallButton("重渲")) {
        commands.Push(Core::RefreshStoryboardThumbnailCommand{shot.id});
    }
    ImGui::SeparatorText("交付");
    ImGui::Text("导出: %s",
                card != nullptr && card->exported != nullptr ? card->exported : "未导出");
    bool exportTransparent = state.exportTransparent;
    if (ImGui::Checkbox("透明导出", &exportTransparent)) {
        commands.Push(Core::SetExportTransparentCommand{exportTransparent});
    }
    const bool res1080 =
        state.exportResolutionId != nullptr && std::strcmp(state.exportResolutionId, "2k") != 0;
    if (ImGui::RadioButton("1080p", res1080)) {
        commands.Push(Core::SelectExportResolutionCommand{"1080p"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("2K", !res1080)) {
        commands.Push(Core::SelectExportResolutionCommand{"2k"});
    }
    if (ImGui::Button("导出当前镜头")) {
        commands.Push(Core::ExportCurrentShotCommand{
            state.exportResolutionId != nullptr ? state.exportResolutionId : "1080p"});
    }
    ImGui::PopID();
}

void DrawNodeInspector(Core::CommandQueue& commands, const NodeView& node) {
    ImGui::PushID(node.id.c_str());
    ImGui::TextUnformatted(node.name.c_str());
    ImGui::TextDisabled("场景对象为所有镜头共享");
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
        const float monitorH = std::min(avail.y * 0.26f, 160.0f);
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(avail.x, monitorH));
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddImage(ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)), cursor,
                       ImVec2(cursor.x + avail.x, cursor.y + monitorH));
        draw->AddRect(cursor, ImVec2(cursor.x + avail.x, cursor.y + monitorH),
                      ImGui::GetColorU32(ImGuiCol_Border));
    }
    ImGui::SeparatorText("交付");
    bool exportTransparent = state.exportTransparent;
    if (ImGui::Checkbox("透明导出", &exportTransparent)) {
        commands.Push(Core::SetExportTransparentCommand{exportTransparent});
    }
    const bool res1080 =
        state.exportResolutionId != nullptr && std::strcmp(state.exportResolutionId, "2k") != 0;
    if (ImGui::RadioButton("1080p", res1080)) {
        commands.Push(Core::SelectExportResolutionCommand{"1080p"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("2K", !res1080)) {
        commands.Push(Core::SelectExportResolutionCommand{"2k"});
    }
    if (state.exportIssues != nullptr && !state.exportIssues->empty()) {
        ImGui::SeparatorText("未就绪");
        for (const ExportIssueView& issue : *state.exportIssues) {
            ImGui::PushID(issue.shotId.c_str());
            ImGui::TextWrapped("%s · %s", issue.shotTitle.c_str(),
                               issue.reason != nullptr ? issue.reason : "");
            if (issue.reason != nullptr && std::strcmp(issue.reason, "未关联相机") == 0) {
                if (ImGui::SmallButton("建机位")) {
                    commands.Push(Core::BindShotToNewCameraCommand{issue.shotId});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("关联当前")) {
                    commands.Push(Core::LinkShotToCameraCommand{issue.shotId, ""});
                }
            } else if (ImGui::SmallButton("重渲缩略图")) {
                commands.Push(Core::RefreshStoryboardThumbnailCommand{issue.shotId});
            }
            ImGui::PopID();
        }
    }
    if (ImGui::Button("导出当前镜头")) {
        commands.Push(Core::ExportCurrentShotCommand{
            state.exportResolutionId != nullptr ? state.exportResolutionId : "1080p"});
    }
    if (ImGui::Button("导出分镜总览")) {
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

} // namespace

void WorkspacePanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool empty = ProjectIsEmpty(state);
    const char* modeId = ModeId(state);

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
                ImGui::Separator();
                if (ImGui::MenuItem("为该镜头建机位")) {
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
                ImGui::TextUnformatted("Ctrl+E  导出当前镜头 1080p");
                ImGui::Separator();
                ImGui::TextUnformatted("视口：左键旋转  右键平移  滚轮缩放");
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
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    const ImGuiWindowFlags sideFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration;
    if (ImGui::BeginViewportSideBar("##ToolStrip", viewport, ImGuiDir_Up, 34.0f, sideFlags)) {
        ImGui::AlignTextToFramePadding();
        DrawModeButton(state, commands, "script", "编剧", !empty);
        ImGui::SameLine();
        DrawModeButton(state, commands, "set", "置景", !empty);
        ImGui::SameLine();
        DrawModeButton(state, commands, "shoot", "掌机", true);
        ImGui::SameLine();
        DrawModeButton(state, commands, "review", "审片", !empty);
        ImGui::SameLine(0.0f, 16.0f);
        const ScriptSceneView* selectedScene = nullptr;
        const ScriptShotView* selectedShot = FindSelectedShot(state, &selectedScene);
        if (selectedShot != nullptr) {
            ImGui::TextColored(kAccent, "%s", selectedShot->title.c_str());
        } else {
            ImGui::TextDisabled("未选镜头");
        }
        int shotCount = 0;
        int readyCount = 0;
        CountShots(state, shotCount, readyCount);
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextDisabled("%d 镜 / %d 已成镜", shotCount, readyCount);
        float right = ImGui::GetWindowWidth() - 108.0f;
        if (right > ImGui::GetCursorPosX()) {
            ImGui::SameLine(right);
        } else {
            ImGui::SameLine();
        }
        if (ModeIs(state, "script")) {
            if (ImGui::SmallButton("保存剧本")) {
                commands.Push(Core::SaveScriptCommand{});
            }
        } else if (ModeIs(state, "set")) {
            if (ImGui::SmallButton("导入模型")) {
                commands.Push(Core::ImportModelCommand{});
            }
        } else if (ModeIs(state, "review")) {
            if (ImGui::SmallButton("导出总览")) {
                commands.Push(Core::ExportStoryboardBoardCommand{});
            }
        } else if (ImGui::SmallButton("导出镜头")) {
            commands.Push(Core::ExportCurrentShotCommand{
                state.exportResolutionId != nullptr ? state.exportResolutionId : "1080p"});
        }
        ImGui::End();
    }

    if (ImGui::BeginViewportSideBar("##StatusBar", viewport, ImGuiDir_Down, 22.0f, sideFlags)) {
        ImGui::AlignTextToFramePadding();
        const char* status =
            state.statusText != nullptr && state.statusText[0] != '\0' ? state.statusText : "就绪";
        ImGui::TextUnformatted(status);
        if (state.importInProgress) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kLive, "导入中");
        }
        if (state.officialCatalogStatus != nullptr && state.officialCatalogStatus[0] != '\0') {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextDisabled("%s", state.officialCatalogStatus);
        }
        if (ModeIs(state, "review") && state.exportIssues != nullptr &&
            !state.exportIssues->empty()) {
            ImGui::SameLine(0.0f, 16.0f);
            ImGui::TextColored(kAccent, "%d 项未就绪",
                               static_cast<int>(state.exportIssues->size()));
        }
        const char* path = state.projectPath != nullptr && state.projectPath[0] != '\0'
                               ? state.projectPath
                               : "尚未保存";
        char rightText[256];
        std::snprintf(rightText, sizeof(rightText), "%s   VIEW %u×%u", path,
                      state.viewportTextureWidth, state.viewportTextureHeight);
        const float rightWidth = ImGui::CalcTextSize(rightText).x + 12.0f;
        const float rightX = ImGui::GetWindowWidth() - rightWidth;
        if (rightX > ImGui::GetCursorPosX()) {
            ImGui::SameLine(rightX);
            ImGui::TextDisabled("%s", rightText);
        }
        ImGui::End();
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
            commands.Push(Core::ExportCurrentShotCommand{"1080p"});
        }
    }

    const ImGuiID dockspaceId = ImGui::GetID("DirectorDeskMainDockSpace.uipro");
    ApplyDockLayout(dockspaceId, ImGui::GetContentRegionAvail(), modeId,
                    state.layoutRebuildRequested);
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
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
            ImGui::TextColored(kAccent, "开始一块新的分镜");
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
            ImGui::BeginChild("ViewportHud", ImVec2(0.0f, 28.0f), ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(kLive, "● LIVE");
            ImGui::SameLine();
            const ScriptSceneView* hudScene = nullptr;
            if (const ScriptShotView* shot = FindSelectedShot(state, &hudScene)) {
                ImGui::TextUnformatted(shot->title.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("·");
                ImGui::SameLine();
                ImGui::TextDisabled("%s", shot->linkedCameraName.empty()
                                              ? "未关联"
                                              : shot->linkedCameraName.c_str());
            } else {
                ImGui::TextDisabled("未选镜头");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%u × %u", state.viewportTextureWidth, state.viewportTextureHeight);
            ImGui::EndChild();

            const ImVec2 available = ImGui::GetContentRegionAvail();
            const std::uint32_t width =
                available.x > 1.0f ? static_cast<std::uint32_t>(available.x) : 1;
            const std::uint32_t height =
                available.y > 1.0f ? static_cast<std::uint32_t>(available.y) : 1;
            const int dw = static_cast<int>(width) - static_cast<int>(m_lastViewportW);
            const int dh = static_cast<int>(height) - static_cast<int>(m_lastViewportH);
            if (m_lastViewportW == 0 || m_lastViewportH == 0 || dw * dw + dh * dh >= 4) {
                m_lastViewportW = width;
                m_lastViewportH = height;
            }
            commands.Push(Core::ViewportResizeCommand{m_lastViewportW, m_lastViewportH});
            const ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("viewport_input", available);
            const bool viewportHovered = ImGui::IsItemHovered();
            const bool viewportActive = ImGui::IsItemActive();
            if (state.viewportTextureIndex != 0xFFFFu) {
                ImDrawList* draw = ImGui::GetWindowDrawList();
                draw->AddImage(ImTextureRef(static_cast<ImTextureID>(state.viewportTextureIndex)),
                               cursor, ImVec2(cursor.x + available.x, cursor.y + available.y));
                draw->AddRect(cursor, ImVec2(cursor.x + available.x, cursor.y + available.y),
                              ImGui::GetColorU32(ImGuiCol_Border));
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

    if (!ModeIs(state, "review")) {
        ImGui::Begin("层级###Hierarchy");
        if (!ModeIs(state, "set")) {
            if (ImGui::CollapsingHeader("镜头表", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SmallButton("+ 场次")) {
                    commands.Push(Core::InsertSceneCommand{});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("+ 镜头")) {
                    commands.Push(Core::InsertShotCommand{});
                }
                if (state.scriptScenes == nullptr || state.scriptScenes->empty()) {
                    ImGui::TextDisabled("打开剧本后生成镜头表");
                } else {
                    for (const ScriptSceneView& scene : *state.scriptScenes) {
                        if (ImGui::TreeNodeEx(scene.id.c_str(), ImGuiTreeNodeFlags_DefaultOpen,
                                              "%s", scene.title.c_str())) {
                            for (const ScriptShotView& shot : scene.shots) {
                                const std::string label = shot.title + "##" + shot.id;
                                if (ImGui::Selectable(label.c_str(), shot.selected)) {
                                    commands.Push(Core::SelectShotCommand{shot.id});
                                }
                                if (ImGui::BeginPopupContextItem()) {
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
        }
        if (!ModeIs(state, "script")) {
            if (ImGui::CollapsingHeader("场景对象", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (state.nodes == nullptr || state.nodes->empty()) {
                    ImGui::TextDisabled("从资源库拖入模型，或导入 OBJ/GLB。");
                } else {
                    for (const NodeView& node : *state.nodes) {
                        const std::string nodeLabel =
                            std::string("◆  ") + node.name + "##" + node.id;
                        if (ImGui::Selectable(nodeLabel.c_str(), node.selected)) {
                            commands.Push(Core::SelectNodeCommand{node.id});
                        }
                    }
                }
            }
            if (ImGui::CollapsingHeader("相机", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SmallButton("+ 添加相机")) {
                    commands.Push(Core::AddCameraCommand{});
                }
                if (state.cameras != nullptr) {
                    for (const CameraItemView& camera : *state.cameras) {
                        const std::string label =
                            std::string("▣  ") + camera.name + "##" + camera.id;
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
            }
        }
        ImGui::End();
    }

    ImGui::Begin("检查器###Inspector");
    if (ModeIs(state, "script")) {
        DrawScriptInspector(state);
    } else if (ModeIs(state, "review")) {
        DrawDeliveryInspector(state, commands);
    } else if (KindIs(state, "shot")) {
        const ScriptSceneView* scene = nullptr;
        if (const ScriptShotView* shot = FindSelectedShot(state, &scene)) {
            DrawShotInspector(state, commands, *shot, scene);
        } else {
            DrawOnboarding(state, commands);
        }
    } else if (KindIs(state, "node")) {
        if (const NodeView* node = FindSelectedNode(state)) {
            DrawNodeInspector(commands, *node);
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
    if (const LibraryAssetView* asset = FindSelectedAsset(state)) {
        DrawAssetInspector(*asset, commands);
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
        ImGui::OpenPopup("缩略图未就绪");
    }
    PushModalColors();
    ImGui::SetNextWindowBgAlpha(1.0f);
    if (ImGui::BeginPopupModal("缩略图未就绪", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char staleText[128];
        std::snprintf(staleText, sizeof(staleText), "有 %d 个镜头缩略图缺失、过期或失败。",
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
