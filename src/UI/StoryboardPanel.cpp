// StoryboardPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/StoryboardPanel.h"

#include "DirectorDesk/Core/Command.h"
#include "UiChrome.h"
#include "UiFonts.h"
#include "UiIcons.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kAccent(0.847f, 0.604f, 0.290f, 1.0f);
constexpr ImVec4 kWarning(0.878f, 0.537f, 0.290f, 1.0f);
constexpr ImVec4 kSuccess(0.310f, 0.749f, 0.498f, 1.0f);
constexpr ImVec4 kMuted(0.604f, 0.604f, 0.635f, 1.0f);

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
        if (it->kindEnum == CardKind::Root) {
            continue;
        }
        if (wx >= it->x && wx <= it->x + it->w && wy >= it->y && wy <= it->y + it->h) {
            return &(*it);
        }
    }
    return nullptr;
}

const char* ModeId(const AppViewState& state) {
    return state.workspaceModeId != nullptr && state.workspaceModeId[0] != '\0'
               ? state.workspaceModeId
               : "shoot";
}

void DrawStatusDots(const StoryboardCardView& card) {
    auto dot = [](bool on) {
        ImGui::SameLine();
        ImGui::TextColored(on ? kSuccess : kMuted, "%s", on ? "●" : "○");
    };
    dot(card.linkEnum == LinkStatus::Linked);
    dot(card.previewEnum == PreviewStatus::Ready);
    dot(card.exportEnum == ExportStatus::Exported);
}

bool CardUnready(const StoryboardCardView& card) {
    if (card.kindEnum != CardKind::Shot) {
        return false;
    }
    const bool unlinked = card.linkEnum == LinkStatus::Unlinked;
    const bool previewBad = card.previewEnum == PreviewStatus::Stale ||
                            card.previewEnum == PreviewStatus::Failed ||
                            card.previewEnum == PreviewStatus::Missing;
    return unlinked || previewBad;
}

std::string ShotStripCaption(const AppViewState& state, const StoryboardCardView& card) {
    int sceneN = 0;
    if (state.scriptScenes != nullptr) {
        for (const ScriptSceneView& scene : *state.scriptScenes) {
            ++sceneN;
            int shotN = 0;
            for (const ScriptShotView& shot : scene.shots) {
                ++shotN;
                if (shot.id == card.shotId) {
                    return std::to_string(sceneN) + "." + std::to_string(shotN) + " " + card.title;
                }
            }
        }
    }
    return card.title;
}

bool ReviewHasExportLog(const AppViewState& state) {
    return state.exportLog != nullptr && !state.exportLog->empty();
}

