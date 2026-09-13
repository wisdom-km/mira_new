// DispatchLibrary: Library / official catalog Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"

#include <variant>

namespace DirectorDesk::App {

bool TryDispatchLibrary(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (const auto* typed = std::get_if<Core::SetLibrarySearchCommand>(&command)) {
        state.librarySearch = typed->text;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetLibraryOriginFilterCommand>(&command)) {
        state.libraryOriginFilter = typed->originFilter;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetLibraryViewModeCommand>(&command)) {
        state.libraryViewMode = typed->viewMode;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectLibraryAssetCommand>(&command)) {
        state.selectedLibraryAssetId = typed->assetId;
        if (typed->assetId.empty()) {
            if (state.selectionKind == "asset") {
                state.selectionKind = "none";
                state.selectionId.clear();
                state.selectionLabel.clear();
            }
        } else {
            state.selectionKind = "asset";
            state.selectionId = typed->assetId;
            state.selectionLabel =
                LibraryAssetLabel(state.library, state.officialCatalog, typed->assetId);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::RemoveLibraryAssetCommand>(&command)) {
        if (!state.library.Remove(typed->assetId)) {
            state.status = "无法从资源库删除";
        } else {
            if (state.selectedLibraryAssetId == typed->assetId) {
                state.selectedLibraryAssetId.clear();
            }
            if (state.selectionKind == "asset" && state.selectionId == typed->assetId) {
                state.selectionKind = "none";
                state.selectionId.clear();
                state.selectionLabel.clear();
            }
            state.status = "已从资源库删除";
        }
        return true;
    }
    if (std::holds_alternative<Core::RefreshLibraryCommand>(command)) {
        state.library.Refresh();
        state.status = "已刷新资源库";
        return true;
    }
    if (std::holds_alternative<Core::RefreshOfficialCatalogCommand>(command)) {
        if (services.submitOfficialRefresh) {
            services.submitOfficialRefresh();
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetOfficialCategoryCommand>(&command)) {
        state.officialCategory = typed->categoryId;
        return true;
    }
    if (const auto* typed = std::get_if<Core::DownloadOfficialAssetCommand>(&command)) {
        if (services.submitOfficialDownload) {
            services.submitOfficialDownload(typed->assetId);
        } else {
            state.status = "无法下载官方资产";
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::CancelOfficialDownloadCommand>(&command)) {
        const auto found = state.officialCancels.find(typed->assetId);
        if (found != state.officialCancels.end()) {
            found->second->store(true);
            state.status = "正在取消下载";
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
