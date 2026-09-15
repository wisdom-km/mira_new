// ScriptPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/ScriptPanel.h"

#include "DirectorDesk/Core/Command.h"
#include "UiChrome.h"
#include "UiFonts.h"
#include "UiIcons.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kMuted(0.604f, 0.604f, 0.635f, 1.0f);
constexpr ImU32 kLineHighlight = IM_COL32(0xD8, 0x9A, 0x4A, 15);

int ScriptResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* text = static_cast<std::string*>(data->UserData);
        text->resize(static_cast<std::size_t>(data->BufTextLen));
        data->Buf = text->data();
    }
    return 0;
}

int CountScriptLines(const std::string& text) {
    int lines = 1;
    for (char ch : text) {
        if (ch == '\n') {
            ++lines;
        }
    }
    return lines;
}

bool DrawScriptEditor(std::string& text, const ImVec2& size, int highlightStart, int highlightEnd) {
    if (text.capacity() < text.size() + 16) {
        text.reserve(text.size() + 256);
    }
    PushUiFont(kUiEditor);
    const float lineH = ImGui::GetTextLineHeight();
    const int lineCount = std::max(1, CountScriptLines(text));
    char gutterSample[8];
    std::snprintf(gutterSample, sizeof(gutterSample), "%d", std::max(lineCount, 999));
    const float gutterW = ImGui::CalcTextSize(gutterSample).x + 10.0f;
    const float contentH = std::max(size.y, lineH * static_cast<float>(lineCount) + 8.0f);
    ImGui::BeginChild("##script-scroll", size, ImGuiChildFlags_None, ImGuiWindowFlags_None);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    if (highlightStart > 0 && highlightEnd >= highlightStart) {
        const float y0 = origin.y + lineH * static_cast<float>(highlightStart - 1);
        const float y1 = origin.y + lineH * static_cast<float>(highlightEnd);
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(origin.x + gutterW, y0),
                                                  ImVec2(origin.x + ImGui::GetWindowWidth(), y1),
                                                  kLineHighlight);
    }
    ImGui::BeginGroup();
    for (int line = 1; line <= lineCount; ++line) {
        const bool hot = highlightStart > 0 && line >= highlightStart && line <= highlightEnd;
        if (hot) {
            ImGui::TextColored(ImVec4(0.847f, 0.604f, 0.290f, 0.85f), "%d", line);
        } else {
            ImGui::TextDisabled("%d", line);
        }
    }
    ImGui::EndGroup();
    ImGui::SameLine(0.0f, 8.0f);
    const bool changed = ImGui::InputTextMultiline(
        "##script-editor", text.data(), text.capacity() + 1,
        ImVec2(std::max(8.0f, ImGui::GetContentRegionAvail().x), contentH),
        ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_NoHorizontalScroll,
        ScriptResizeCallback, &text);
    ImGui::EndChild();
    PopUiFont();
    return changed;
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
        state.scriptPath != nullptr && state.scriptPath[0] != '\0' ? state.scriptPath : "";
    const char* shown = "(未保存)";
    if (path[0] != '\0') {
        shown = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '/' || *p == '\\') {
                shown = p + 1;
            }
        }
        if (shown[0] == '\0') {
            shown = path;
        }
    }
    ImGui::SameLine();
    ImGui::TextColored(kMuted, "%s", shown);
    if (path[0] != '\0' && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", path);
    }
    if (state.scriptDirty) {
        ImGui::SameLine();
        ImGui::TextUnformatted("*");
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 8.0f) {
        avail.x = 8.0f;
    }
    if (avail.y < 8.0f) {
        ImGui::End();
        return;
    }
    const bool hasScript = state.scriptText != nullptr && state.scriptText[0] != '\0';
    const bool hasPath = state.scriptPath != nullptr && state.scriptPath[0] != '\0';
    if (!hasScript && !hasPath && !state.scriptHasSnapshot) {
        ImGui::Dummy(ImVec2(0.0f, avail.y * 0.28f));
        ImGui::TextUnformatted("还没有剧本。打开一份 Markdown，或从示例开始。");
        if (ImGui::Button("打开剧本", ImVec2(UiPx(160.0f), 0.0f))) {
            commands.Push(Core::LoadScriptCommand{});
        }
        if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
            ImGui::SameLine();
            if (ImGui::Button("打开示例", ImVec2(UiPx(160.0f), 0.0f))) {
                commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
            }
        }
        ImGui::End();
        return;
    }
    const float maxW = UiPx(900.0f);
    const float editorW = std::min(avail.x, maxW);
    const float pad = std::max(0.0f, (avail.x - editorW) * 0.5f);
    if (pad > 1.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
    }
    ImVec2 editorSize(editorW, avail.y);
    if (DrawScriptEditor(m_editorText, editorSize, state.scriptSelectedLineStart,
                         state.scriptSelectedLineEnd)) {
        commands.Push(Core::SetScriptTextCommand{m_editorText});
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
