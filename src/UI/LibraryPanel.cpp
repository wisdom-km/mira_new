// LibraryPanel: Implementation for the DirectorDesk UI module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/UI/LibraryPanel.h"

#include "DirectorDesk/Core/Command.h"

#include <cstring>
#include <imgui.h>

namespace DirectorDesk::UI {
namespace {

void DrawAssetRow(const LibraryAssetView& asset, Core::CommandQueue& commands, bool grid) {
    const std::string label = asset.name + "##" + asset.id;
    if (grid) {
        ImGui::PushID(asset.id.c_str());
        ImGui::BeginGroup();
        if (ImGui::Button(asset.missing ? "缺失" : asset.format.c_str(), ImVec2(88.0f, 56.0f))) {
            commands.Push(Core::SelectLibraryAssetCommand{asset.id});
        }
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
            ImGui::TextUnformatted(asset.name.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::TextUnformatted(asset.origin.c_str());
        ImGui::EndGroup();
        ImGui::PopID();
        return;
    }

    if (ImGui::Selectable(label.c_str(), asset.selected)) {
        commands.Push(Core::SelectLibraryAssetCommand{asset.id});
    }
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("DD_ASSET_ID", asset.id.c_str(), asset.id.size() + 1);
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::EndDragDropSource();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s %s", asset.format.c_str(), asset.status.c_str());
}

} // namespace

void LibraryPanel::Draw(const AppViewState& state, Core::CommandQueue& commands) {
    if (state.librarySearch != nullptr && m_search != state.librarySearch) {
        m_search = state.librarySearch;
    }

    ImGui::Begin("资源库###Library");
    if (ImGui::Button("导入...")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("刷新")) {
        if (std::strcmp(state.libraryOriginFilter != nullptr ? state.libraryOriginFilter : "all",
                        "online") == 0) {
            commands.Push(Core::RefreshOfficialCatalogCommand{});
        } else {
            commands.Push(Core::RefreshLibraryCommand{});
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("加入场景")) {
        if (state.libraryAssets != nullptr) {
            for (const LibraryAssetView& asset : *state.libraryAssets) {
                if (asset.selected) {
                    commands.Push(Core::AddLibraryAssetToSceneCommand{asset.id});
                    break;
                }
            }
        }
    }

    const char* origin = state.libraryOriginFilter != nullptr ? state.libraryOriginFilter : "all";
    if (ImGui::BeginTabBar("library-origin")) {
        if (ImGui::BeginTabItem("本地")) {
            if (std::strcmp(origin, "online") == 0) {
                commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("在线")) {
            if (std::strcmp(origin, "online") != 0) {
                commands.Push(Core::SetLibraryOriginFilterCommand{"online"});
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    char searchBuffer[128];
    const std::size_t copy =
        m_search.size() < sizeof(searchBuffer) - 1 ? m_search.size() : sizeof(searchBuffer) - 1;
    std::memcpy(searchBuffer, m_search.data(), copy);
    searchBuffer[copy] = '\0';
    if (ImGui::InputText("搜索", searchBuffer, sizeof(searchBuffer))) {
        m_search = searchBuffer;
        commands.Push(Core::SetLibrarySearchCommand{m_search});
    }

    if (std::strcmp(origin, "online") != 0) {
        if (ImGui::RadioButton("全部", std::strcmp(origin, "all") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("内置", std::strcmp(origin, "builtin") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"builtin"});
        }
        if (ImGui::RadioButton("用户", std::strcmp(origin, "user") == 0)) {
            commands.Push(Core::SetLibraryOriginFilterCommand{"user"});
        }
    }

    const char* mode = state.libraryViewMode != nullptr ? state.libraryViewMode : "list";
    if (ImGui::RadioButton("列表", std::strcmp(mode, "list") == 0)) {
        commands.Push(Core::SetLibraryViewModeCommand{"list"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("网格", std::strcmp(mode, "grid") == 0)) {
        commands.Push(Core::SetLibraryViewModeCommand{"grid"});
    }

    if (std::strcmp(origin, "online") == 0) {
        if (!state.officialConfigured) {
            ImGui::TextUnformatted("官方地址未配置，仅显示本地缓存。");
        }
        if (state.officialCatalogStatus != nullptr && state.officialCatalogStatus[0] != '\0') {
            ImGui::TextUnformatted(state.officialCatalogStatus);
        }
        if (state.officialCategories != nullptr) {
            if (ImGui::RadioButton("全部分类",
                                   state.officialCategory == nullptr ||
                                       state.officialCategory[0] == '\0')) {
                commands.Push(Core::SetOfficialCategoryCommand{});
            }
            for (const std::string& category : *state.officialCategories) {
                ImGui::SameLine();
                if (ImGui::RadioButton(category.c_str(), state.officialCategory != nullptr &&
                                                             category == state.officialCategory)) {
                    commands.Push(Core::SetOfficialCategoryCommand{category});
                }
            }
        }
    }

    ImGui::Separator();
    const bool grid = std::strcmp(mode, "grid") == 0;
    if (state.libraryAssets != nullptr) {
        int column = 0;
        for (const LibraryAssetView& asset : *state.libraryAssets) {
            if (grid) {
                if (column > 0) {
                    ImGui::SameLine();
                }
                DrawAssetRow(asset, commands, true);
                column = (column + 1) % 3;
            } else {
                DrawAssetRow(asset, commands, false);
            }
        }
        if (state.libraryAssets->empty()) {
            ImGui::TextUnformatted(std::strcmp(origin, "online") == 0
                                       ? "没有可显示的官方资产。"
                                       : "资源库为空。导入模型后会保留在这里。");
        }
        if (std::strcmp(origin, "online") == 0) {
            for (const LibraryAssetView& asset : *state.libraryAssets) {
                if (!asset.selected) {
                    continue;
                }
                ImGui::Separator();
                ImGui::TextWrapped("%s", asset.description.c_str());
                ImGui::Text("许可: %s", asset.license.c_str());
                ImGui::Text("作者: %s", asset.author.c_str());
                ImGui::Text("状态: %s  %.0f%%", asset.status.c_str(), asset.progress * 100.0f);
                if (asset.canDownload && ImGui::Button("下载")) {
                    commands.Push(Core::DownloadOfficialAssetCommand{asset.id});
                }
                if (asset.canCancel) {
                    ImGui::SameLine();
                    if (ImGui::Button("取消下载")) {
                        commands.Push(Core::CancelOfficialDownloadCommand{asset.id});
                    }
                }
                break;
            }
        }
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
