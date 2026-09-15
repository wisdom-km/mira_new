// DispatchScript: Script Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Script/ImportStoryboard.h"

#include <algorithm>
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
        if (!state.script.SelectedShotId().empty()) {
            RecordShotSelection(state, state.script.SelectedShotId());
        }
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
    if (const auto* typed = std::get_if<Core::SetShotMetaCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        if (shotId.empty() || !state.script.SetShotMeta(shotId, typed->key, typed->value)) {
            state.status = "无法改写镜头元数据";
        } else {
            RefreshBoard(state);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ImportStoryboardCommand>(&command)) {
        if (!services.openJsonFile) {
            return true;
        }
        auto path = services.openJsonFile();
        if (!path.IsOk()) {
            state.status = path.GetError().userMessage;
        } else if (!path.Value().empty()) {
            ApplyStoryboardImport(state, path.Value(), typed->mode);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ImportStoryboardFromPathCommand>(&command)) {
        ApplyStoryboardImport(state, typed->utf8Path, typed->mode);
        return true;
    }
    return false;
}

void ApplyStoryboardImport(AppState& state, const std::string& utf8Path, const std::string& mode) {
    const std::string importMode = mode.empty() ? "append" : mode;
    if (importMode == "replace" && (state.projectDirty || state.script.IsDirty())) {
        state.pendingImportPath = utf8Path;
        state.pendingImportMode = importMode;
        state.pendingAction = PendingProjectAction::ImportStoryboard;
        return;
    }
    auto text = Platform::Paths::ReadTextFile(utf8Path);
    if (!text.IsOk()) {
        state.status = text.GetError().userMessage;
        state.importDiagnostics = {state.status};
        return;
    }
    const Script::Snapshot* existing =
        state.script.HasPublishedSnapshot() ? &state.script.PublishedSnapshot() : nullptr;
    const Script::StoryboardImportMode parsedMode = importMode == "replace"
                                                        ? Script::StoryboardImportMode::Replace
                                                        : Script::StoryboardImportMode::Append;
    auto imported = Script::ImportStoryboard(text.Value(), parsedMode, existing);
    if (!imported.IsOk()) {
        state.status = imported.GetError().userMessage;
        state.importDiagnostics = {state.status};
        return;
    }
    state.importDiagnostics = imported.Value().diagnostics;
    std::string markdown = imported.Value().markdown;
    if (parsedMode == Script::StoryboardImportMode::Append && !state.script.Text().empty()) {
        std::string current = state.script.Text();
        if (current.back() != '\n') {
            current += '\n';
        }
        current += '\n';
        current += markdown;
        markdown = std::move(current);
    }
    std::vector<std::string> previousShotIds;
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& scene : state.script.PublishedSnapshot().scenes) {
            for (const Script::Shot& shot : scene.shots) {
                previousShotIds.push_back(shot.id);
            }
        }
    }
    state.script.SetText(markdown);
    state.projectDirty = true;
    RefreshBoard(state);
    std::string newShotId;
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& scene : state.script.PublishedSnapshot().scenes) {
            for (const Script::Shot& shot : scene.shots) {
                if (std::find(previousShotIds.begin(), previousShotIds.end(), shot.id) ==
                    previousShotIds.end()) {
                    newShotId = shot.id;
                    break;
                }
            }
            if (!newShotId.empty()) {
                break;
            }
        }
    }
    if (!newShotId.empty()) {
        RecordShotSelection(state, newShotId);
    } else {
        SelectFirstShotIfNone(state);
    }
    state.status = "已导入 " + std::to_string(imported.Value().sceneCount) + " 场 " +
                   std::to_string(imported.Value().shotCount) + " 镜";
    if (!imported.Value().diagnostics.empty()) {
        const std::size_t shown = std::min<std::size_t>(imported.Value().diagnostics.size(), 3);
        for (std::size_t i = 0; i < shown; ++i) {
            state.status += "；";
            state.status += imported.Value().diagnostics[i];
        }
    }
}

} // namespace DirectorDesk::App
