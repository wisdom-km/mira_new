// DispatchCamera: Camera / orbit / link Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Camera/Presets.h"

#include <variant>

namespace DirectorDesk::App {

bool TryDispatchCamera(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (const auto* typed = std::get_if<Core::OrbitDeltaCommand>(&command)) {
        if (Camera::CameraRig* rig = state.cameras.Selected()) {
            rig->orbit.Rotate(typed->rotateYaw, typed->rotatePitch);
            rig->orbit.Pan(typed->panX, typed->panY);
            rig->orbit.Zoom(typed->zoom);
            state.projectDirty = true;
            state.storyboard.MarkCameraShotsStale(state.cameras.SelectedId());
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ApplyCameraPresetCommand>(&command)) {
        Camera::CameraPresetKind kind = Camera::CameraPresetKind::Front;
        if (Camera::TryParseCameraPreset(typed->presetId, kind)) {
            state.cameras.ApplyPreset(kind, SubjectFromScene(state.scene));
            state.projectDirty = true;
            state.storyboard.MarkCameraShotsStale(state.cameras.SelectedId());
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
            state.status = std::string("已应用机位 ") + typed->presetId;
        } else {
            state.status = "未知机位预设";
        }
        return true;
    }
    if (std::holds_alternative<Core::AddCameraCommand>(command)) {
        state.cameras.Add();
        state.projectDirty = true;
        state.status = "已添加 " + state.cameras.Selected()->name;
        return true;
    }
    if (const auto* typed = std::get_if<Core::RemoveCameraCommand>(&command)) {
        if (!state.cameras.Remove(typed->cameraId)) {
            state.status = "至少需要保留一台相机";
        } else {
            state.projectDirty = true;
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::RenameCameraCommand>(&command)) {
        state.cameras.Rename(typed->cameraId, typed->name);
        state.projectDirty = true;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectCameraCommand>(&command)) {
        state.cameras.Select(typed->cameraId);
        state.selectedLibraryAssetId.clear();
        state.selectionKind = "camera";
        state.selectionId = typed->cameraId;
        state.selectionLabel = typed->cameraId;
        if (const Camera::CameraRig* rig = state.cameras.Find(typed->cameraId)) {
            state.selectionLabel = rig->name;
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetLightPresetCommand>(&command)) {
        Camera::LightPresetKind kind = Camera::LightPresetKind::Neutral;
        if (Camera::TryParseLightPreset(typed->presetId, kind)) {
            state.cameras.SetLightPreset(kind);
            state.projectDirty = true;
            state.storyboard.MarkLinkedStale();
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
            state.status = std::string("灯光预设 ") + typed->presetId;
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::LinkShotToCameraCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        const std::string cameraId =
            typed->cameraId.empty() ? state.cameras.SelectedId() : typed->cameraId;
        if (shotId.empty() || cameraId.empty()) {
            state.status = "请先选择镜头和相机";
        } else {
            state.links.Set(shotId, cameraId);
            state.projectDirty = true;
            state.storyboard.MarkShotStale(shotId);
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
            state.status = "已关联镜头与相机";
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::UnlinkShotCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        state.links.ClearShot(shotId);
        state.projectDirty = true;
        state.status = "已取消镜头关联";
        return true;
    }
    if (const auto* typed = std::get_if<Core::BindShotToNewCameraCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        if (shotId.empty()) {
            state.status = "请先选择镜头";
        } else {
            Camera::CameraRig& camera = state.cameras.Add();
            state.links.Set(shotId, camera.id);
            state.projectDirty = true;
            state.storyboard.MarkShotStale(shotId);
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
            state.script.SelectShot(shotId);
            state.storyboard.SetSelectedShot(shotId);
            state.selectionKind = "shot";
            state.selectionId = shotId;
            state.selectionLabel = FindShotTitle(state.script, shotId);
            state.status = "已为 " + state.selectionLabel + " 新建并绑定 " + camera.name;
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
