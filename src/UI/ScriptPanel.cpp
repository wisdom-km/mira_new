// ScriptPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/ScriptPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

int ScriptResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* text = static_cast<std::string*>(data->UserData);
        text->resize(static_cast<std::size_t>(data->BufTextLen));
        data->Buf = text->data();
    }
    return 0;
}

const char* SeverityLabel(const char* severity) {
    return severity == nullptr || severity[0] == '\0' ? "提示" : severity;
}

} // namespace

void ScriptPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    if (state.scriptExternalRevision != m_seenRevision) {
        m_editorText = state.scriptText != nullptr ? state.scriptText : "";
        m_seenRevision = state.scriptExternalRevision;
    }

    ImGui::Begin("剧本###Script");
    if (ImGui::Button("打开...")) {
        commands.Push(Core::LoadScriptCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("保存")) {
        commands.Push(Core::SaveScriptCommand{});
    }
    if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
        ImGui::SameLine();
        if (ImGui::Button("打开示例剧本")) {
            commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
        }
    }
    if (ImGui::Button("添加场次")) {
        commands.Push(Core::InsertSceneCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("添加镜头")) {
        commands.Push(Core::InsertShotCommand{});
    }

    const char* path = state.scriptPath != nullptr && state.scriptPath[0] != '\0'
                           ? state.scriptPath
                           : "(未保存)";
    ImGui::TextUnformatted(path);
    if (state.scriptDirty) {
        ImGui::SameLine();
        ImGui::TextUnformatted("*");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("镜头列表");
    if (state.scriptScenes != nullptr && state.scriptScenes->empty()) {
        ImGui::TextUnformatted("还没有场次。打开示例剧本，或点击添加场次。");
    }
    if (state.scriptScenes != nullptr) {
        for (const ScriptSceneView& scene : *state.scriptScenes) {
            if (ImGui::TreeNodeEx(scene.id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s",
                                  scene.title.c_str())) {
                for (const ScriptShotView& shot : scene.shots) {
                    const std::string label = shot.title + "##" + shot.id;
                    if (ImGui::Selectable(label.c_str(), shot.selected)) {
                        commands.Push(Core::SelectShotCommand{shot.id});
                    }
                    if (shot.selected && !shot.linkedCameraName.empty()) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("%s%s", shot.linkedCameraName.c_str(),
                                            shot.linkedMissing ? " (缺失)" : "");
                    }
                }
                ImGui::TreePop();
            }
        }
    }
    if (!state.scriptHasSnapshot) {
        ImGui::TextUnformatted("尚无已发布的剧本结构快照");
    }

    ImGui::Separator();
    if (m_editorText.capacity() < m_editorText.size() + 16) {
        m_editorText.reserve(m_editorText.size() + 256);
    }
    const ImVec2 editorSize = ImVec2(-1.0f, 220.0f);
    if (ImGui::InputTextMultiline("##script-editor", m_editorText.data(),
                                  m_editorText.capacity() + 1, editorSize,
                                  ImGuiInputTextFlags_CallbackResize, ScriptResizeCallback,
                                  &m_editorText)) {
        commands.Push(Core::SetScriptTextCommand{m_editorText});
    }

    ImGui::Separator();
    ImGui::TextUnformatted("诊断");
    if (state.scriptDiagnostics != nullptr) {
        for (const ScriptDiagnosticView& diagnostic : *state.scriptDiagnostics) {
            ImGui::Text("%s L%d [%s] %s", SeverityLabel(diagnostic.severity), diagnostic.line,
                        diagnostic.code != nullptr ? diagnostic.code : "",
                        diagnostic.message != nullptr ? diagnostic.message : "");
        }
        if (state.scriptDiagnostics->empty()) {
            ImGui::TextUnformatted("无");
        }
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
