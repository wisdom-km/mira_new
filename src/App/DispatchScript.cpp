// DispatchScript: Script Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Core/Log.h"

#include <variant>

namespace DirectorDesk::App {

bool TryDispatchScript(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (std::holds_alternative<Core::LoadScriptCommand>(command)) {
        if (!services.openMarkdownFile) {
            return true;
        }
        auto path = services.openMarkdownFile();
        if (!path.IsOk()) {
            state.status = path.GetError().userMessage;
            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
        } else if (!path.Value().empty()) {
            ApplyScriptLoad(state.script, path.Value(), state.status);
            state.projectDirty = true;
            SelectFirstShotIfNone(state);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::LoadScriptFromPathCommand>(&command)) {
        ApplyScriptLoad(state.script, typed->utf8Path, state.status);
        state.projectDirty = true;
        SelectFirstShotIfNone(state);
        return true;
    }
    if (std::holds_alternative<Core::SaveScriptCommand>(command)) {
        HandleSaveScript(state.script, state.status);
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetScriptTextCommand>(&command)) {
        state.script.SetText(typed->text);
        return true;
    }
    if (std::holds_alternative<Core::InsertSceneCommand>(command)) {
        state.script.InsertScene();
        state.status = "已添加场次";
        return true;
    }
    if (const auto* typed = std::get_if<Core::InsertShotCommand>(&command)) {
        state.script.InsertShot(typed->afterShotId);
        state.projectDirty = true;
        state.status = "已添加镜头";
        return true;
    }
    if (const auto* typed = std::get_if<Core::DeleteShotCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        if (shotId.empty() || !state.script.RemoveShot(shotId)) {
            state.status = "无法删除该镜头";
        } else {
            state.links.ClearShot(shotId);
            state.projectDirty = true;
            state.storyboard.SetSelectedShot(state.script.SelectedShotId());
            if (state.selectionId == shotId) {
                if (state.script.SelectedShotId().empty()) {
                    state.selectionKind = "none";
                    state.selectionId.clear();
                    state.selectionLabel.clear();
                } else {
                    state.selectionKind = "shot";
                    state.selectionId = state.script.SelectedShotId();
                    state.selectionLabel = FindShotTitle(state.script, state.selectionId);
                }
            }
            state.status = "已删除镜头";
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectShotCommand>(&command)) {
        RecordShotSelection(state, typed->shotId);
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectAdjacentShotCommand>(&command)) {
        const std::string shotId = state.script.AdjacentShotId(typed->delta);
        if (!shotId.empty()) {
            RecordShotSelection(state, shotId);
            state.status = std::string("已选中 ") + state.selectionLabel;
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
