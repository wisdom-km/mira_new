// DispatchProject: Project / quit Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"

#include <utility>
#include <variant>

namespace DirectorDesk::App {

void ContinuePendingProjectAction(AppState& state, DispatchServices& services) {
    const PendingProjectAction action = state.pendingAction;
    const std::string openPath = state.pendingOpenPath;
    state.pendingAction = PendingProjectAction::None;
    state.pendingOpenPath.clear();
    if (action == PendingProjectAction::New) {
        if (services.renderer == nullptr) {
            return;
        }
        ResetProject(state.scene, state.cameras, state.links, state.script, *services.renderer,
                     state.projectId, state.projectName, state.projectPath, state.collapsedScenes,
                     state.projectDirty);
        BeginProjectGeneration(state);
        state.storyboard.Clear();
        state.selectedLibraryAssetId.clear();
        state.selectionKind = "none";
        state.selectionId.clear();
        state.selectionLabel.clear();
        RefreshBoard(state);
        state.status = "已新建工程";
    } else if (action == PendingProjectAction::Open) {
        if (services.renderer == nullptr) {
            return;
        }
        OpenProjectAt(openPath, state, services.renderer, services.worker, services.loadResults);
        state.storyboard.Clear();
        RefreshBoard(state);
        SelectFirstShotIfNone(state);
    } else if (action == PendingProjectAction::Quit) {
        if (services.requestClose) {
            services.requestClose();
        }
    }
}

namespace {

void RequestIfDirty(AppState& state, DispatchServices& services, PendingProjectAction action,
                    std::string openPath = {}) {
    state.pendingAction = action;
    state.pendingOpenPath = std::move(openPath);
    if (!state.projectDirty) {
        ContinuePendingProjectAction(state, services);
    }
}

void RequestSave(AppState& state, DispatchServices& services, const std::string& path,
                   bool proceedIfSaved) {
    const SaveProjectStatus status =
        RequestSaveProject(state, path, services.worker, services.hashResults);
    if (proceedIfSaved && status == SaveProjectStatus::Saved) {
        ContinuePendingProjectAction(state, services);
    } else if (proceedIfSaved && status == SaveProjectStatus::Deferred) {
        state.proceedAfterSave = true;
    }
}

} // namespace

bool TryDispatchProject(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (std::holds_alternative<Core::QuitCommand>(command)) {
        RequestIfDirty(state, services, PendingProjectAction::Quit);
        return true;
    }
    if (std::holds_alternative<Core::NewProjectCommand>(command)) {
        RequestIfDirty(state, services, PendingProjectAction::New);
        return true;
    }
    if (std::holds_alternative<Core::OpenProjectCommand>(command)) {
        if (!services.openProjectFile) {
            return true;
        }
        auto path = services.openProjectFile();
        if (!path.IsOk()) {
            state.status = path.GetError().userMessage;
        } else if (!path.Value().empty()) {
            RequestIfDirty(state, services, PendingProjectAction::Open, path.Value());
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::OpenProjectFromPathCommand>(&command)) {
        RequestIfDirty(state, services, PendingProjectAction::Open, typed->utf8Path);
        return true;
    }
    if (std::holds_alternative<Core::SaveProjectCommand>(command)) {
        state.collapsedScenes = state.storyboard.CollapsedScenes();
        if (state.projectPath.empty()) {
            if (!services.saveProjectFile) {
                return true;
            }
            auto path = services.saveProjectFile();
            if (path.IsOk() && !path.Value().empty()) {
                RequestSave(state, services, path.Value(), false);
            }
        } else {
            RequestSave(state, services, state.projectPath, false);
        }
        return true;
    }
    if (std::holds_alternative<Core::SaveProjectAsCommand>(command)) {
        state.collapsedScenes = state.storyboard.CollapsedScenes();
        if (!services.saveProjectFile) {
            return true;
        }
        auto path = services.saveProjectFile();
        if (path.IsOk() && !path.Value().empty()) {
            RequestSave(state, services, path.Value(), false);
        }
        return true;
    }
    if (std::holds_alternative<Core::ConfirmSaveProjectCommand>(command)) {
        state.collapsedScenes = state.storyboard.CollapsedScenes();
        if (state.projectPath.empty()) {
            if (!services.saveProjectFile) {
                return true;
            }
            auto path = services.saveProjectFile();
            if (path.IsOk() && !path.Value().empty()) {
                RequestSave(state, services, path.Value(), true);
            }
        } else {
            RequestSave(state, services, state.projectPath, true);
        }
        return true;
    }
    if (std::holds_alternative<Core::DiscardProjectCommand>(command)) {
        state.projectDirty = false;
        state.proceedAfterSave = false;
        ContinuePendingProjectAction(state, services);
        return true;
    }
    if (std::holds_alternative<Core::CancelProjectPromptCommand>(command)) {
        state.pendingAction = PendingProjectAction::None;
        state.pendingOpenPath.clear();
        state.proceedAfterSave = false;
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
