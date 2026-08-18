#include "DirectorDesk/UI/StoryboardPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

ImVec2 WorldToScreen(float x, float y, float panX, float panY, float zoom) {
    return ImVec2(panX + x * zoom, panY + y * zoom);
}

const StoryboardCardView* HitCard(const AppViewState& state, ImVec2 local, float panX, float panY,
                                  float zoom) {
    if (state.storyboardCards == nullptr) {
        return nullptr;
    }
    const float wx = (local.x - panX) / zoom;
    const float wy = (local.y - panY) / zoom;
    for (auto it = state.storyboardCards->rbegin(); it != state.storyboardCards->rend(); ++it) {
        if (wx >= it->x && wx <= it->x + it->w && wy >= it->y && wy <= it->y + it->h) {
            return &(*it);
        }
    }
    return nullptr;
}

} // namespace

void StoryboardPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    ImGui::Begin("Storyboard");
    if (ImGui::Button("适配全部")) {
        if (state.storyboardContentWidth > 1.0f && state.storyboardContentHeight > 1.0f) {
            const ImVec2 area = ImGui::GetContentRegionAvail();
            const float scale = std::min(std::max(area.x, 1.0f) / state.storyboardContentWidth,
                                         std::max(area.y, 1.0f) / state.storyboardContentHeight);
            m_zoom = std::clamp(scale, 0.35f, 2.0f);
            m_panX = 16.0f;
            m_panY = 16.0f;
        }
        commands.Push(Core::FitStoryboardCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("聚焦当前")) {
        commands.Push(Core::FocusStoryboardSelectionCommand{});
        if (state.storyboardCards != nullptr) {
            for (const StoryboardCardView& card : *state.storyboardCards) {
                if (card.selected) {
                    m_panX = 80.0f - card.x * m_zoom;
                    m_panY = 80.0f - card.y * m_zoom;
                    break;
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("刷新缩略图")) {
        commands.Push(Core::RefreshStoryboardThumbnailCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("导出分镜总览")) {
        commands.Push(Core::ExportStoryboardBoardCommand{});
    }
    ImGui::SameLine();
    ImGui::Text("缩放 %.0f%%", m_zoom * 100.0f);
    if (state.storyboardHeldLastValid) {
        ImGui::TextUnformatted("剧本有错误，画布未更新");
    }

    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 8.0f) {
        canvasSize.x = 8.0f;
    }
    if (canvasSize.y < 8.0f) {
        canvasSize.y = 8.0f;
    }
    ImGui::InvisibleButton("StoryboardCanvas", canvasSize);
    const bool hovered = ImGui::IsItemHovered();
    const ImGuiIO& io = ImGui::GetIO();
    if (hovered && std::abs(io.MouseWheel) > 0.0f) {
        const float factor = io.MouseWheel > 0.0f ? 1.1f : 0.9f;
        m_zoom = std::clamp(m_zoom * factor, 0.35f, 2.0f);
    }
    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }
    if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        std::abs(io.MouseDelta.x) < 1.0f) {
        const ImVec2 local(io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y);
        if (const StoryboardCardView* card = HitCard(state, local, m_panX, m_panY, m_zoom)) {
            if (card->kind == "scene") {
                commands.Push(Core::SetStoryboardSceneCollapsedCommand{card->sceneId, !card->collapsed});
            } else if (card->kind == "shot") {
                commands.Push(Core::SelectShotCommand{card->shotId});
            }
        }
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                       true);
    draw->AddRectFilled(canvasPos,
                        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                        IM_COL32(24, 26, 32, 255));
    if (state.storyboardCards != nullptr) {
        for (const StoryboardCardView& card : *state.storyboardCards) {
            if (card.kind == "shot" || card.kind == "scene") {
                const ImVec2 from = WorldToScreen(card.x - 48.0f, card.y + card.h * 0.5f, m_panX,
                                                  m_panY, m_zoom);
                const ImVec2 to =
                    WorldToScreen(card.x, card.y + card.h * 0.5f, m_panX, m_panY, m_zoom);
                draw->AddLine(ImVec2(canvasPos.x + from.x, canvasPos.y + from.y),
                              ImVec2(canvasPos.x + to.x, canvasPos.y + to.y),
                              IM_COL32(90, 100, 120, 255), 1.5f);
            }
        }
        for (const StoryboardCardView& card : *state.storyboardCards) {
            const ImVec2 min = WorldToScreen(card.x, card.y, m_panX, m_panY, m_zoom);
            const ImVec2 max = WorldToScreen(card.x + card.w, card.y + card.h, m_panX, m_panY, m_zoom);
            ImU32 color = IM_COL32(46, 52, 64, 255);
            if (card.kind == "root") {
                color = IM_COL32(40, 70, 80, 255);
            } else if (card.kind == "scene") {
                color = IM_COL32(52, 48, 72, 255);
            }
            if (card.selected) {
                color = IM_COL32(70, 90, 130, 255);
            }
            draw->AddRectFilled(ImVec2(canvasPos.x + min.x, canvasPos.y + min.y),
                                ImVec2(canvasPos.x + max.x, canvasPos.y + max.y), color, 6.0f);
            draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 8.0f),
                          IM_COL32(235, 235, 235, 255), card.title.c_str());
            if (card.kind == "shot") {
                char meta[128];
                std::snprintf(meta, sizeof(meta), "%s · %s · %s", card.link, card.preview,
                              card.exported);
                draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 28.0f),
                              IM_COL32(180, 180, 190, 255), meta);
                if (card.thumbTexture != 0xFFFFu) {
                    const ImVec2 thumbMin(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 48.0f);
                    const ImVec2 thumbMax(canvasPos.x + max.x - 8.0f, canvasPos.y + max.y - 8.0f);
                    draw->AddImage(ImTextureRef(static_cast<ImTextureID>(card.thumbTexture)),
                                   thumbMin, thumbMax);
                }
            } else if (card.kind == "scene" && card.collapsed) {
                draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 32.0f),
                              IM_COL32(180, 180, 190, 255), "已折叠");
            }
        }
    }
    draw->PopClipRect();
    commands.Push(Core::ReportStoryboardViewCommand{m_panX, m_panY, m_zoom, canvasSize.x,
                                                    canvasSize.y});
    ImGui::End();
}

} // namespace DirectorDesk::UI
