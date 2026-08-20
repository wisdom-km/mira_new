// StoryboardPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/StoryboardPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kAccent(0.85f, 0.60f, 0.29f, 1.0f);
constexpr ImVec4 kWarning(0.94f, 0.51f, 0.35f, 1.0f);
constexpr ImVec4 kLive(0.31f, 0.82f, 0.58f, 1.0f);
constexpr ImVec4 kMuted(0.56f, 0.61f, 0.68f, 1.0f);

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

const char* ModeId(const AppViewState& state) {
    return state.workspaceModeId != nullptr && state.workspaceModeId[0] != '\0'
               ? state.workspaceModeId
               : "shoot";
}

bool ProjectIsEmpty(const AppViewState& state) {
    const bool noProject = state.projectPath == nullptr || state.projectPath[0] == '\0';
    const bool noNodes = state.nodes == nullptr || state.nodes->empty();
    return noProject && !state.scriptHasSnapshot && noNodes;
}

void DrawStatusDots(const StoryboardCardView& card) {
    auto dot = [](bool on) {
        ImGui::SameLine();
        ImGui::TextColored(on ? kLive : kMuted, "%s", on ? "●" : "○");
    };
    dot(card.link != nullptr && std::strcmp(card.link, "已关联") == 0);
    dot(card.preview != nullptr && std::strcmp(card.preview, "就绪") == 0);
    dot(card.exported != nullptr && std::strcmp(card.exported, "已导出") == 0);
}

bool CardUnready(const StoryboardCardView& card) {
    if (card.kind != "shot") {
        return false;
    }
    const bool unlinked = card.link == nullptr || std::strcmp(card.link, "已关联") != 0;
    const bool previewBad = card.preview != nullptr && (std::strcmp(card.preview, "过期") == 0 ||
                                                        std::strcmp(card.preview, "失败") == 0 ||
                                                        std::strcmp(card.preview, "缺失") == 0);
    return unlinked || previewBad;
}

