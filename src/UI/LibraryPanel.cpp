// LibraryPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/LibraryPanel.h"

#include "DirectorDesk/Core/Command.h"
#include "UiChrome.h"
#include "UiFonts.h"
#include "UiIcons.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kMuted(0.604f, 0.604f, 0.635f, 1.0f);

bool IsIndexMissing(const LibraryAssetView& asset) {
    return asset.missing && !asset.canDownload;
}

void DrawAssetContextMenu(const LibraryAssetView& asset, Core::CommandQueue& commands) {
    if (asset.origin == "online") {
        return;
    }
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("删除")) {
            commands.Push(Core::RemoveLibraryAssetCommand{asset.id});
        }
        ImGui::EndPopup();
    }
}

void DrawAssetRow(const LibraryAssetView& asset, Core::CommandQueue& commands, bool grid) {
    const std::string label = asset.name + "##" + asset.id;
    if (grid) {
        const ImVec2 kThumb(UiPx(96.0f), UiPx(72.0f));
        constexpr ImU32 kCellBg = IM_COL32(0x2A, 0x2A, 0x2F, 255);
        constexpr ImU32 kCellBorder = IM_COL32(0x36, 0x36, 0x3C, 255);
        constexpr ImU32 kAccent = IM_COL32(216, 154, 74, 255);

        ImGui::PushID(asset.id.c_str());
        ImGui::BeginGroup();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("##thumb", kThumb)) {
            commands.Push(Core::SelectLibraryAssetCommand{asset.id});
        }
        DrawAssetContextMenu(asset, commands);
        if (asset.canAddToScene && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
            ImGui::TextUnformatted(asset.name.c_str());
            ImGui::EndDragDropSource();
        }
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 max(pos.x + kThumb.x, pos.y + kThumb.y);
        if (asset.previewTexture != 0xFFFFu) {
            draw->AddImage(ImTextureRef(static_cast<ImTextureID>(asset.previewTexture)), pos, max);
        } else {
            draw->AddRectFilled(pos, max, kCellBg);
            const char* cellLabel = asset.missing ? "缺失" : asset.format.c_str();
            const ImVec2 textSize = ImGui::CalcTextSize(cellLabel);
            draw->AddText(ImVec2(pos.x + (kThumb.x - textSize.x) * 0.5f,
                                 pos.y + (kThumb.y - textSize.y) * 0.5f),
                          ImGui::GetColorU32(ImGuiCol_TextDisabled), cellLabel);
        }
        draw->AddRect(pos, max, asset.selected ? kAccent : kCellBorder);
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + kThumb.x);
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::PopTextWrapPos();
        if (asset.hasSkin) {
            ImGui::TextColored(kMuted, "%s  ·  含骨骼", asset.origin.c_str());
        } else {
            ImGui::TextColored(kMuted, "%s", asset.origin.c_str());
        }
        ImGui::EndGroup();
        ImGui::PopID();
        return;
    }

    if (ImGui::Selectable(label.c_str(), asset.selected)) {
        commands.Push(Core::SelectLibraryAssetCommand{asset.id});
    }
    DrawAssetContextMenu(asset, commands);
    if (asset.canAddToScene && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::EndDragDropSource();
    }
    ImGui::SameLine();
    if (asset.hasSkin) {
        ImGui::TextDisabled("%s  ·  %s  ·  含骨骼", asset.format.c_str(), asset.status.c_str());
    } else {
        ImGui::TextDisabled("%s  ·  %s", asset.format.c_str(), asset.status.c_str());
    }
}

bool DrawSegment(const char* label, bool selected) {
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
    }
    const bool pressed = ImGui::SmallButton(label);
    if (selected) {
        ImGui::PopStyleColor();
    }
    return pressed;
}

} // namespace

void LibraryPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    const bool empty = state.projectIsEmpty;
    LeftRailState& rail = CurrentLeftRail();
    if (empty) {
        return;
    }
    if (rail.iconBar && rail.overlay != LeftRailOverlay::Library) {
        return;
    }

    if (state.librarySearch != nullptr && m_search != state.librarySearch) {
        m_search = state.librarySearch;
    }

    if (rail.iconBar) {
        ImGui::SetNextWindowPos(rail.overlayPos);
        ImGui::SetNextWindowSize(rail.overlaySize);
        ImGui::Begin("资源库###Library", nullptr,
                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar);
    } else {
        ImGui::Begin("资源库###Library");
    }
    DrawPanelCaption("资源库");
    const char* origin = state.libraryOriginFilter != nullptr ? state.libraryOriginFilter : "all";
    const bool online = std::strcmp(origin, "online") == 0;
    const char* viewMode = state.libraryViewMode != nullptr ? state.libraryViewMode : "grid";
    const bool grid = std::strcmp(viewMode, "grid") == 0;

    if (DrawSegment("本地", !online)) {
        if (online) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
        }
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (DrawSegment("在线", online)) {
        if (!online) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"online"});
        }
    }
    ImGui::SameLine(0.0f, 8.0f);
    char searchBuffer[128];
    const std::size_t copy =
        m_search.size() < sizeof(searchBuffer) - 1 ? m_search.size() : sizeof(searchBuffer) - 1;
    std::memcpy(searchBuffer, m_search.data(), copy);
    searchBuffer[copy] = '\0';
    const float overflowW = ImGui::GetFrameHeight() + 8.0f;
    ImGui::SetNextItemWidth(std::max(48.0f, ImGui::GetContentRegionAvail().x - overflowW));
    if (ImGui::InputTextWithHint("##library-search", "搜索...", searchBuffer,
                                 sizeof(searchBuffer))) {
        m_search = searchBuffer;
        commands.Push(Core::SetLibrarySearchCommand{m_search});
    }
    ImGui::SameLine();
    char moreLabel[24];
    std::snprintf(moreLabel, sizeof(moreLabel), "%s##library-more", Icon::Ellipsis);
    if (ImGui::SmallButton(moreLabel)) {
        ImGui::OpenPopup("##library-overflow");
    }
    if (ImGui::BeginPopup("##library-overflow")) {
        if (ImGui::MenuItem("导入")) {
            commands.Push(Core::ImportModelCommand{});
        }
        if (ImGui::MenuItem("刷新")) {
            if (online) {
                commands.Push(Core::RefreshOfficialCatalogCommand{});
            } else {
                commands.Push(Core::RefreshLibraryCommand{});
            }
        }
        if (ImGui::MenuItem(grid ? "列表" : "网格")) {
            commands.Push(Core::SetLibraryViewModeCommand{grid ? "list" : "grid"});
        }
        if (!online && state.libraryAssets != nullptr) {
            int missingCount = 0;
            for (const LibraryAssetView& asset : *state.libraryAssets) {
                if (IsIndexMissing(asset)) {
                    ++missingCount;
                }
            }
            if (missingCount > 0 && ImGui::MenuItem("清理缺失")) {
                for (const LibraryAssetView& asset : *state.libraryAssets) {
                    if (IsIndexMissing(asset)) {
                        commands.Push(Core::RemoveLibraryAssetCommand{asset.id});
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::BeginChild("##library-chips", ImVec2(0.0f, ImGui::GetFrameHeight() + 6.0f),
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    auto chip = [](const char* label, bool selected) {
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        }
        const bool pressed = ImGui::SmallButton(label);
        if (selected) {
            ImGui::PopStyleColor();
        }
        return pressed;
    };
    if (!online) {
        if (chip("全部", std::strcmp(origin, "all") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
        }
        ImGui::SameLine();
        if (chip("内置", std::strcmp(origin, "builtin") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"builtin"});
        }
        ImGui::SameLine();
        if (chip("用户", std::strcmp(origin, "user") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"user"});
        }
    } else if (state.officialCategories != nullptr) {
        if (chip("全部", state.officialCategory == nullptr || state.officialCategory[0] == '\0')) {
            commands.Push(Core::SetOfficialCategoryCommand{});
        }
        for (const std::string& category : *state.officialCategories) {
            ImGui::SameLine();
            if (chip(category.c_str(),
                     state.officialCategory != nullptr && category == state.officialCategory)) {
                commands.Push(Core::SetOfficialCategoryCommand{category});
            }
        }
    }
    ImGui::EndChild();

    if (state.libraryAssets != nullptr) {
        int column = 0;
        int visible = 0;
        const int gridColumns =
            std::max(2, static_cast<int>(ImGui::GetContentRegionAvail().x / UiPx(104.0f)));
        for (const LibraryAssetView& asset : *state.libraryAssets) {
            if (asset.kind == "skill" || asset.format == "skill") {
                continue;
            }
            if (IsIndexMissing(asset)) {
                continue;
            }
            ++visible;
            if (grid) {
                if (column > 0) {
                    ImGui::SameLine();
                }
                DrawAssetRow(asset, commands, true);
                column = (column + 1) % gridColumns;
            } else {
                DrawAssetRow(asset, commands, false);
            }
        }
        if (visible == 0) {
            if (online) {
                ImGui::TextUnformatted(state.officialConfigured
                                           ? "当前分类没有可显示的官方模型。"
                                           : "未配置官方清单，无法列出在线模型。");
                if (ImGui::Button("刷新", ImVec2(-1.0f, 0.0f))) {
                    commands.Push(Core::RefreshOfficialCatalogCommand{});
                }
            } else {
                ImGui::TextUnformatted("还没有模型。导入模型，或从在线清单下载。");
                if (ImGui::Button("导入模型", ImVec2(-1.0f, 0.0f))) {
                    commands.Push(Core::ImportModelCommand{});
                }
            }
        }
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
