// Layout: Public or internal interface for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Storyboard/Types.h"

#include <vector>

namespace DirectorDesk::Storyboard {

struct LayoutMetrics {
    float pad = 32.0f;
    float columnGap = 48.0f;
    float rowGap = 16.0f;
    float rootW = 180.0f;
    float rootH = 64.0f;
    float sceneW = 240.0f;
    float sceneH = 80.0f;
    float shotW = 220.0f;
    float shotH = 168.0f;
    float minZoom = 0.35f;
    float maxZoom = 2.0f;
};

[[nodiscard]] const LayoutMetrics& DefaultLayoutMetrics();
[[nodiscard]] LayoutResult BuildLayout(const StoryboardSourceSnapshot& snapshot,
                                       const LayoutMetrics& metrics = DefaultLayoutMetrics(),
                                       bool expandAll = false);
[[nodiscard]] bool CardsOverlap(const LayoutResult& layout);
[[nodiscard]] std::vector<std::string> VisibleCardIds(const LayoutResult& layout,
                                                      const ViewRect& view);
[[nodiscard]] const LayoutCard* FindCard(const LayoutResult& layout, const std::string& id);
void FitView(const LayoutResult& layout, float viewportW, float viewportH, float& panX, float& panY,
             float& zoom, const LayoutMetrics& metrics = DefaultLayoutMetrics());
void FocusCard(const LayoutResult& layout, const std::string& id, float viewportW, float viewportH,
               float& panX, float& panY, float zoom);

} // namespace DirectorDesk::Storyboard