void DrawShotStrip(const AppViewState& state, Core::CommandQueue& commands) {
    ImGui::Begin("镜头条###ShotStrip");
    const char* mode = ModeId(state);
    const bool empty = ProjectIsEmpty(state);
    if (std::strcmp(mode, "review") == 0) {
        if (state.exportLog == nullptr || state.exportLog->empty()) {
            ImGui::TextDisabled("尚无导出记录");
        } else {
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
                    if (!entry.shotTitle.empty() && state.storyboardCards != nullptr) {
                        for (const StoryboardCardView& card : *state.storyboardCards) {
                            if (card.kind == "shot" && card.title == entry.shotTitle) {
                                commands.Push(Core::SelectShotCommand{card.shotId});
                                break;
                            }
                        }
                    }
                }
                if (failed) {
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("%s", entry.path.c_str());
                if (!entry.message.empty()) {
                    ImGui::TextWrapped("%s", entry.message.c_str());
                }
                ImGui::PopID();
            }
        }
        ImGui::End();
        return;
    }

    if (std::strcmp(mode, "script") == 0) {
        int scenes = 0;
        int shots = 0;
        if (state.scriptScenes != nullptr) {
            scenes = static_cast<int>(state.scriptScenes->size());
            for (const ScriptSceneView& scene : *state.scriptScenes) {
                shots += static_cast<int>(scene.shots.size());
            }
        }
        const int diagnostics = state.scriptDiagnostics != nullptr
                                    ? static_cast<int>(state.scriptDiagnostics->size())
                                    : 0;
        ImGui::Text("本剧本解析出 %d 场 %d 镜 · %d 诊断", scenes, shots, diagnostics);
        ImGui::End();
        return;
    }

    if (empty || state.storyboardCards == nullptr) {
        ImGui::TextDisabled("镜头表为空 · 打开剧本后生成");
        if (ImGui::SmallButton("打开剧本...")) {
            commands.Push(Core::LoadScriptCommand{});
        }
        if (state.exampleScriptPath != nullptr && state.exampleScriptPath[0] != '\0') {
            ImGui::SameLine();
            if (ImGui::SmallButton("打开示例剧本")) {
                commands.Push(Core::LoadScriptFromPathCommand{state.exampleScriptPath});
            }
        }
        ImGui::End();
        return;
    }

    const float cellW = std::strcmp(mode, "set") == 0 ? 72.0f : 96.0f;
    const float cellH = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ShotStripRow", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    int index = 0;
    for (const StoryboardCardView& card : *state.storyboardCards) {
        if (card.kind != "shot") {
            continue;
        }
        ImGui::PushID(card.shotId.c_str());
        if (index > 0) {
            ImGui::SameLine();
        }
        const ImVec2 childSize(cellW, std::max(cellH - 4.0f, 48.0f));
        ImGui::BeginChild(("cell" + card.shotId).c_str(), childSize, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar);
        if (card.selected) {
            const ImVec2 min = ImGui::GetWindowPos();
            ImGui::GetWindowDrawList()->AddRectFilled(min,
                                                      ImVec2(min.x + childSize.x, min.y + 3.0f),
                                                      ImGui::GetColorU32(ImGuiCol_SeparatorActive));
        }
        if (card.thumbTexture != 0xFFFFu) {
            const ImVec2 thumbMin = ImGui::GetCursorScreenPos();
            const ImVec2 thumbMax(thumbMin.x + childSize.x - 8.0f, thumbMin.y + 40.0f);
            ImGui::Dummy(ImVec2(childSize.x - 8.0f, 40.0f));
            ImGui::GetWindowDrawList()->AddImage(
                ImTextureRef(static_cast<ImTextureID>(card.thumbTexture)), thumbMin, thumbMax);
        } else {
            ImGui::Dummy(ImVec2(childSize.x - 8.0f, 40.0f));
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(28, 34, 45, 255));
            ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
            ImGui::TextDisabled("无预览");
        }
        if (ImGui::Selectable(card.title.c_str(), card.selected)) {
            commands.Push(Core::SelectShotCommand{card.shotId});
        }
        DrawStatusDots(card);
        if (ImGui::BeginPopupContextItem("shot-cell-menu")) {
            if (ImGui::MenuItem("重渲缩略图")) {
                commands.Push(Core::RefreshStoryboardThumbnailCommand{card.shotId});
            }
            if (ImGui::MenuItem("聚焦")) {
                commands.Push(Core::FocusStoryboardSelectionCommand{});
            }
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        if (card.selected) {
            ImGui::SetScrollHereX();
        }
        ImGui::PopID();
        ++index;
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace

void StoryboardPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    DrawShotStrip(state, commands);

    const char* mode = ModeId(state);
    if (std::strcmp(mode, "review") != 0) {
        return;
    }

    ImGui::Begin("分镜###Storyboard");
    ImGui::TextColored(kAccent, "STORYBOARD / BEAT MAP");
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
    if (ImGui::SmallButton("刷新缩略图")) {
        commands.Push(Core::RefreshStoryboardThumbnailCommand{});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("导出总览")) {
        commands.Push(Core::ExportStoryboardBoardCommand{});
    }
    if (state.storyboardHeldLastValid) {
        ImGui::TextColored(kWarning, "剧本有错误，画布未更新");
    } else if (state.storyboardCards == nullptr || state.storyboardCards->empty()) {
        ImGui::TextUnformatted("打开剧本后会自动生成分镜画布。");
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
                commands.Push(
                    Core::SetStoryboardSceneCollapsedCommand{card->sceneId, !card->collapsed});
            } else if (card->kind == "shot") {
                commands.Push(Core::SelectShotCommand{card->shotId});
            }
        }
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                       true);
    draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                        IM_COL32(11, 13, 18, 255));
    constexpr float kGrid = 32.0f;
    const float gridOffsetX = std::fmod(m_panX, kGrid);
    const float gridOffsetY = std::fmod(m_panY, kGrid);
    for (float x = gridOffsetX; x < canvasSize.x; x += kGrid) {
        draw->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                      ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
                      IM_COL32(49, 57, 70, 52));
    }
    for (float y = gridOffsetY; y < canvasSize.y; y += kGrid) {
        draw->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                      IM_COL32(49, 57, 70, 52));
    }
    if (state.storyboardCards != nullptr) {
        for (const StoryboardCardView& card : *state.storyboardCards) {
            if (card.kind == "shot" || card.kind == "scene") {
                const ImVec2 from =
                    WorldToScreen(card.x - 48.0f, card.y + card.h * 0.5f, m_panX, m_panY, m_zoom);
                const ImVec2 to =
                    WorldToScreen(card.x, card.y + card.h * 0.5f, m_panX, m_panY, m_zoom);
                draw->AddLine(ImVec2(canvasPos.x + from.x, canvasPos.y + from.y),
                              ImVec2(canvasPos.x + to.x, canvasPos.y + to.y),
                              ImGui::GetColorU32(ImGuiCol_Border), 1.5f);
            }
        }
        for (const StoryboardCardView& card : *state.storyboardCards) {
            const ImVec2 min = WorldToScreen(card.x, card.y, m_panX, m_panY, m_zoom);
            const ImVec2 max =
                WorldToScreen(card.x + card.w, card.y + card.h, m_panX, m_panY, m_zoom);
            ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
            if (card.kind == "root") {
                color = ImGui::GetColorU32(ImGuiCol_Header);
            } else if (card.kind == "scene") {
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
            if (card.selected) {
                draw->AddRectFilled(cardMin, ImVec2(cardMax.x, cardMin.y + 3.0f),
                                    ImGui::GetColorU32(ImGuiCol_SeparatorActive), 4.0f);
            }
            draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 8.0f),
                          ImGui::GetColorU32(ImGuiCol_Text), card.title.c_str());
            if (card.kind == "shot") {
                char meta[128];
                std::snprintf(meta, sizeof(meta), "%s · %s · %s", card.link, card.preview,
                              card.exported);
                draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 28.0f),
                              ImGui::GetColorU32(ImGuiCol_TextDisabled), meta);
                if (card.thumbTexture != 0xFFFFu) {
                    const ImVec2 thumbMin(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 48.0f);
                    const ImVec2 thumbMax(canvasPos.x + max.x - 8.0f, canvasPos.y + max.y - 8.0f);
                    draw->AddImage(ImTextureRef(static_cast<ImTextureID>(card.thumbTexture)),
                                   thumbMin, thumbMax);
                }
            } else if (card.kind == "scene" && card.collapsed) {
                draw->AddText(ImVec2(canvasPos.x + min.x + 8.0f, canvasPos.y + min.y + 32.0f),
                              ImGui::GetColorU32(ImGuiCol_TextDisabled), "已折叠");
            }
        }
    }
    draw->PopClipRect();
    commands.Push(
        Core::ReportStoryboardViewCommand{m_panX, m_panY, m_zoom, canvasSize.x, canvasSize.y});
    ImGui::End();
}

} // namespace DirectorDesk::UI
