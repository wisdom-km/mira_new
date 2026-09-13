// UiChrome: caption / section-header helpers (UIC-07 / UIC-12 / UIC-33).
#pragma once

#include "UiFonts.h"
#include "UiIcons.h"

#include <algorithm>
#include <cstdio>

namespace DirectorDesk::UI {

enum class LeftRailOverlay : int {
    None = 0,
    Shots,
    Scene,
    Library,
};

struct LeftRailState {
    bool iconBar = false;
    LeftRailOverlay overlay = LeftRailOverlay::None;
    ImVec2 overlayPos = ImVec2(0.0f, 0.0f);
    ImVec2 overlaySize = ImVec2(280.0f, 400.0f);
};

inline LeftRailState& CurrentLeftRail() {
    static LeftRailState state;
    return state;
}

inline void DrawPanelCaption(const char* label) {
    PushUiFont(kUiCaption);
    ImGui::TextDisabled("%s", label);
    PopUiFont();
}

inline bool BeginSection(const char* label, bool defaultOpen = true,
                         bool* headerClicked = nullptr, bool* addClicked = nullptr) {
    ImGui::PushID(label);
    const ImGuiID id = ImGui::GetID("open");
    ImGuiStorage* storage = ImGui::GetStateStorage();
    bool open = storage->GetBool(id, defaultOpen);

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float rowH = ImGui::GetFrameHeight();
    float plusW = 0.0f;
    if (addClicked != nullptr) {
        PushUiFont(kUiCaption);
        plusW = ImGui::CalcTextSize(Icon::Plus).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        PopUiFont();
        plusW = std::max(plusW, rowH);
    }
    const float toggleW = std::max(1.0f, width - plusW);

    if (ImGui::InvisibleButton("##toggle", ImVec2(toggleW, rowH))) {
        open = !open;
        storage->SetBool(id, open);
        if (headerClicked != nullptr) {
            *headerClicked = true;
        }
    }
    if (addClicked != nullptr) {
        ImGui::SameLine(0.0f, 0.0f);
        PushUiFont(kUiCaption);
        char plusLabel[16];
        std::snprintf(plusLabel, sizeof(plusLabel), "%s##add", Icon::Plus);
        if (ImGui::SmallButton(plusLabel)) {
            *addClicked = true;
        }
        PopUiFont();
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32 line = ImGui::GetColorU32(ImGuiCol_Separator);
    PushUiFont(kUiCaption);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const float textY = start.y + (rowH - textSize.y) * 0.5f;
    draw->AddText(ImVec2(start.x, textY), text, label);
    PopUiFont();
    draw->AddLine(ImVec2(start.x, start.y + rowH), ImVec2(start.x + width, start.y + rowH), line);
    ImGui::PopID();
    return open;
}

} // namespace DirectorDesk::UI
