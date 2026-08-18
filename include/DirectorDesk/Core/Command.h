#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace DirectorDesk::Core {

struct QuitCommand {};

struct ViewportResizeCommand {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct OrbitDeltaCommand {
    float rotateYaw = 0.0f;
    float rotatePitch = 0.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 0.0f;
};

struct ExportTestPngCommand {};

struct ImportModelCommand {};

struct ImportModelFromPathCommand {
    std::string utf8Path;
};

struct SelectNodeCommand {
    std::string nodeId;
};

struct SetNodeTransformCommand {
    std::string nodeId;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float eulerDegrees[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
};

struct LoadScriptCommand {};

struct LoadScriptFromPathCommand {
    std::string utf8Path;
};

struct SaveScriptCommand {};

struct SetScriptTextCommand {
    std::string text;
};

struct InsertSceneCommand {};

struct InsertShotCommand {};

struct SelectShotCommand {
    std::string shotId;
};

struct ApplyCameraPresetCommand {
    std::string presetId;
};

struct AddCameraCommand {};

struct RemoveCameraCommand {
    std::string cameraId;
};

struct RenameCameraCommand {
    std::string cameraId;
    std::string name;
};

struct SelectCameraCommand {
    std::string cameraId;
};

struct SetLightPresetCommand {
    std::string presetId;
};

struct AddLibraryAssetToSceneCommand {
    std::string assetId;
};

struct SetLibrarySearchCommand {
    std::string text;
};

struct SetLibraryOriginFilterCommand {
    std::string originFilter;
};

struct SetLibraryViewModeCommand {
    std::string viewMode;
};

struct SelectLibraryAssetCommand {
    std::string assetId;
};

struct RefreshLibraryCommand {};

struct RefreshOfficialCatalogCommand {};

struct DownloadOfficialAssetCommand {
    std::string assetId;
};

struct CancelOfficialDownloadCommand {
    std::string assetId;
};

struct SetOfficialCategoryCommand {
    std::string categoryId;
};

struct NewProjectCommand {};

struct OpenProjectCommand {};

struct OpenProjectFromPathCommand {
    std::string utf8Path;
};

struct SaveProjectCommand {};

struct SaveProjectAsCommand {};

struct LinkShotToCameraCommand {
    std::string shotId;
    std::string cameraId;
};

struct UnlinkShotCommand {
    std::string shotId;
};

struct ConfirmSaveProjectCommand {};

struct DiscardProjectCommand {};

struct CancelProjectPromptCommand {};

struct SetStoryboardSceneCollapsedCommand {
    std::string sceneId;
    bool collapsed = false;
};

struct FocusStoryboardSelectionCommand {};

struct FitStoryboardCommand {};

struct RefreshStoryboardThumbnailCommand {
    std::string shotId;
};

struct ExportCurrentShotCommand {
    std::string resolutionId = "1080p";
};

struct ExportStoryboardBoardCommand {};

struct SetExportTransparentCommand {
    bool transparent = true;
};

struct ConfirmExportOverwriteCommand {};

struct CancelExportOverwriteCommand {};

struct ReportStoryboardViewCommand {
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ConfirmStoryboardStaleExportCommand {};

struct CancelStoryboardStaleExportCommand {};

using Command = std::variant<
    QuitCommand, ViewportResizeCommand, OrbitDeltaCommand, ExportTestPngCommand, ImportModelCommand,
    ImportModelFromPathCommand, SelectNodeCommand, SetNodeTransformCommand, LoadScriptCommand,
    LoadScriptFromPathCommand, SaveScriptCommand, SetScriptTextCommand, InsertSceneCommand,
    InsertShotCommand, SelectShotCommand, ApplyCameraPresetCommand, AddCameraCommand,
    RemoveCameraCommand, RenameCameraCommand, SelectCameraCommand, SetLightPresetCommand,
    AddLibraryAssetToSceneCommand, SetLibrarySearchCommand, SetLibraryOriginFilterCommand,
    SetLibraryViewModeCommand, SelectLibraryAssetCommand, RefreshLibraryCommand,
    RefreshOfficialCatalogCommand, DownloadOfficialAssetCommand, CancelOfficialDownloadCommand,
    SetOfficialCategoryCommand, NewProjectCommand,
    OpenProjectCommand, OpenProjectFromPathCommand, SaveProjectCommand, SaveProjectAsCommand,
    LinkShotToCameraCommand, UnlinkShotCommand, ConfirmSaveProjectCommand, DiscardProjectCommand,
    CancelProjectPromptCommand, SetStoryboardSceneCollapsedCommand, FocusStoryboardSelectionCommand,
    FitStoryboardCommand, RefreshStoryboardThumbnailCommand, ExportCurrentShotCommand,
    ExportStoryboardBoardCommand, SetExportTransparentCommand, ConfirmExportOverwriteCommand,
    CancelExportOverwriteCommand, ReportStoryboardViewCommand, ConfirmStoryboardStaleExportCommand,
    CancelStoryboardStaleExportCommand>;

} // namespace DirectorDesk::Core
