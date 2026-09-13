// ScriptPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/ScriptPanel.h"

#include "DirectorDesk/Core/Command.h"
#include "UiChrome.h"
#include "UiFonts.h"
#include "UiIcons.h"

#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kMuted(0.604f, 0.604f, 0.635f, 1.0f);

int ScriptResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* text = static_cast<std::string*>(data->UserData);
        text->resize(static_cast<std::size_t>(data->BufTextLen));
        data->Buf = text->data();
    }
    return 0;
}

bool DrawScriptEditor(std::string& text, const ImVec2& size) {
    if (text.capacity() < text.size() + 16) {
        text.reserve(text.size() + 256);
    }
    return ImGui::InputTextMultiline("##script-editor", text.data(), text.capacity() + 1, size,
                                     ImGuiInputTextFlags_CallbackResize, ScriptResizeCallback,
                                     &text);
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
    DrawPanelCaption("剧本");
    char openScript[32];
    char saveScript[32];
    std::snprintf(openScript, sizeof(openScript), "%s 打开...", Icon::FileInput);
    std::snprintf(saveScript, sizeof(saveScript), "%s 保存", Icon::Save);
    if (ImGui::SmallButton(openScript)) {
        commands.Push(Core::LoadScriptCommand{});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(saveScript)) {
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

    ImVec2 editorSize = ImGui::GetContentRegionAvail();
    if (editorSize.x < 8.0f) {
        editorSize.x = 8.0f;
    }
    if (editorSize.y < 8.0f) {
        ImGui::End();
        return;
    }
    PushUiFont(kUiEditor);
    if (DrawScriptEditor(m_editorText, editorSize)) {
        commands.Push(Core::SetScriptTextCommand{m_editorText});
    }
    PopUiFont();
    ImGui::End();
}

} // namespace DirectorDesk::UI
