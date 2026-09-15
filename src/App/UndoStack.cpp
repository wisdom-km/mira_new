// UndoStack: Dressing undo/redo for Scene / Camera / Link (FND-42).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

namespace DirectorDesk::App {
namespace {

constexpr std::size_t kUndoLimit = 20;

DressingSnapshot CaptureDressing(const AppState& state) {
    DressingSnapshot snap;
    snap.nodes = state.scene.Nodes();
    snap.selectedNodeId = state.scene.SelectedId();
    snap.cameras = state.cameras.Cameras();
    snap.selectedCameraId = state.cameras.SelectedId();
    snap.light = state.cameras.LightPreset();
    snap.links = state.links.All();
    snap.selectionKind = state.selectionKind;
    snap.selectionId = state.selectionId;
    snap.selectionLabel = state.selectionLabel;
    return snap;
}

bool GpuInHistory(const AppState& state, std::uint32_t id) {
    for (const DressingSnapshot& snap : state.undoStack) {
        for (const Scene::Node& node : snap.nodes) {
            if (node.gpuModelId == id) {
                return true;
            }
        }
    }
    for (const DressingSnapshot& snap : state.redoStack) {
        for (const Scene::Node& node : snap.nodes) {
            if (node.gpuModelId == id) {
                return true;
            }
        }
    }
    return false;
}

void SyncLiveGpuRefs(AppState& state, DispatchServices& services) {
    std::unordered_map<std::uint32_t, std::uint32_t> live;
    for (const Scene::Node& node : state.scene.Nodes()) {
        if (node.gpuModelId != 0) {
            ++live[node.gpuModelId];
        }
    }
    std::unordered_set<std::uint32_t> previous;
    for (const auto& item : state.gpuModelRefs) {
        previous.insert(item.first);
    }
    for (std::uint32_t id : previous) {
        if (live.find(id) == live.end() && !GpuInHistory(state, id) && services.renderer != nullptr) {
            services.renderer->DestroyModel(id);
        }
    }
    state.gpuModelRefs = std::move(live);
}

void ApplyDressing(AppState& state, DispatchServices& services, DressingSnapshot snap) {
    state.scene.ReplaceNodes(std::move(snap.nodes), snap.selectedNodeId);
    state.cameras.Replace(std::move(snap.cameras), snap.selectedCameraId, snap.light);
    state.links.Replace(std::move(snap.links));
    state.selectionKind = snap.selectionKind;
    state.selectionId = snap.selectionId;
    state.selectionLabel = snap.selectionLabel;
    SyncLiveGpuRefs(state, services);
    state.projectDirty = true;
    state.storyboard.MarkLinkedStale();
    if (services.nowMs) {
        state.thumbScheduler.NotifyBusy(services.nowMs());
    }
}

std::string CoalesceKey(const Core::Command& command) {
    if (const auto* typed = std::get_if<Core::SetNodeTransformCommand>(&command)) {
        return std::string("transform:") + typed->nodeId;
    }
    return {};
}

bool ShouldRecordUndo(const Core::Command& command) {
    return std::holds_alternative<Core::SetNodeTransformCommand>(command) ||
           std::holds_alternative<Core::DeleteNodeCommand>(command) ||
           std::holds_alternative<Core::DuplicateNodeCommand>(command) ||
           std::holds_alternative<Core::SetNodeVisibleCommand>(command) ||
           std::holds_alternative<Core::AddLibraryAssetToSceneCommand>(command) ||
           std::holds_alternative<Core::AddCameraCommand>(command) ||
           std::holds_alternative<Core::RemoveCameraCommand>(command) ||
           std::holds_alternative<Core::RenameCameraCommand>(command) ||
           std::holds_alternative<Core::ApplyCameraPresetCommand>(command) ||
           std::holds_alternative<Core::SetLightPresetCommand>(command) ||
           std::holds_alternative<Core::LinkShotToCameraCommand>(command) ||
           std::holds_alternative<Core::UnlinkShotCommand>(command) ||
           std::holds_alternative<Core::BindShotToNewCameraCommand>(command);
}

void TrimUndo(AppState& state) {
    while (state.undoStack.size() > kUndoLimit) {
        state.undoStack.erase(state.undoStack.begin());
    }
}

void RecordUndo(AppState& state, const Core::Command& command) {
    const std::string key = CoalesceKey(command);
    if (!key.empty() && key == state.undoCoalesceKey && !state.undoStack.empty()) {
        return;
    }
    state.undoStack.push_back(CaptureDressing(state));
    TrimUndo(state);
    state.redoStack.clear();
    state.undoCoalesceKey = key;
}

} // namespace

bool TryDispatchUndo(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (std::holds_alternative<Core::UndoCommand>(command)) {
        if (state.undoStack.empty()) {
            return true;
        }
        DressingSnapshot current = CaptureDressing(state);
        DressingSnapshot previous = std::move(state.undoStack.back());
        state.undoStack.pop_back();
        state.redoStack.push_back(std::move(current));
        ApplyDressing(state, services, std::move(previous));
        state.undoCoalesceKey.clear();
        state.status = "已撤销";
        return true;
    }
    if (std::holds_alternative<Core::RedoCommand>(command)) {
        if (state.redoStack.empty()) {
            return true;
        }
        DressingSnapshot current = CaptureDressing(state);
        DressingSnapshot next = std::move(state.redoStack.back());
        state.redoStack.pop_back();
        state.undoStack.push_back(std::move(current));
        TrimUndo(state);
        ApplyDressing(state, services, std::move(next));
        state.undoCoalesceKey.clear();
        state.status = "已重做";
        return true;
    }
    if (ShouldRecordUndo(command)) {
        RecordUndo(state, command);
    } else if (!std::holds_alternative<Core::OrbitDeltaCommand>(command) &&
               !std::holds_alternative<Core::ViewportResizeCommand>(command) &&
               !std::holds_alternative<Core::ReportStoryboardViewCommand>(command)) {
        state.undoCoalesceKey.clear();
    }
    return false;
}

} // namespace DirectorDesk::App
