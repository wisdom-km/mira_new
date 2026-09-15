// DispatchWorkspace: Workspace / storyboard / viewport Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"

#include <variant>

namespace DirectorDesk::App {

bool TryDispatchWorkspace(AppState& state, const Core::Command& command,
                          DispatchServices& services) {
    if (const auto* typed = std::get_if<Core::ViewportResizeCommand>(&command)) {
        if (services.renderer != nullptr) {
            services.renderer->SetViewportSize(typed->width, typed->height);
        }
        state.viewportWidth = typed->width;
        state.viewportHeight = typed->height;
        return true;
    }
    if (std::holds_alternative<Core::ExportTestPngCommand>(command)) {
        if (services.renderer != nullptr && state.cameras.Selected() != nullptr) {
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            if (services.framebufferSize) {
                services.framebufferSize(&width, &height);
            }
            HandleExportTestPng(*services.renderer, state.cameras.Selected()->orbit,
                                BuildSceneView(state.scene, state.cameras.CurrentLight(), false),
                                width, height, state.status);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetStoryboardSceneCollapsedCommand>(&command)) {
        state.storyboard.SetCollapsed(typed->sceneId, typed->collapsed);
        state.collapsedScenes = state.storyboard.CollapsedScenes();
        state.projectDirty = true;
        return true;
    }
    if (std::holds_alternative<Core::FocusStoryboardSelectionCommand>(command)) {
        state.status = "已聚焦当前镜头";
        return true;
    }
    if (std::holds_alternative<Core::FitStoryboardCommand>(command)) {
        state.status = "已适配分镜画布";
        return true;
    }
    if (const auto* typed = std::get_if<Core::RefreshStoryboardThumbnailCommand>(&command)) {
        const std::string shotId =
            typed->shotId.empty() ? state.script.SelectedShotId() : typed->shotId;
        if (!shotId.empty()) {
            state.storyboard.MarkShotStale(shotId);
            state.thumbScheduler.NotifyBusy(0);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ReportStoryboardViewCommand>(&command)) {
        state.storyboardViewPanX = typed->panX;
        state.storyboardViewPanY = typed->panY;
        state.storyboardViewZoom = typed->zoom > 0.0f ? typed->zoom : 1.0f;
        state.storyboardViewWidth = typed->width;
        state.storyboardViewHeight = typed->height;
        state.storyboard.SetCanvasWidth(typed->width);
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetWorkspaceModeCommand>(&command)) {
        if (!IsWorkspaceModeId(typed->modeId)) {
            state.status = "未知工作区模式";
        } else {
            state.workspaceModeId = typed->modeId;
            state.layoutRebuildRequested = true;
            state.status = std::string("已切换到") + WorkspaceModeLabel(state.workspaceModeId);
        }
        return true;
    }
    if (std::holds_alternative<Core::ResetLayoutCommand>(command)) {
        state.layoutRebuildRequested = true;
        state.status = "已重置布局";
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