void DrawShotStrip(const AppViewState& state, Core::CommandQueue& commands) {
    const char* mode = ModeId(state);
    if (std::strcmp(mode, "script") == 0 || state.projectIsEmpty) {
        return;
    }
    if (std::strcmp(mode, "review") == 0 && !ReviewHasExportLog(state)) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiWindowFlags sideFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(UiPx(8.0f), UiPx(2.0f)));
    if (!ImGui::BeginViewportSideBar("镜头条###ShotStrip", viewport, ImGuiDir_Down,
                                     ShotStripBarH(state.windowWidth),
                                     sideFlags)) {
        ImGui::PopStyleVar();
        ImGui::End();
        return;
    }

    if (std::strcmp(mode, "review") == 0) {
        DrawPanelCaption("导出记录");
        for (int i = static_cast<int>(state.exportLog->size()) - 1; i >= 0; --i) {
            const ExportLogView& entry = (*state.exportLog)[static_cast<std::size_t>(i)];
            ImGui::PushID(i);
            const bool failed = !entry.ok;
            if (failed) {
                ImGui::PushStyleColor(ImGuiCol_Text, kWarning);
            }
            const std::string label = entry.label + "  " +
                                      (entry.shotTitle.empty() ? "分镜总览" : entry.shotTitle) +
                                      "  " + (entry.ok ? "成功" : "失败");
            if (ImGui::Selectable(label.c_str())) {
                if (FindShotCardById(state.storyboardCards, entry.shotId) != nullptr) {
                    commands.Push(Core::SelectShotCommand{entry.shotId});
                }
            }
            if (failed) {
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", entry.path.c_str());
            if (!entry.path.empty()) {
                ImGui::SameLine();
                char openFile[32];
                char openFolder[32];
                std::snprintf(openFile, sizeof(openFile), "%s 打开", Icon::FileInput);
                std::snprintf(openFolder, sizeof(openFolder), "%s 文件夹", Icon::FolderOpen);
                if (ImGui::SmallButton(openFile)) {
                    commands.Push(Core::RevealPathCommand{entry.path, false});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(openFolder)) {
                    commands.Push(Core::RevealPathCommand{entry.path, true});
                }
            }
            if (!entry.message.empty()) {
                ImGui::TextWrapped("%s", entry.message.c_str());
            }
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    if (state.storyboardCards == nullptr) {
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
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    const float thumbW = ShotStripThumbW(state.windowWidth);
    const float thumbH = ShotStripThumbH(state.windowWidth);
    const float captionH = UiPx(18.0f);
    ImGui::BeginChild("ShotStripRow", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::IsWindowHovered()) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f) {
            ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseWheel * UiPx(48.0f));
            io.MouseWheel = 0.0f;
        }
    }
    int index = 0;
    for (const StoryboardCardView& card : *state.storyboardCards) {
        if (card.kindEnum != CardKind::Shot) {
            continue;
        }
        ImGui::PushID(card.shotId.c_str());
        if (index > 0) {
            ImGui::SameLine();
        }
        const ImVec2 childSize(thumbW + UiPx(8.0f), thumbH + UiPx(8.0f));
        ImGui::BeginChild(("cell" + card.shotId).c_str(), childSize, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar);
        if (card.selected) {
            const ImVec2 min = ImGui::GetWindowPos();
            const ImVec2 max = ImVec2(min.x + childSize.x, min.y + childSize.y);
            ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(kAccent), 0.0f, 0,
                                                UiPx(2.0f));
        }
        const ImVec2 thumbMin = ImGui::GetCursorScreenPos();
        const ImVec2 thumbSize(thumbW, thumbH);
        if (ImGui::InvisibleButton("##thumb", thumbSize)) {
            commands.Push(Core::SelectShotCommand{card.shotId});
        }
        if (ImGui::BeginPopupContextItem("shot-cell-menu")) {
            if (ImGui::MenuItem("在此后插入镜头")) {
                commands.Push(Core::InsertShotCommand{card.shotId});
            }
            if (ImGui::MenuItem("刷新预览")) {
                commands.Push(Core::RefreshStoryboardThumbnailCommand{card.shotId});
            }
            if (ImGui::MenuItem("导出镜头包")) {
                commands.Push(Core::ExportShotPackageCommand{card.shotId, {}});
            }
            if (ImGui::MenuItem("聚焦")) {
                commands.Push(Core::FocusStoryboardSelectionCommand{});
            }
            ImGui::EndPopup();
        }
        if (card.thumbTexture != 0xFFFFu) {
            ImGui::GetWindowDrawList()->AddImage(
                ImTextureRef(static_cast<ImTextureID>(card.thumbTexture)), thumbMin,
                ImVec2(thumbMin.x + thumbSize.x, thumbMin.y + thumbSize.y));
        } else {
            ImGui::GetWindowDrawList()->AddRectFilled(
                thumbMin, ImVec2(thumbMin.x + thumbSize.x, thumbMin.y + thumbSize.y),
                IM_COL32(42, 42, 47, 255));
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(thumbMin.x + UiPx(8.0f), thumbMin.y + UiPx(8.0f)),
                ImGui::GetColorU32(ImGuiCol_TextDisabled), "无预览");
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float barTop = thumbMin.y + thumbH - captionH;
        draw->AddRectFilled(ImVec2(thumbMin.x, barTop),
                            ImVec2(thumbMin.x + thumbW, thumbMin.y + thumbH),
                            IM_COL32(12, 12, 16, 210));
        const std::string caption = ShotStripCaption(state, card);
        draw->AddText(ImVec2(thumbMin.x + UiPx(4.0f), barTop + UiPx(2.0f)),
                      ImGui::GetColorU32(kMuted), caption.c_str());
        auto dot = [&](bool on, float slot) {
            const ImVec2 c(thumbMin.x + thumbW - UiPx(8.0f) - slot * UiPx(10.0f),
                           barTop + captionH * 0.5f);
            draw->AddCircleFilled(c, UiPx(3.0f), ImGui::GetColorU32(on ? kSuccess : kMuted));
        };
        dot(card.exportEnum == ExportStatus::Exported, 0.0f);
        dot(card.previewEnum == PreviewStatus::Ready, 1.0f);
        dot(card.linkEnum == LinkStatus::Linked, 2.0f);
        ImGui::EndChild();
        if (card.selected) {
            static std::string scrolled;
            if (scrolled != card.shotId) {
                ImGui::SetScrollHereX(0.5f);
                scrolled = card.shotId;
            }
        }
        ImGui::PopID();
        ++index;
    }
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace

void StoryboardPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    DrawShotStrip(state, commands);

    const char* mode = ModeId(state);
    if (std::strcmp(mode, "review") != 0) {
        return;
    }

    ImGui::Begin("分镜###Storyboard");
    DrawPanelCaption("分镜总览");
    ImGui::TextUnformatted("分镜总览");
    ImGui::SameLine();
    ImGui::TextDisabled("%.0f%%", m_zoom * 100.0f);
    if (ImGui::SmallButton("适配全部")) {
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
    if (ImGui::SmallButton("聚焦当前")) {
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
    if (ImGui::SmallButton("刷新预览")) {
        commands.Push(Core::RefreshStoryboardThumbnailCommand{});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("导出总览")) {
        commands.Push(Core::ExportStoryboardBoardCommand{});
    }
    if (state.storyboardHeldLastValid) {
        ImGui::TextColored(kWarning, "剧本有错误，画布未更新");
    } else if (state.storyboardCards == nullptr || state.storyboardCards->empty()) {
        ImGui::TextUnformatted("还没有分镜。打开或新建剧本后会生成总览。");
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
            if (card->kindEnum == CardKind::Scene) {
                commands.Push(
                    Core::SetStoryboardSceneCollapsedCommand{card->sceneId, !card->collapsed});
            } else if (card->kindEnum == CardKind::Shot) {
                commands.Push(Core::SelectShotCommand{card->shotId});
            }
        }
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                       true);
    draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                        IM_COL32(11, 13, 18, 255));
    const float kGrid = UiPx(32.0f);
    const float gridOffsetX = std::fmod(m_panX, kGrid);
    const float gridOffsetY = std::fmod(m_panY, kGrid);
    const float gridThickness = UiPx(1.0f);
    for (float x = gridOffsetX; x < canvasSize.x; x += kGrid) {
        draw->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                      ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
                      IM_COL32(49, 57, 70, 52), gridThickness);
    }
    for (float y = gridOffsetY; y < canvasSize.y; y += kGrid) {
        draw->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                      IM_COL32(49, 57, 70, 52), gridThickness);
    }
    if (state.storyboardCards != nullptr) {
        auto shotCountInScene = [&](const std::string& sceneId) {
            int count = 0;
            for (const StoryboardCardView& item : *state.storyboardCards) {
                if (item.kindEnum == CardKind::Shot && item.sceneId == sceneId) {
                    ++count;
                }
            }
            return count;
        };
        for (const StoryboardCardView& card : *state.storyboardCards) {
            if (card.kindEnum == CardKind::Root) {
                continue;
            }
            const ImVec2 min = WorldToScreen(card.x, card.y, m_panX, m_panY, m_zoom);
            ImVec2 max = WorldToScreen(card.x + card.w, card.y + card.h, m_panX, m_panY, m_zoom);
            if (card.kindEnum == CardKind::Scene) {
                const float bannerH = std::min(max.y - min.y, UiPx(28.0f) * m_zoom);
                max.y = min.y + std::max(bannerH, 22.0f);
            }
            ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
            if (card.kindEnum == CardKind::Scene) {
                color = ImGui::GetColorU32(ImGuiCol_TabSelected);
            }
            if (card.selected) {
                color = ImGui::GetColorU32(ImGuiCol_HeaderActive);
            }
            const ImVec2 cardMin(canvasPos.x + min.x, canvasPos.y + min.y);
            const ImVec2 cardMax(canvasPos.x + max.x, canvasPos.y + max.y);
            draw->AddRectFilled(cardMin, cardMax, color, 4.0f);
            const bool unready = CardUnready(card);
            const ImU32 border = card.selected ? ImGui::GetColorU32(ImGuiCol_SeparatorActive)
                                               : (unready ? IM_COL32(238, 130, 89, 255)
                                                          : ImGui::GetColorU32(ImGuiCol_Border));
            draw->AddRect(cardMin, cardMax, border, 4.0f, 0,
                          card.selected || unready ? 2.0f : 1.0f);
            if (card.kindEnum == CardKind::Scene) {
                char banner[192];
                std::snprintf(banner, sizeof(banner), "%s  %d 镜  %s", card.title.c_str(),
                              shotCountInScene(card.sceneId), card.collapsed ? ">" : "v");
                draw->AddText(ImVec2(cardMin.x + 8.0f, cardMin.y + 6.0f),
                              ImGui::GetColorU32(ImGuiCol_Text), banner);
                continue;
            }
            draw->AddText(ImVec2(cardMin.x + 8.0f, cardMin.y + 6.0f),
                          ImGui::GetColorU32(ImGuiCol_Text), card.title.c_str());
            float textY = cardMin.y + 24.0f;
            if (!card.metaLine.empty()) {
                draw->AddText(ImVec2(cardMin.x + 8.0f, textY),
                              ImGui::GetColorU32(ImGuiCol_TextDisabled), card.metaLine.c_str());
                textY += 16.0f;
            }
            const char* linkText =
                card.linkEnum == LinkStatus::Linked ? "已绑机位" : "无机位";
            const char* previewText = "无";
            if (card.previewEnum == PreviewStatus::Ready) {
                previewText = "最新";
            } else if (card.previewEnum == PreviewStatus::Stale) {
                previewText = "需刷新";
            } else if (card.previewEnum == PreviewStatus::Failed) {
                previewText = "失败";
            } else if (card.previewEnum == PreviewStatus::Rendering) {
                previewText = "渲染中";
            }
            char statusLine[128];
            std::snprintf(statusLine, sizeof(statusLine), "%s · %s", linkText, previewText);
            draw->AddText(ImVec2(cardMin.x + 8.0f, textY),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), statusLine);
            const float innerW = std::max(8.0f, (cardMax.x - cardMin.x) - 16.0f);
            const float thumbH = innerW * 9.0f / 16.0f;
            const float thumbTop = textY + 20.0f;
            const float thumbBottom = std::min(cardMin.y + 44.0f + thumbH, cardMax.y - 8.0f);
            if (thumbBottom > thumbTop + 8.0f) {
                const ImVec2 thumbMin(cardMin.x + 8.0f, thumbTop);
                const ImVec2 thumbMax(cardMin.x + 8.0f + innerW, thumbBottom);
                if (card.thumbTexture != 0xFFFFu) {
                    draw->AddImage(ImTextureRef(static_cast<ImTextureID>(card.thumbTexture)),
                                   thumbMin, thumbMax);
                } else {
                    draw->AddRectFilled(thumbMin, thumbMax, IM_COL32(42, 42, 47, 255));
                }
            }
        }
    }
    draw->PopClipRect();
    commands.Push(
        Core::ReportStoryboardViewCommand{m_panX, m_panY, m_zoom, canvasSize.x, canvasSize.y});
    ImGui::End();
}

} // namespace DirectorDesk::UI
