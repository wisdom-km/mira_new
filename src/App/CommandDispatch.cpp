// CommandDispatch: Command visit entry (FND-10 / CR-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"

#include <utility>
#include <variant>

namespace DirectorDesk::App {

void RecordShotSelection(AppState& state, const std::string& shotId) {
    state.script.SelectShot(shotId);
    state.storyboard.SetSelectedShot(shotId);
    state.selectedLibraryAssetId.clear();
    state.selectionKind = "shot";
    state.selectionId = shotId;
    state.selectionLabel = FindShotTitle(state.script, shotId);
    if (const std::string* cameraId = state.links.CameraForShot(shotId)) {
        if (state.cameras.Find(*cameraId) != nullptr) {
            state.cameras.Select(*cameraId);
        } else {
            state.status = "关联相机已不存在";
        }
    }
}

void SelectFirstShotIfNone(AppState& state) {
    if (!state.script.SelectedShotId().empty()) {
        return;
    }
    const std::string shotId = state.script.FirstShotId();
    if (!shotId.empty()) {
        RecordShotSelection(state, shotId);
    }
}

void RefreshBoard(AppState& state) {
    state.storyboard.ApplySource(MakeBoardSource(state.script, state.links, state.cameras,
                                                 state.collapsedScenes, state.projectId));
    state.collapsedScenes = state.storyboard.CollapsedScenes();
}

void Dispatch(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (TryDispatchWorkspace(state, command, services) ||
        TryDispatchProject(state, command, services) ||
        TryDispatchScript(state, command, services) ||
        TryDispatchScene(state, command, services) ||
        TryDispatchCamera(state, command, services) ||
        TryDispatchLibrary(state, command, services) ||
        TryDispatchExport(state, command, services)) {
        return;
    }
}

} // namespace DirectorDesk::App
