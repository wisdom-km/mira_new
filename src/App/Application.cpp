// Application: Implementation for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/App/Application.h"

#include "AppInternals.h"
#include "DirectorDesk/App/CommandDispatch.h"
#include "DirectorDesk/App/UserSettings.h"
#include "DirectorDesk/App/ViewStateBuilder.h"
#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/CommandQueue.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Platform/FileDialog.h"
#include "DirectorDesk/Platform/IHttpClient.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Startup.h"
#include "DirectorDesk/Platform/Window.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/UI/LibraryPanel.h"
#include "DirectorDesk/UI/ScriptPanel.h"
#include "DirectorDesk/UI/StoryboardPanel.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include "CreateBgfxRenderer.h"
#include "CurlHttpClient.h"
#include "ImGuiGlfwBackend.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace DirectorDesk::App {

int Application::Run(int argc, char** argv) {
    Platform::InitializeProcess();
    const LaunchOptions options = ParseOptions(argc, argv);

    auto logDir = Platform::Paths::LogDirectory();
    if (!logDir.IsOk() || !Platform::Paths::CreateDirectories(logDir.Value()).IsOk() ||
        !Core::Log::Init(logDir.Value()).IsOk()) {
        Platform::ShutdownProcess();
        return 1;
    }
    DD_LOG_INFO("DirectorDesk starting");

    auto exeDir = Platform::Paths::ExecutableDirectory();
    if (!exeDir.IsOk()) {
        DD_LOG_ERROR("{}", exeDir.GetError().technicalMessage);
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }
    const std::string shaderDirectory = Platform::Paths::Join(exeDir.Value(), "shaders");

    Platform::Window window;
    if (!window.Create(Platform::WindowDesc{}).IsOk()) {
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }

    auto renderer = Backends::CreateBgfxRenderer();
    const auto framebuffer = window.GetFramebufferSize();
    Renderer::RendererInitDesc rendererDesc;
    rendererDesc.nativeWindowHandle = window.NativeOsHandle();
    rendererDesc.width = framebuffer.width;
    rendererDesc.height = framebuffer.height;
    rendererDesc.shaderDirectory = shaderDirectory;
    if (!renderer->Init(rendererDesc).IsOk()) {
        window.Destroy();
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }

    auto officialRoot = Platform::Paths::OfficialAssetsDirectory();
    const std::string officialCache = officialRoot.IsOk() ? officialRoot.Value() : std::string();
    if (!officialCache.empty()) {
        Platform::Paths::CreateDirectories(officialCache);
    }
    AppState state(officialCache);
    const std::string examples = Platform::Paths::Join(exeDir.Value(), "examples");
    const std::string models = Platform::Paths::Join(examples, "models");
    state.exampleObj = Platform::Paths::Join(models, "cube.obj");
    state.exampleGlb = Platform::Paths::Join(models, "cube.glb");
    state.exampleScript = Platform::Paths::Join(Platform::Paths::Join(examples, "scripts"), "cafe.md");
    state.exampleProject = Platform::Paths::Join(examples, "cafe.ddproj");
    state.exampleStoryboardImport = Platform::Paths::Join(examples, "storyboard-import.example.json");
    state.exampleSkill =
        Platform::Paths::Join(Platform::Paths::Join(Platform::Paths::Join(examples, "skills"),
                                                    "storyboard"),
                              "SKILL.md");

    auto libraryDir = Platform::Paths::LibraryDirectory();
    if (libraryDir.IsOk()) {
        auto opened = state.library.Open(libraryDir.Value());
        if (!opened.IsOk()) {
            DD_LOG_ERROR("{}", opened.GetError().technicalMessage);
        } else if (state.library.RecoveredFromCorruptIndex()) {
            DD_LOG_WARN("Library index was corrupt and was recovered");
        }
        IndexLibraryPath(state.library, state.exampleObj, Asset::AssetOrigin::Builtin);
        IndexLibraryPath(state.library, state.exampleGlb, Asset::AssetOrigin::Builtin);
    }
    std::unique_ptr<Platform::IHttpClient> http = Backends::CreateCurlHttpClient();
    state.officialCatalog.LoadCache();
    IndexReadyOfficial(state.officialCatalog, state.library);
    if (options.exportAndQuit && options.importPath.empty()) {
        const auto startupSize = window.GetFramebufferSize();
        const bool ok = HandleExportTestPng(
            *renderer, state.cameras.Selected()->orbit,
            BuildSceneView(state.scene, state.cameras.CurrentLight(), false), startupSize.width,
            startupSize.height, state.status);
        renderer->Shutdown();
        window.Destroy();
        DD_LOG_INFO("DirectorDesk exiting");
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return ok ? 0 : 1;
    }

    Backends::ImGuiGlfwBackend imgui;
    if (!imgui.Init(window, shaderDirectory, 255).IsOk()) {
        renderer->Shutdown();
        window.Destroy();
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }

    Platform::Worker worker;
    worker.Start();
    Core::ResultQueue<Asset::ModelLoadResult> loadResults;
    Core::ResultQueue<OfficialRefreshResult> officialRefreshResults;
    Core::ResultQueue<OfficialDownloadJobResult> officialDownloadResults;
    Core::ResultQueue<OfficialProgressUpdate> officialProgressResults;
    Core::ResultQueue<SaveHashJobResult> hashResults;
    Core::ResultQueue<AiJobResult> aiResults;
    UI::WorkspacePanel workspace;
    UI::ScriptPanel scriptPanel;
    UI::LibraryPanel libraryPanel;
    UI::StoryboardPanel storyboardPanel;
    Core::CommandQueue commands;
    UI::AppViewState viewState;
    FrameStrings frame;

    SubmitOfficialRefresh(state, worker, http.get(), officialRefreshResults);

    if (!options.importPath.empty()) {
        SubmitImport(state, worker, loadResults, options.importPath);
    }
    if (!options.scriptPath.empty()) {
        ApplyScriptLoad(state.script, options.scriptPath, state.status);
    }
    if (!options.projectPath.empty()) {
        OpenProjectAt(options.projectPath, state, renderer.get(), &worker, &loadResults);
    }
    SelectFirstShotIfNone(state);
    std::string userSettingsPath;
    bool canPersistSettings = false;
    if (auto settingsPath = Platform::Paths::UserSettingsFile(); settingsPath.IsOk()) {
        userSettingsPath = settingsPath.Value();
        state.userSettingsPath = userSettingsPath;
        if (Platform::Paths::Exists(userSettingsPath)) {
            auto loaded = LoadUserSettings(userSettingsPath);
            if (loaded.IsOk()) {
                state.userSettings = loaded.Value();
                canPersistSettings = true;
            } else {
                DD_LOG_WARN("{}", loaded.GetError().technicalMessage);
            }
        } else {
            canPersistSettings = true;
        }
    }
    const auto resolveSize = window.GetFramebufferSize();
    const unsigned resolveWidth =
        std::max(static_cast<unsigned>(resolveSize.width), imgui.PrimaryMonitorWidth());
    const float resolvedScale =
        ResolveUiScale(state.userSettings.uiScale, resolveWidth, imgui.ContentScale());
    const bool persistResolvedDefault = state.userSettings.uiScale <= 0.0f;
    state.userSettings.uiScale = resolvedScale;
    workspace.ApplyPreferences(ToUiPreferences(state.userSettings));
    imgui.ApplyUiScale(resolvedScale);
    float appliedUiScale = resolvedScale;
    if (persistResolvedDefault && canPersistSettings && !userSettingsPath.empty()) {
        auto saved = SaveUserSettings(userSettingsPath, state.userSettings);
        if (!saved.IsOk()) {
            DD_LOG_WARN("{}", saved.GetError().technicalMessage);
        }
    }
    state.exportResolutionId =
        state.userSettings.exportResolutionId == "2k" ? "2k" : "1080p";
    state.exportTransparent = state.userSettings.exportTransparent;
    if (options.projectPath.empty() && state.userSettings.openLastProject &&
        !state.userSettings.lastProjectPath.empty() &&
        Platform::Paths::Exists(state.userSettings.lastProjectPath)) {
        OpenProjectAt(state.userSettings.lastProjectPath, state, renderer.get(), &worker,
                      &loadResults);
        SelectFirstShotIfNone(state);
    }
    if (IsWorkspaceModeId(options.workspaceMode)) {
        state.workspaceModeId = options.workspaceMode;
        state.layoutRebuildRequested = true;
    }
    RefreshBoard(state);

    DispatchServices services;
    services.renderer = renderer.get();
    services.worker = &worker;
    services.loadResults = &loadResults;
    services.hashResults = &hashResults;
    services.nowMs = []() { return NowMs(); };
    services.requestClose = [&]() { window.RequestClose(); };
    services.framebufferSize = [&](std::uint32_t* width, std::uint32_t* height) {
        const auto size = window.GetFramebufferSize();
        if (width != nullptr) {
            *width = size.width;
        }
        if (height != nullptr) {
            *height = size.height;
        }
    };
    services.submitImport = [&](const std::string& path) {
        SubmitImport(state, worker, loadResults, path);
    };
    services.submitOfficialRefresh = [&]() {
        SubmitOfficialRefresh(state, worker, http.get(), officialRefreshResults);
    };
    services.submitOfficialDownload = [&](const std::string& assetId) {
        SubmitOfficialDownload(state, worker, http.get(), assetId, officialDownloadResults,
                               officialProgressResults);
    };
    services.openModelFile = []() { return Platform::FileDialog::OpenModelFile(); };
    services.openMarkdownFile = []() { return Platform::FileDialog::OpenMarkdownFile(); };
    services.openJsonFile = []() { return Platform::FileDialog::OpenJsonFile(); };
    services.openProjectFile = []() { return Platform::FileDialog::OpenProjectFile(); };
    services.saveProjectFile = []() { return Platform::FileDialog::SaveProjectFile(); };
    services.savePngFile = [](const std::string& name) {
        return Platform::FileDialog::SavePngFile(name);
    };
    services.exportShotTo = [&](const std::string& path, Export::ShotResolution resolution) {
        const auto windowSize = window.GetFramebufferSize();
        return ExportShotPng(state, *renderer, windowSize.width, windowSize.height, path,
                             resolution);
    };
    services.exportShotPackageTo = [&](const std::string& path, Export::ShotResolution resolution,
                                       const std::string& shotId) {
        const auto windowSize = window.GetFramebufferSize();
        return ExportShotPackage(state, *renderer, windowSize.width, windowSize.height, path,
                                 resolution, shotId);
    };
    services.exportBoardTo = [&](const std::string& path) { return ExportBoardPng(state, path); };
    services.exportBoardPdfTo = [&](const std::string& path) { return ExportBoardPdf(state, path); };
    services.savePdfFile = [](const std::string& name) {
        return Platform::FileDialog::SavePdfFile(name);
    };
    services.http = http.get();
    services.aiResults = &aiResults;

    while (true) {
        window.PollEvents();
        if (window.ShouldClose()) {
            if (state.projectDirty && state.pendingAction != PendingProjectAction::Quit) {
                window.CancelClose();
                state.pendingAction = PendingProjectAction::Quit;
            } else {
                break;
            }
        }

        DrainAppQueues(state, *renderer, loadResults, officialRefreshResults, officialDownloadResults,
                        officialProgressResults, hashResults, &aiResults);
        if (!state.projectSaveInProgress && state.projectSaveQueued) {
            state.projectSaveQueued = false;
            RequestSaveProject(state, state.projectSavePendingPath, &worker, &hashResults);
        }
        if (state.proceedAfterSave && !state.projectSaveInProgress) {
            state.proceedAfterSave = false;
            ContinuePendingProjectAction(state, services);
        }

        Core::Command command;
        while (commands.TryPop(command)) {
            Dispatch(state, command, services);
        }

        BuildViewState(state, viewState, frame);
        SyncLibraryPreviewTextures(*renderer, frame.libraryAssets, frame.libraryPreviewPaths,
                                   state.libraryPreviewGpu, state.libraryPreviewFailed);
        const auto size = window.GetFramebufferSize();
        viewState.windowWidth = size.width;
        viewState.windowHeight = size.height;
        viewState.viewportTextureIndex = renderer->ViewportTextureIndex();
        viewState.viewportTextureWidth = renderer->ViewportWidth();
        viewState.viewportTextureHeight = renderer->ViewportHeight();

        TickStoryboardGpu(state, *renderer);

        const float aspect = renderer->ViewportHeight() == 0
                                 ? 1.0f
                                 : static_cast<float>(renderer->ViewportWidth()) /
                                       static_cast<float>(renderer->ViewportHeight());
        renderer->BeginFrame(size.width, size.height);
        const UI::UiPreferences prefs = workspace.Preferences();
        Renderer::RenderTargetDesc viewportTarget;
        viewportTarget.opaqueClearRgba = ViewportClearRgba(prefs.viewportBackground);
        renderer->RenderScene(BuildSceneView(state.scene, state.cameras.CurrentLight(),
                                               prefs.showGroundGrid, prefs.showGroundAxes,
                                               prefs.uiScale),
                              state.cameras.Selected()->orbit.BuildView(aspect), viewportTarget);
        MaybeRequestStoryboardThumbnail(state, *renderer);
        imgui.BeginFrame();
        workspace.Draw(viewState, commands);
        if (workspace.ConsumePreferencesDirty()) {
            UserSettings next = FromUiPreferences(workspace.Preferences());
            next.aiProvider = state.userSettings.aiProvider;
            next.aiBaseUrl = state.userSettings.aiBaseUrl;
            next.aiApiKey = state.userSettings.aiApiKey;
            next.aiImageModel = state.userSettings.aiImageModel;
            next.aiVideoModel = state.userSettings.aiVideoModel;
            next.aiChatModel = state.userSettings.aiChatModel;
            next.lastProjectPath = state.userSettings.lastProjectPath;
            next.exportResolutionId = state.userSettings.exportResolutionId;
            next.exportTransparent = state.userSettings.exportTransparent;
            state.userSettings = next;
            if (std::fabs(next.uiScale - appliedUiScale) > 0.01f) {
                imgui.ApplyUiScale(next.uiScale);
                appliedUiScale = next.uiScale;
            }
            if (!userSettingsPath.empty()) {
                auto saved = SaveUserSettings(userSettingsPath, state.userSettings);
                if (!saved.IsOk()) {
                    DD_LOG_WARN("{}", saved.GetError().technicalMessage);
                }
            }
        }
        scriptPanel.Draw(viewState, commands);
        libraryPanel.Draw(viewState, commands);
        storyboardPanel.Draw(viewState, commands);
        imgui.Submit(size.width, size.height);
        renderer->EndFrame();
        state.layoutRebuildRequested = false;
    }

    worker.Shutdown();
    imgui.Shutdown();
    DestroyLibraryPreviewTextures(state, *renderer);
    renderer->Shutdown();
    window.Destroy();
    DD_LOG_INFO("DirectorDesk exiting");
    Core::Log::Shutdown();
    Platform::ShutdownProcess();
    return 0;
}

} // namespace DirectorDesk::App
