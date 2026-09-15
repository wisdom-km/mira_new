// DispatchExport: Export / reveal Command family (FND-10 / FND-21).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/RevealPath.h"

#include <variant>

namespace DirectorDesk::App {
namespace {

bool PackageFilesExist(const std::string& chosenPath, const std::string& shotId) {
    const std::string directory = Platform::Paths::Parent(chosenPath);
    const std::string png = Platform::Paths::Join(directory, shotId + ".png");
    const std::string json = Platform::Paths::Join(directory, shotId + ".shot.json");
    return Platform::Paths::Exists(png) || Platform::Paths::Exists(json);
}

void RequestExportPath(AppState& state, DispatchServices& services, bool board, bool package,
                       Export::ShotResolution resolution, const std::string& shotId) {
    const std::string name = board ? (state.projectName + "-storyboard.png")
                                   : (package ? (shotId + ".png")
                                              : Export::DefaultShotFileName(
                                                    state.projectName, shotId, resolution,
                                                    state.exportTransparent));
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
    const bool exists =
        package ? PackageFilesExist(path.Value(), shotId) : Platform::Paths::Exists(path.Value());
    if (exists) {
        state.exportPendingPath = path.Value();
        state.exportPendingBoard = board;
        state.exportPendingPackage = package;
        state.exportPendingPdf = false;
        state.exportPendingShotId = shotId;
        state.exportPendingResolution = resolution;
        state.exportOverwritePrompt = true;
        return;
    }
    if (board) {
        if (services.exportBoardTo) {
            services.exportBoardTo(path.Value());
        }
    } else if (package) {
        if (services.exportShotPackageTo) {
            services.exportShotPackageTo(path.Value(), resolution, shotId);
        }
    } else if (services.exportShotTo) {
        services.exportShotTo(path.Value(), resolution);
    }
}

void RequestExportPdfPath(AppState& state, DispatchServices& services) {
    const std::string name = state.projectName + "-storyboard.pdf";
    if (!services.savePdfFile) {
        return;
    }
    auto path = services.savePdfFile(name);
    if (!path.IsOk()) {
        state.status = path.GetError().userMessage;
        return;
    }
    if (path.Value().empty()) {
        return;
    }
    if (Platform::Paths::Exists(path.Value())) {
        state.exportPendingPath = path.Value();
        state.exportPendingPdf = true;
        state.exportPendingBoard = false;
        state.exportPendingPackage = false;
        state.exportOverwritePrompt = true;
        return;
    }
    if (services.exportBoardPdfTo) {
        services.exportBoardPdfTo(path.Value());
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
            RequestExportPath(state, services, false, false, resolution,
                              state.script.SelectedShotId());
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ExportShotPackageCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        Export::ShotResolution resolution = Export::ShotResolution::Hd1080;
        const std::string resolutionId =
            typed->resolutionId.empty() ? state.exportResolutionId : typed->resolutionId;
        if (shotId.empty()) {
            state.status = "没有可导出的镜头";
        } else if (!Export::TryParseResolution(resolutionId, resolution)) {
            state.status = "未知导出分辨率";
        } else {
            RequestExportPath(state, services, false, true, resolution, shotId);
        }
        return true;
    }
    if (std::holds_alternative<Core::ExportStoryboardBoardCommand>(command)) {
        state.exportPendingPdf = false;
        const Storyboard::PreviewIssueCount issues = state.storyboard.CountExportPreviewIssues();
        if (issues.Total() > 0) {
            state.exportStalePrompt = true;
            state.exportStaleCount = issues.Total();
        } else {
            RequestExportPath(state, services, true, false, Export::ShotResolution::Hd1080, {});
        }
        return true;
    }
    if (std::holds_alternative<Core::ExportStoryboardPdfCommand>(command)) {
        state.exportPendingPdf = true;
        const Storyboard::PreviewIssueCount issues = state.storyboard.CountExportPreviewIssues();
        if (issues.Total() > 0) {
            state.exportStalePrompt = true;
            state.exportStaleCount = issues.Total();
        } else {
            RequestExportPdfPath(state, services);
        }
        return true;
    }
    if (std::holds_alternative<Core::ConfirmStoryboardStaleExportCommand>(command)) {
        state.exportStalePrompt = false;
        state.exportStaleCount = 0;
        if (state.exportPendingPdf) {
            RequestExportPdfPath(state, services);
        } else {
            RequestExportPath(state, services, true, false, Export::ShotResolution::Hd1080, {});
        }
        return true;
    }
    if (std::holds_alternative<Core::CancelStoryboardStaleExportCommand>(command)) {
        state.exportStalePrompt = false;
        state.exportStaleCount = 0;
        state.exportPendingPdf = false;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetExportTransparentCommand>(&command)) {
        state.exportTransparent = typed->transparent;
        state.userSettings.exportTransparent = typed->transparent;
        PersistUserSettings(state);
        return true;
    }
    if (std::holds_alternative<Core::ConfirmExportOverwriteCommand>(command)) {
        state.exportOverwritePrompt = false;
        if (state.exportPendingPdf) {
            if (services.exportBoardPdfTo) {
                services.exportBoardPdfTo(state.exportPendingPath);
            }
        } else if (state.exportPendingBoard) {
            if (services.exportBoardTo) {
                services.exportBoardTo(state.exportPendingPath);
            }
        } else if (state.exportPendingPackage) {
            if (services.exportShotPackageTo) {
                services.exportShotPackageTo(state.exportPendingPath, state.exportPendingResolution,
                                             state.exportPendingShotId);
            }
        } else if (services.exportShotTo) {
            services.exportShotTo(state.exportPendingPath, state.exportPendingResolution);
        }
        state.exportPendingPath.clear();
        state.exportPendingPackage = false;
        state.exportPendingBoard = false;
        state.exportPendingPdf = false;
        state.exportPendingShotId.clear();
        return true;
    }
    if (std::holds_alternative<Core::CancelExportOverwriteCommand>(command)) {
        state.exportOverwritePrompt = false;
        state.exportPendingPath.clear();
        state.exportPendingPackage = false;
        state.exportPendingBoard = false;
        state.exportPendingPdf = false;
        state.exportPendingShotId.clear();
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectExportResolutionCommand>(&command)) {
        Export::ShotResolution parsed = Export::ShotResolution::Hd1080;
        if (!Export::TryParseResolution(typed->resolutionId, parsed)) {
            state.status = "未知导出分辨率";
        } else {
            state.exportResolutionId = typed->resolutionId;
            state.userSettings.exportResolutionId = typed->resolutionId;
            PersistUserSettings(state);
            state.status = std::string("导出分辨率 ") + state.exportResolutionId;
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
