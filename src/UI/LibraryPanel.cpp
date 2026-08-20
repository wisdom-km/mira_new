// LibraryPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/LibraryPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <algorithm>
#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

constexpr ImVec4 kMuted(0.56f, 0.61f, 0.68f, 1.0f);

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
        ImGui::PushID(asset.id.c_str());
        ImGui::BeginGroup();
        if (ImGui::Button(asset.missing ? "缺失" : asset.format.c_str(), ImVec2(72.0f, 48.0f))) {
            commands.Push(Core::SelectLibraryAssetCommand{asset.id});
        }
        DrawAssetContextMenu(asset, commands);
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
            ImGui::TextUnformatted(asset.name.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::TextColored(kMuted, "%s", asset.origin.c_str());
        ImGui::EndGroup();
        ImGui::PopID();
        return;
    }

    if (ImGui::Selectable(label.c_str(), asset.selected)) {
        commands.Push(Core::SelectLibraryAssetCommand{asset.id});
    }
    DrawAssetContextMenu(asset, commands);
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::EndDragDropSource();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s  ·  %s", asset.format.c_str(), asset.status.c_str());
}

} // namespace

void LibraryPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    const char* mode = state.workspaceModeId != nullptr && state.workspaceModeId[0] != '\0'
                           ? state.workspaceModeId
                           : "shoot";
    if (std::strcmp(mode, "script") == 0 || std::strcmp(mode, "review") == 0) {
        return;
    }

    if (state.librarySearch != nullptr && m_search != state.librarySearch) {
        m_search = state.librarySearch;
    }

    ImGui::Begin("资源库###Library");
    const char* origin = state.libraryOriginFilter != nullptr ? state.libraryOriginFilter : "all";
    const bool online = std::strcmp(origin, "online") == 0;
    if (ImGui::SmallButton("本地")) {
        if (online) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("在线")) {
        if (!online) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"online"});
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("刷新")) {
        if (online) {
            commands.Push(Core::RefreshOfficialCatalogCommand{});
        } else {
            commands.Push(Core::RefreshLibraryCommand{});
        }
    }
    ImGui::SameLine();
    char searchBuffer[128];
    const std::size_t copy =
        m_search.size() < sizeof(searchBuffer) - 1 ? m_search.size() : sizeof(searchBuffer) - 1;
    std::memcpy(searchBuffer, m_search.data(), copy);
    searchBuffer[copy] = '\0';
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    if (ImGui::InputTextWithHint("##library-search", "搜索...", searchBuffer,
                                 sizeof(searchBuffer))) {
        m_search = searchBuffer;
        commands.Push(Core::SetLibrarySearchCommand{m_search});
    }
    ImGui::SameLine();
    const char* viewMode = state.libraryViewMode != nullptr ? state.libraryViewMode : "list";
    const bool grid = std::strcmp(viewMode, "grid") == 0;
    if (ImGui::SmallButton(grid ? "▦" : "▤")) {
        commands.Push(Core::SetLibraryViewModeCommand{grid ? "list" : "grid"});
    }
    if (!online && state.libraryAssets != nullptr) {
        int missingCount = 0;
        for (const LibraryAssetView& asset : *state.libraryAssets) {
            if (IsIndexMissing(asset)) {
                ++missingCount;
            }
        }
        if (missingCount > 0) {
            ImGui::SameLine();
            if (ImGui::SmallButton("清理缺失")) {
                for (const LibraryAssetView& asset : *state.libraryAssets) {
                    if (IsIndexMissing(asset)) {
                        commands.Push(Core::RemoveLibraryAssetCommand{asset.id});
                    }
                }
            }
        }
    }

    if (!online) {
        if (ImGui::SmallButton("全部")) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("内置")) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"builtin"});
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("用户")) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"user"});
        }
    } else if (state.officialCategories != nullptr && !state.officialCategories->empty()) {
        const char* current = state.officialCategory != nullptr && state.officialCategory[0] != '\0'
                                  ? state.officialCategory
                                  : "全部分类";
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##official-cat", current)) {
            if (ImGui::Selectable("全部分类", state.officialCategory == nullptr ||
                                                  state.officialCategory[0] == '\0')) {
                commands.Push(Core::SetOfficialCategoryCommand{});
            }
            for (const std::string& category : *state.officialCategories) {
                if (ImGui::Selectable(category.c_str(), state.officialCategory != nullptr &&
                                                            category == state.officialCategory)) {
                    commands.Push(Core::SetOfficialCategoryCommand{category});
                }
            }
            ImGui::EndCombo();
        }
    }

    if (state.libraryAssets != nullptr) {
        int column = 0;
        int visible = 0;
        const int gridColumns =
            std::max(2, static_cast<int>(ImGui::GetContentRegionAvail().x / 84.0f));
        for (const LibraryAssetView& asset : *state.libraryAssets) {
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
            ImGui::TextUnformatted(online ? "没有可显示的官方资产。"
                                          : "资源库为空。导入模型后会保留在这里。");
        }
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
