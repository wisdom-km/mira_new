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

    ImGui::Begin("Library");
    if (ImGui::Button("Import...")) {
        commands.Push(Core::ImportModelCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        commands.Push(Core::RefreshLibraryCommand{});
    }
    ImGui::SameLine();
    if (ImGui::Button("Add to Scene")) {
        if (state.libraryAssets != nullptr) {
            for (const LibraryAssetView& asset : *state.libraryAssets) {
                if (asset.selected) {
                    commands.Push(Core::AddLibraryAssetToSceneCommand{asset.id});
                    break;
                }
            }
        }
    }

    char searchBuffer[128];
    const std::size_t copy =
        m_search.size() < sizeof(searchBuffer) - 1 ? m_search.size() : sizeof(searchBuffer) - 1;
    std::memcpy(searchBuffer, m_search.data(), copy);
    searchBuffer[copy] = '\0';
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        m_search = searchBuffer;
        commands.Push(Core::SetLibrarySearchCommand{m_search});
    }

    const char* origin = state.libraryOriginFilter != nullptr ? state.libraryOriginFilter : "all";
    if (ImGui::RadioButton("All", std::strcmp(origin, "all") == 0)) {
        commands.Push(Core::SetLibraryOriginFilterCommand{"all"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Builtin", std::strcmp(origin, "builtin") == 0)) {
        commands.Push(Core::SetLibraryOriginFilterCommand{"builtin"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("User", std::strcmp(origin, "user") == 0)) {
        commands.Push(Core::SetLibraryOriginFilterCommand{"user"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Online", std::strcmp(origin, "online") == 0)) {
        commands.Push(Core::SetLibraryOriginFilterCommand{"online"});
    }

    const char* mode = state.libraryViewMode != nullptr ? state.libraryViewMode : "list";
    if (ImGui::RadioButton("List", std::strcmp(mode, "list") == 0)) {
        commands.Push(Core::SetLibraryViewModeCommand{"list"});
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Grid", std::strcmp(mode, "grid") == 0)) {
        commands.Push(Core::SetLibraryViewModeCommand{"grid"});
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
            ImGui::TextUnformatted("资源库为空。导入模型后会保留在这里。");
        }
    }
    ImGui::End();
}

} // namespace DirectorDesk::UI
