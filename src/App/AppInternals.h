// AppInternals: Shared helpers used by Application, Dispatch, and ViewStateBuilder.
#pragma once

#include "DirectorDesk/App/UserSettings.h"
#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Asset/LoaderRegistry.h"
#include "DirectorDesk/Asset/ModelLoadResult.h"
#include "DirectorDesk/Asset/OfficialCatalog.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Platform/IHttpClient.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/Renderer/IRenderer.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"
#include "DirectorDesk/Storyboard/Document.h"
#include "DirectorDesk/UI/IPanel.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::App {

struct LaunchOptions {
    bool exportAndQuit = false;
    std::string importPath;
    std::string scriptPath;
    std::string projectPath;
    std::string workspaceMode;
};

LaunchOptions ParseOptions(int argc, char** argv);
Asset::OfficialEndpoints MakeOfficialEndpoints();
std::uint64_t NowMs();
bool IsWorkspaceModeId(const std::string& modeId);
const char* WorkspaceModeLabel(const std::string& modeId);
std::string FindShotTitle(const Script::Document& script, const std::string& shotId);
std::string LibraryAssetLabel(const Asset::Library& library,
                              const Asset::OfficialCatalog& officialCatalog,
                              const std::string& assetId);
Camera::SubjectFrame SubjectFromScene(const Scene::Document& scene);
Renderer::GpuModelDesc ToGpuModel(const Asset::ModelData& model);
Renderer::RenderSceneView BuildSceneView(const Scene::Document& scene,
                                          const Camera::LightState& light, bool showGroundGrid,
                                          bool showGroundAxes = false);
UI::UiPreferences ToUiPreferences(const UserSettings& settings);
UserSettings FromUiPreferences(const UI::UiPreferences& preferences);
const char* PreviewText(Storyboard::PreviewStatus status);
const char* DiagnosticSeverityText(Script::DiagnosticSeverity severity);
void PushExportLog(std::vector<UI::ExportLogView>& log, UI::ExportLogView entry);
void SubmitImport(AppState& state, Platform::Worker& worker,
                   Core::ResultQueue<Asset::ModelLoadResult>& results, const std::string& path);
void ApplyLoadedModel(AppState& state, Renderer::IRenderer& renderer, Asset::ModelLoadResult result);
void RetainGpuModel(AppState& state, std::uint32_t modelId);
void ReleaseGpuModel(AppState& state, Renderer::IRenderer* renderer, std::uint32_t modelId);
void IndexLibraryPath(Asset::Library& library, const std::string& path, Asset::AssetOrigin origin);
void IndexReadyOfficial(Asset::OfficialCatalog& catalog, Asset::Library& library);
void BeginProjectGeneration(AppState& state);
void QueueSceneModelLoads(AppState& state, Platform::Worker& worker,
                            Core::ResultQueue<Asset::ModelLoadResult>& results);
void DrainLoadResults(AppState& state, Renderer::IRenderer* renderer,
                       Core::ResultQueue<Asset::ModelLoadResult>& loadResults);
void ResetProject(Scene::Document& scene, Camera::CameraManager& cameras, Link::Table& links,
                   Script::Document& script, Renderer::IRenderer& renderer, std::string& projectId,
                   std::string& projectName, std::string& projectPath,
                   std::vector<std::string>& collapsedScenes, bool& projectDirty);
enum class SaveProjectStatus { Failed, Saved, Deferred };
bool FinishSaveProject(AppState& state, const std::string& path);
SaveProjectStatus RequestSaveProject(AppState& state, const std::string& path,
                                       Platform::Worker* worker,
                                       Core::ResultQueue<SaveHashJobResult>* hashResults);
void DrainSaveHashResults(AppState& state, Core::ResultQueue<SaveHashJobResult>& hashResults);
bool OpenProjectAt(const std::string& path, AppState& state, Renderer::IRenderer* renderer,
                    Platform::Worker* worker, Core::ResultQueue<Asset::ModelLoadResult>* loadResults);
void ApplyScriptLoad(Script::Document& script, const std::string& path, std::string& status);
void HandleSaveScript(Script::Document& script, std::string& status);
bool HandleExportTestPng(Renderer::IRenderer& renderer, const Camera::OrbitCamera& camera,
                          const Renderer::RenderSceneView& sceneView, std::uint32_t windowWidth,
                          std::uint32_t windowHeight, std::string& status);
Storyboard::StoryboardSourceSnapshot MakeBoardSource(const Script::Document& script,
                                                     const Link::Table& links,
                                                     const Camera::CameraManager& cameras,
                                                     const std::vector<std::string>& collapsed,
                                                     const std::string& projectId);
void SyncLibraryPreviewTextures(Renderer::IRenderer& renderer,
                                 std::vector<UI::LibraryAssetView>& views,
                                 const std::unordered_map<std::string, std::string>& paths,
                                 std::unordered_map<std::string, LibraryPreviewGpu>& gpu,
                                 std::unordered_map<std::string, std::string>& failed);
bool ExportShotPng(AppState& state, Renderer::IRenderer& renderer, std::uint32_t fbW,
                    std::uint32_t fbH, const std::string& path, Export::ShotResolution resolution);
bool ExportBoardPng(AppState& state, const std::string& path);
void TickStoryboardGpu(AppState& state, Renderer::IRenderer& renderer);
void MaybeRequestStoryboardThumbnail(AppState& state, Renderer::IRenderer& renderer);
void DestroyLibraryPreviewTextures(AppState& state, Renderer::IRenderer& renderer);
void SubmitOfficialRefresh(AppState& state, Platform::Worker& worker,
                             Platform::IHttpClient* http,
                             Core::ResultQueue<OfficialRefreshResult>& results);
void SubmitOfficialDownload(AppState& state, Platform::Worker& worker,
                              Platform::IHttpClient* http, const std::string& assetId,
                              Core::ResultQueue<OfficialDownloadJobResult>& downloadResults,
                              Core::ResultQueue<OfficialProgressUpdate>& progressResults);
void DrainAppQueues(AppState& state, Renderer::IRenderer& renderer,
                      Core::ResultQueue<Asset::ModelLoadResult>& loadResults,
                      Core::ResultQueue<OfficialRefreshResult>& officialRefreshResults,
                      Core::ResultQueue<OfficialDownloadJobResult>& officialDownloadResults,
                      Core::ResultQueue<OfficialProgressUpdate>& officialProgressResults,
                      Core::ResultQueue<SaveHashJobResult>& hashResults);

} // namespace DirectorDesk::App
