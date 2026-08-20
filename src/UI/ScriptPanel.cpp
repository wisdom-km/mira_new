// ScriptPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/ScriptPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kMuted(0.56f, 0.61f, 0.68f, 1.0f);

int ScriptResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* text = static_cast<std::string*>(data->UserData);
        text->resize(static_cast<std::size_t>(data->BufTextLen));
        data->Buf = text->data();
    }
    return 0;
}

} // namespace

void ScriptPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    const char* mode = state.workspaceModeId != nullptr && state.workspaceModeId[0] != '\0'
                           ? state.workspaceModeId
                           : "shoot";
    if (std::strcmp(mode, "script") != 0) {
        return;
    }

    if (state.scriptExternalRevision != m_seenRevision) {
        m_editorText = state.scriptText != nullptr ? state.scriptText : "";
        m_seenRevision = state.scriptExternalRevision;
    }

    ImGui::Begin("剧本###Script");
    if (ImGui::SmallButton("打开...")) {
        commands.Push(Core::LoadScriptCommand{});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("保存")) {
        commands.Push(Core::SaveScriptCommand{});
    }
    const char* path =
        state.scriptPath != nullptr && state.scriptPath[0] != '\0' ? state.scriptPath : "(未保存)";
    ImGui::SameLine();
    ImGui::TextColored(kMuted, "%s", path);
    if (state.scriptDirty) {
        ImGui::SameLine();
        ImGui::TextUnformatted("*");
    }

    if (m_editorText.capacity() < m_editorText.size() + 16) {
        m_editorText.reserve(m_editorText.size() + 256);
    }
    const ImVec2 editorSize = ImVec2(-1.0f, ImGui::GetContentRegionAvail().y);
    if (ImGui::InputTextMultiline(
            "##script-editor", m_editorText.data(), m_editorText.capacity() + 1, editorSize,
            ImGuiInputTextFlags_CallbackResize, ScriptResizeCallback, &m_editorText)) {
        commands.Push(Core::SetScriptTextCommand{m_editorText});
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
