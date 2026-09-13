// DispatchExport: Export / reveal Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/RevealPath.h"

#include <variant>

namespace DirectorDesk::App {
namespace {

void RequestExportPath(AppState& state, DispatchServices& services, bool board,
                        Export::ShotResolution resolution) {
    const std::string name =
        board ? (state.projectName + "-storyboard.png")
              : Export::DefaultShotFileName(state.projectName, state.script.SelectedShotId(),
                                            resolution, state.exportTransparent);
    if (!services.savePngFile) {
        return;
    }
    auto path = services.savePngFile(name);
    if (!path.IsOk()) {
        state.status = path.GetError().userMessage;
        return;
    }
    if (path.Value().empty()) {
        return;
    }
    if (Platform::Paths::Exists(path.Value())) {
        state.exportPendingPath = path.Value();
        state.exportPendingBoard = board;
        state.exportPendingResolution = resolution;
        state.exportOverwritePrompt = true;
        return;
    }
    if (board) {
        if (services.exportBoardTo) {
            services.exportBoardTo(path.Value());
        }
    } else if (services.exportShotTo) {
        services.exportShotTo(path.Value(), resolution);
    }
}

} // namespace

bool TryDispatchExport(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (const auto* typed = std::get_if<Core::RevealPathCommand>(&command)) {
        const auto revealed = Platform::RevealPath(typed->utf8Path, typed->folder);
        if (!revealed.IsOk()) {
            state.status = revealed.GetError().userMessage;
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ExportCurrentShotCommand>(&command)) {
        Export::ShotResolution resolution = Export::ShotResolution::Hd1080;
        const std::string resolutionId =
            typed->resolutionId.empty() ? state.exportResolutionId : typed->resolutionId;
        if (!Export::TryParseResolution(resolutionId, resolution)) {
            state.status = "未知导出分辨率";
        } else {
            RequestExportPath(state, services, false, resolution);
        }
        return true;
    }
    if (std::holds_alternative<Core::ExportStoryboardBoardCommand>(command)) {
        const Storyboard::PreviewIssueCount issues = state.storyboard.CountExportPreviewIssues();
        if (issues.Total() > 0) {
            state.exportStalePrompt = true;
            state.exportStaleCount = issues.Total();
        } else {
            RequestExportPath(state, services, true, Export::ShotResolution::Hd1080);
        }
        return true;
    }
    if (std::holds_alternative<Core::ConfirmStoryboardStaleExportCommand>(command)) {
        state.exportStalePrompt = false;
        state.exportStaleCount = 0;
        RequestExportPath(state, services, true, Export::ShotResolution::Hd1080);
        return true;
    }
    if (std::holds_alternative<Core::CancelStoryboardStaleExportCommand>(command)) {
        state.exportStalePrompt = false;
        state.exportStaleCount = 0;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetExportTransparentCommand>(&command)) {
        state.exportTransparent = typed->transparent;
        return true;
    }
    if (std::holds_alternative<Core::ConfirmExportOverwriteCommand>(command)) {
        state.exportOverwritePrompt = false;
        if (state.exportPendingBoard) {
            if (services.exportBoardTo) {
                services.exportBoardTo(state.exportPendingPath);
            }
        } else if (services.exportShotTo) {
            services.exportShotTo(state.exportPendingPath, state.exportPendingResolution);
        }
        state.exportPendingPath.clear();
        return true;
    }
    if (std::holds_alternative<Core::CancelExportOverwriteCommand>(command)) {
        state.exportOverwritePrompt = false;
        state.exportPendingPath.clear();
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectExportResolutionCommand>(&command)) {
        Export::ShotResolution parsed = Export::ShotResolution::Hd1080;
        if (!Export::TryParseResolution(typed->resolutionId, parsed)) {
            state.status = "未知导出分辨率";
        } else {
            state.exportResolutionId = typed->resolutionId;
            state.status = std::string("导出分辨率 ") + state.exportResolutionId;
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
