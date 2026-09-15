// CommandDispatch: App Command visit split (FND-10 / CR-10).
#pragma once

#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Renderer/IRenderer.h"

#include <cstdint>
#include <functional>
#include <string>

namespace DirectorDesk::Platform {
class Worker;
class IHttpClient;
}

namespace DirectorDesk::App {

struct DispatchServices {
    Renderer::IRenderer* renderer = nullptr;
    Platform::Worker* worker = nullptr;
    Core::ResultQueue<Asset::ModelLoadResult>* loadResults = nullptr;
    Core::ResultQueue<SaveHashJobResult>* hashResults = nullptr;
    std::function<std::uint64_t()> nowMs;
    std::function<void()> requestClose;
    std::function<void(std::uint32_t*, std::uint32_t*)> framebufferSize;
    std::function<void(const std::string& path)> submitImport;
    std::function<void()> submitOfficialRefresh;
    std::function<void(const std::string& assetId)> submitOfficialDownload;
    std::function<void(const std::string& assetId)> cancelOfficialDownload;
    std::function<Core::Result<std::string>()> openModelFile;
    std::function<Core::Result<std::string>()> openMarkdownFile;
    std::function<Core::Result<std::string>()> openJsonFile;
    std::function<Core::Result<std::string>()> openProjectFile;
    std::function<Core::Result<std::string>()> saveProjectFile;
    std::function<Core::Result<std::string>(const std::string& suggestedName)> savePngFile;
    std::function<bool(const std::string& path, Export::ShotResolution)> exportShotTo;
    std::function<bool(const std::string& path, Export::ShotResolution, const std::string& shotId)>
        exportShotPackageTo;
    std::function<bool(const std::string& path)> exportBoardTo;
    std::function<bool(const std::string& path)> exportBoardPdfTo;
    std::function<Core::Result<std::string>(const std::string& suggestedName)> savePdfFile;
    Platform::IHttpClient* http = nullptr;
    Core::ResultQueue<AiJobResult>* aiResults = nullptr;
};

void Dispatch(AppState& state, const Core::Command& command, DispatchServices& services);

bool TryDispatchUndo(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchProject(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchScript(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchScene(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchCamera(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchLibrary(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchExport(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchWorkspace(AppState& state, const Core::Command& command, DispatchServices& services);
bool TryDispatchAi(AppState& state, const Core::Command& command, DispatchServices& services);

void RecordShotSelection(AppState& state, const std::string& shotId);
void SelectFirstShotIfNone(AppState& state);
void RefreshBoard(AppState& state);
void ContinuePendingProjectAction(AppState& state, DispatchServices& services);
void ApplyStoryboardImport(AppState& state, const std::string& utf8Path, const std::string& mode);

} // namespace DirectorDesk::App
