// UiFonts: design-size helpers for ImGui 1.92 dynamic fonts (UIC-08).
#pragma once

#include <imgui.h>

namespace DirectorDesk::UI {

constexpr float kUiCaption = 12.0f;
constexpr float kUiBody = 14.0f;
constexpr float kUiTitle = 16.0f;
constexpr float kUiDisplay = 20.0f;
constexpr float kUiEditor = 16.0f;

inline float UiDpiScale() {
    const float base = ImGui::GetStyle().FontSizeBase;
    return base > 0.05f ? base / kUiBody : 1.0f;
}

inline float UiScale() {
    return UiDpiScale();
}

inline float UiPx(float designPx) {
    return designPx * UiScale();
}

inline float ShotStripThumbW(unsigned windowWidth) {
    return UiPx(windowWidth >= 1920 ? 208.0f : 176.0f);
}

inline float ShotStripThumbH(unsigned windowWidth) {
    return UiPx(windowWidth >= 1920 ? 117.0f : 99.0f);
}

inline float ShotStripBarH(unsigned windowWidth) {
    return UiPx(windowWidth >= 1920 ? 150.0f : 132.0f);
}

inline void PushUiFont(float designPx) {
    ImGui::PushFont(nullptr, designPx * UiDpiScale());
}

inline void PopUiFont() {
    ImGui::PopFont();
}

} // namespace DirectorDesk::UI
