// AppInternals: Implementation for shared App helpers (FND-10).
#include "AppInternals.h"

#include "DirectorDesk/App/AppState.h"
#include "DirectorDesk/App/ProjectBinding.h"
#include "DirectorDesk/App/ProjectFile.h"
#include "DirectorDesk/Asset/ImageDecode.h"
#include "DirectorDesk/Asset/Manifest.h"
#include "DirectorDesk/Camera/Presets.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Export/ShotExport.h"
#include "DirectorDesk/Platform/FileDialog.h"
#include "DirectorDesk/Platform/IHttpClient.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Renderer/PngWriter.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Parser.h"
#include "DirectorDesk/Script/Types.h"
#include "DirectorDesk/Storyboard/BoardComposer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifndef DD_OFFICIAL_MANIFEST_URL
#define DD_OFFICIAL_MANIFEST_URL ""
#endif
#ifndef DD_OFFICIAL_ASSET_BASE_URL
#define DD_OFFICIAL_ASSET_BASE_URL ""
#endif
#ifndef DD_PROJECT_VERSION
#define DD_PROJECT_VERSION "0.1.3"
#endif

namespace DirectorDesk::App {

LaunchOptions ParseOptions(int argc, char** argv) {
    LaunchOptions options;
    for (int i = 1; i < argc; ++i) {
        if (argv == nullptr || argv[i] == nullptr) {
            continue;
        }
        if (std::strcmp(argv[i], "--export-test-png") == 0) {
            options.exportAndQuit = true;
        } else if (std::strcmp(argv[i], "--import") == 0 && i + 1 < argc &&
                   argv[i + 1] != nullptr) {
            options.importPath = argv[++i];
        } else if (std::strcmp(argv[i], "--script") == 0 && i + 1 < argc &&
                   argv[i + 1] != nullptr) {
            options.scriptPath = argv[++i];
        } else if (std::strcmp(argv[i], "--project") == 0 && i + 1 < argc &&
                   argv[i + 1] != nullptr) {
            options.projectPath = argv[++i];
        } else if (std::strcmp(argv[i], "--workspace-mode") == 0 && i + 1 < argc &&
                   argv[i + 1] != nullptr) {
            options.workspaceMode = argv[++i];
        }
    }
    return options;
}

Renderer::GpuModelDesc ToGpuModel(const Asset::ModelData& model) {
    Renderer::GpuModelDesc desc;
    desc.primitives.reserve(model.primitives.size());
    for (const Asset::Primitive& primitive : model.primitives) {
        Renderer::GpuPrimitive gpu;
        gpu.indices = primitive.indices;
        gpu.localTransform = primitive.localTransform;
        if (primitive.materialIndex < model.materials.size()) {
            const Asset::Material& material = model.materials[primitive.materialIndex];
            gpu.baseColor = material.baseColor;
            gpu.textureWidth = material.textureWidth;
            gpu.textureHeight = material.textureHeight;
            gpu.rgba = material.rgba;
        }
        gpu.vertices.reserve(primitive.vertices.size());
        for (const Asset::Vertex& vertex : primitive.vertices) {
            Renderer::GpuVertex gpuVertex;
            gpuVertex.x = vertex.position.x;
            gpuVertex.y = vertex.position.y;
            gpuVertex.z = vertex.position.z;
            gpuVertex.nx = vertex.normal.x;
            gpuVertex.ny = vertex.normal.y;
            gpuVertex.nz = vertex.normal.z;
            gpuVertex.u = vertex.uv.x;
            gpuVertex.v = vertex.uv.y;
            gpuVertex.abgr = vertex.abgr;
            gpu.vertices.push_back(gpuVertex);
        }
        desc.primitives.push_back(std::move(gpu));
    }
    return desc;
}

std::string LibraryAssetLabel(const Asset::Library& library,
                              const Asset::OfficialCatalog& officialCatalog,
                              const std::string& assetId) {
    if (const Asset::LibraryAsset* asset = library.Find(assetId)) {
        return asset->name;
    }
    if (const Asset::ManifestAsset* asset = officialCatalog.FindAsset(assetId)) {
        return Asset::PickLocale(asset->name);
    }
    return assetId;
}

Camera::SubjectFrame SubjectFromScene(const Scene::Document& scene) {
    if (const Scene::Node* node = scene.Selected()) {
        return Camera::MakeSubject(node->transform.position, node->transform.scale);
    }
    return Camera::FallbackSubject();
}

UI::UiPreferences ToUiPreferences(const UserSettings& settings) {
    UI::UiPreferences preferences;
    preferences.lockExportAspect = settings.lockExportAspect;
    preferences.leftFoldExplicit = settings.leftFoldExplicit;
    preferences.leftFolded = settings.leftFolded;
    preferences.showGroundGrid = settings.showGroundGrid;
    preferences.showGroundAxes = settings.showGroundAxes;
    preferences.showThirds = settings.showThirds;
    preferences.showSafeFrame = settings.showSafeFrame;
    preferences.viewportBackground = settings.viewportBackground;
    preferences.uiScale = SanitizeUiScale(settings.uiScale);
    if (preferences.uiScale <= 0.0f) {
        preferences.uiScale = 1.0f;
    }
    preferences.openLastProject = settings.openLastProject;
    preferences.defaultExportDirectory = settings.defaultExportDirectory;
    preferences.defaultSkillId = settings.defaultSkillId;
    return preferences;
}

UserSettings FromUiPreferences(const UI::UiPreferences& preferences) {
    UserSettings settings;
    settings.lockExportAspect = preferences.lockExportAspect;
    settings.leftFoldExplicit = preferences.leftFoldExplicit;
    settings.leftFolded = preferences.leftFolded;
    settings.showGroundGrid = preferences.showGroundGrid;
    settings.showGroundAxes = preferences.showGroundAxes;
    settings.showThirds = preferences.showThirds;
    settings.showSafeFrame = preferences.showSafeFrame;
    settings.viewportBackground = preferences.viewportBackground == "dark" ? "dark" : "neutral";
    settings.uiScale = SanitizeUiScale(preferences.uiScale > 0.0f ? preferences.uiScale : 1.0f);
    settings.openLastProject = preferences.openLastProject;
    settings.defaultExportDirectory = preferences.defaultExportDirectory;
    settings.defaultSkillId = preferences.defaultSkillId;
    return settings;
}

Renderer::RenderSceneView BuildSceneView(const Scene::Document& scene,
                                         const Camera::LightState& light, bool showGroundGrid,
                                         bool showGroundAxes, float uiScale) {
    Renderer::RenderSceneView view;
    view.showTestMesh = scene.IsEmpty();
    view.showGroundGrid = showGroundGrid;
    view.showGroundAxes = showGroundAxes;
    const float scale = uiScale > 0.05f ? uiScale : 1.0f;
    view.groundGridLineWidth = scale;
    view.groundAxisLineWidth = 2.0f * scale;
    view.light.direction = light.direction;
    view.light.color = light.color;
    for (const Scene::Node& node : scene.Nodes()) {
        if (!node.visible) {
            continue;
        }
        Renderer::RenderMeshInstance instance;
        instance.modelId = node.gpuModelId;
        instance.world = node.transform.ToMatrix();
        instance.visible = true;
        view.instances.push_back(instance);
    }
    return view;
}

bool HandleExportTestPng(Renderer::IRenderer& renderer, const Camera::OrbitCamera& camera,
                         const Renderer::RenderSceneView& sceneView, std::uint32_t windowWidth,
                         std::uint32_t windowHeight, std::string& status) {
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = 1280;
    target.height = 720;
    target.transparentBackground = true;

    const float aspect = static_cast<float>(target.width) / static_cast<float>(target.height);
    renderer.BeginFrame(windowWidth == 0 ? 1 : windowWidth, windowHeight == 0 ? 1 : windowHeight);
    renderer.RenderScene(sceneView, camera.BuildView(aspect), target);
    auto pixels = renderer.ReadbackTarget(target);
    if (!pixels.IsOk()) {
        status = pixels.GetError().userMessage;
        DD_LOG_ERROR("{}", pixels.GetError().technicalMessage);
        return false;
    }

    auto userData = Platform::Paths::UserDataDirectory();
    if (!userData.IsOk()) {
        status = userData.GetError().userMessage;
        return false;
    }
    const std::string path = Platform::Paths::Join(
        Platform::Paths::Join(userData.Value(), "exports"), "phase1-offscreen.png");
    auto written = Renderer::WritePng(pixels.Value(), path);
    if (!written.IsOk()) {
        status = written.GetError().userMessage;
        DD_LOG_ERROR("{}", written.GetError().technicalMessage);
        return false;
    }

    bool hasOpaque = false;
    bool hasTransparent = false;
    for (std::size_t i = 3; i < pixels.Value().rgba.size(); i += 4) {
        const std::uint8_t alpha = pixels.Value().rgba[i];
        hasOpaque = hasOpaque || alpha > 200;
        hasTransparent = hasTransparent || alpha < 20;
    }
    const bool alphaOk = hasOpaque && hasTransparent;
    status = alphaOk ? "Exported transparent PNG (alpha OK)"
                     : "Exported PNG, but alpha coverage looks unexpected";
    DD_LOG_INFO("Exported test PNG to {} opaque={} transparent={}", path, hasOpaque,
                hasTransparent);
    return alphaOk;
}

void WriteLibraryPlaceholder(Asset::Library& library, const Asset::LibraryAsset& asset);

bool IsSkillPath(const std::string& utf8Path) {
    const std::string name = Platform::Paths::FileName(utf8Path);
    return name == "SKILL.md" || name == "skill.md";
}

void InstallSkill(AppState& state, const std::string& utf8Path) {
    if (utf8Path.empty()) {
        return;
    }
    if (state.library.Directory().empty()) {
        state.status = "资源库尚未打开";
        return;
    }
    auto imported = state.library.Import(utf8Path, Asset::AssetOrigin::User);
    if (!imported.IsOk()) {
        state.status = imported.GetError().userMessage;
        DD_LOG_ERROR("{}", imported.GetError().technicalMessage);
        return;
    }
    WriteLibraryPlaceholder(state.library, imported.Value());
    state.libraryOriginFilter = "all";
    state.selectedLibraryAssetId = imported.Value().id;
    state.selectionKind = "asset";
    state.selectionId = imported.Value().id;
    state.selectionLabel = imported.Value().name;
    state.status = "已安装 Skill " + imported.Value().name;
}

void SubmitImport(AppState& state, Platform::Worker& worker,
                  Core::ResultQueue<Asset::ModelLoadResult>& results, const std::string& path) {
    if (IsSkillPath(path)) {
        InstallSkill(state, path);
        return;
    }
    if (path.empty() || state.importInProgress) {
        if (state.importInProgress) {
            state.status = "已有模型正在加载";
        }
        return;
    }
    state.importInProgress = true;
    state.status = "正在加载模型...";
    const Asset::LoaderRegistry* registry = &state.registry;
    const std::uint64_t generation = state.projectGeneration;
    worker.Submit([registry, &results, path, generation]() {
        Asset::ModelLoadResult result;
        result.sourcePath = path;
        result.generation = generation;
        auto loaded = registry->Load(path);
        result.ok = loaded.IsOk();
        if (loaded.IsOk()) {
            result.model = std::move(loaded.Value());
        } else {
            result.error = loaded.GetError();
        }
        results.Push(std::move(result));
    });
}

constexpr int kMaxLibraryPreviewUploadsPerFrame = 2;

void SyncLibraryPreviewTextures(Renderer::IRenderer& renderer,
                                std::vector<UI::LibraryAssetView>& views,
                                const std::unordered_map<std::string, std::string>& paths,
                                std::unordered_map<std::string, LibraryPreviewGpu>& gpu,
                                std::unordered_map<std::string, std::string>& failed) {
    std::unordered_set<std::string> visible;
    visible.reserve(views.size());
    for (const UI::LibraryAssetView& item : views) {
        visible.insert(item.id);
    }

    for (auto it = gpu.begin(); it != gpu.end();) {
        if (visible.count(it->first) == 0) {
            if (it->second.texture != 0xFFFFu) {
                renderer.DestroyRgbaTexture(it->second.texture);
            }
            it = gpu.erase(it);
        } else {
            ++it;
        }
    }

    int uploads = 0;
    for (UI::LibraryAssetView& item : views) {
        const auto pathIt = paths.find(item.id);
        const std::string path = pathIt == paths.end() ? std::string() : pathIt->second;

        auto gpuIt = gpu.find(item.id);
        if (gpuIt != gpu.end() && gpuIt->second.path != path) {
            if (gpuIt->second.texture != 0xFFFFu) {
                renderer.DestroyRgbaTexture(gpuIt->second.texture);
            }
            gpu.erase(gpuIt);
            gpuIt = gpu.end();
            failed.erase(item.id);
        }

        if (gpuIt != gpu.end()) {
            item.previewTexture = gpuIt->second.texture;
            continue;
        }

        item.previewTexture = 0xFFFFu;
        if (path.empty()) {
            continue;
        }
        const auto failedIt = failed.find(item.id);
        if (failedIt != failed.end() && failedIt->second == path) {
            continue;
        }
        if (!Platform::Paths::Exists(path)) {
            failed[item.id] = path;
            continue;
        }
        if (uploads >= kMaxLibraryPreviewUploadsPerFrame) {
            continue;
        }

        auto decoded = Asset::DecodeImageFile(path);
        ++uploads;
        if (!decoded.IsOk()) {
            failed[item.id] = path;
            continue;
        }
        auto texture = renderer.CreateRgbaTexture(decoded.Value().width, decoded.Value().height,
                                                  decoded.Value().rgba.data());
        if (!texture.IsOk()) {
            failed[item.id] = path;
            continue;
        }
        failed.erase(item.id);
        LibraryPreviewGpu record;
        record.texture = texture.Value();
        record.path = path;
        gpu[item.id] = std::move(record);
        item.previewTexture = texture.Value();
    }
}

void WriteLibraryPlaceholder(Asset::Library& library, const Asset::LibraryAsset& asset) {
    if (!asset.previewPath.empty()) {
        return;
    }
    Renderer::PixelBuffer pixels;
    pixels.width = 64;
    pixels.height = 64;
    pixels.rgba.assign(64u * 64u * 4u, 255);
    std::uint8_t red = 90;
    std::uint8_t green = 110;
    std::uint8_t blue = 130;
    if (asset.origin == Asset::AssetOrigin::Builtin) {
        red = 40;
        green = 140;
        blue = 130;
    } else if (asset.origin == Asset::AssetOrigin::OnlineCache) {
        red = 110;
        green = 80;
        blue = 160;
    }
    for (std::size_t i = 0; i < pixels.rgba.size(); i += 4) {
        pixels.rgba[i] = red;
        pixels.rgba[i + 1] = green;
        pixels.rgba[i + 2] = blue;
        pixels.rgba[i + 3] = 255;
    }
    const std::string path = Platform::Paths::Join(
        Platform::Paths::Join(library.Directory(), "previews"), asset.id + ".png");
    if (Renderer::WritePng(pixels, path).IsOk()) {
        library.SetPreviewPath(asset.id, path);
    }
}

void IndexLibraryPath(Asset::Library& library, const std::string& path, Asset::AssetOrigin origin) {
    if (path.empty() || !Platform::Paths::Exists(path)) {
        return;
    }
    auto imported = library.Import(path, origin);
    if (imported.IsOk()) {
        WriteLibraryPlaceholder(library, imported.Value());
    }
}

void RetainGpuModel(AppState& state, std::uint32_t modelId) {
    if (modelId == 0) {
        return;
    }
    ++state.gpuModelRefs[modelId];
}

void ReleaseGpuModel(AppState& state, Renderer::IRenderer* renderer, std::uint32_t modelId) {
    if (modelId == 0) {
        return;
    }
    const auto found = state.gpuModelRefs.find(modelId);
    if (found != state.gpuModelRefs.end()) {
        if (found->second > 1) {
            --found->second;
            return;
        }
        state.gpuModelRefs.erase(found);
    }
    if (renderer != nullptr) {
        renderer->DestroyModel(modelId);
    }
}

void ApplyLoadedModel(AppState& state, Renderer::IRenderer& renderer,
                      Asset::ModelLoadResult result) {
    if (!result.ok) {
        state.status = result.error.userMessage;
        DD_LOG_ERROR("{}", result.error.technicalMessage);
        return;
    }

    auto uploaded = renderer.CreateModel(ToGpuModel(result.model));
    if (!uploaded.IsOk()) {
        state.status = uploaded.GetError().userMessage;
        DD_LOG_ERROR("{}", uploaded.GetError().technicalMessage);
        return;
    }

    Scene::Node node;
    node.id = state.scene.NextNodeId();
    node.name = result.model.name.empty() ? Platform::Paths::FileName(result.sourcePath)
                                          : result.model.name;
    node.gpuModelId = uploaded.Value();
    node.hasSkin = result.model.hasSkin;
    node.sourcePath = result.sourcePath;
    if (const Asset::LibraryAsset* indexed = state.library.FindBySourcePath(result.sourcePath);
        indexed != nullptr) {
        node.libraryAssetId = indexed->id;
        node.officialVersion = indexed->version;
        node.officialEntrypoint = indexed->entrypoint;
    } else {
        node.libraryAssetId = Asset::Library::MakeId(result.sourcePath);
        IndexLibraryPath(state.library, result.sourcePath, Asset::AssetOrigin::User);
    }
    state.scene.Add(std::move(node));
    RetainGpuModel(state, uploaded.Value());
    state.status = result.model.warnings.empty() ? "已导入 " + state.scene.Selected()->name
                                                 : result.model.warnings.front();
    DD_LOG_INFO("Imported model {} as {}", result.sourcePath, state.scene.Selected()->name);
}

void ReleaseSceneGpu(Scene::Document& scene, Renderer::IRenderer& renderer) {
    std::unordered_set<std::uint32_t> released;
    for (const Scene::Node& node : scene.Nodes()) {
        if (node.gpuModelId != 0 && released.insert(node.gpuModelId).second) {
            renderer.DestroyModel(node.gpuModelId);
        }
    }
}

Asset::ModelData MakePlaceholderBox();

void BeginProjectGeneration(AppState& state) {
    ++state.projectGeneration;
    state.sceneLoadPending = 0;
    state.sceneLoadTotal = 0;
    state.importInProgress = false;
    state.heldLoadResult.reset();
    state.projectSaveInProgress = false;
    state.projectSaveQueued = false;
    state.proceedAfterSave = false;
    state.projectSavePendingPath.clear();
    state.gpuModelRefs.clear();
}

void AttachPlaceholderModels(AppState& state, Renderer::IRenderer* renderer) {
    if (renderer == nullptr) {
        return;
    }
    for (const Scene::Node& item : state.scene.Nodes()) {
        Scene::Node* node = state.scene.Find(item.id);
        if (node == nullptr) {
            continue;
        }
        auto uploaded = renderer->CreateModel(ToGpuModel(MakePlaceholderBox()));
        if (uploaded.IsOk()) {
            node->gpuModelId = uploaded.Value();
            RetainGpuModel(state, uploaded.Value());
        }
    }
}

void QueueSceneModelLoads(AppState& state, Platform::Worker& worker,
                          Core::ResultQueue<Asset::ModelLoadResult>& results) {
    const Asset::LoaderRegistry* registry = &state.registry;
    const std::uint64_t generation = state.projectGeneration;
    std::uint32_t total = 0;
    for (const Scene::Node& item : state.scene.Nodes()) {
        Scene::Node* node = state.scene.Find(item.id);
        if (node == nullptr || node->assetMissing || node->sourcePath.empty()) {
            continue;
        }
        const std::string nodeId = node->id;
        const std::string path = node->sourcePath;
        ++total;
        worker.Submit([registry, path, nodeId, generation, &results]() {
            Asset::ModelLoadResult result;
            result.sourcePath = path;
            result.nodeId = nodeId;
            result.generation = generation;
            auto loaded = registry->Load(path);
            result.ok = loaded.IsOk();
            if (loaded.IsOk()) {
                result.model = std::move(loaded.Value());
            } else {
                result.error = loaded.GetError();
            }
            results.Push(std::move(result));
        });
    }
    state.sceneLoadTotal = total;
    state.sceneLoadPending = total;
}

constexpr int kMaxSceneModelUploadsPerFrame = 1;

void FinishSceneLoadSlot(AppState& state) {
    if (state.sceneLoadPending > 0) {
        --state.sceneLoadPending;
    }
    if (state.sceneLoadPending == 0) {
        state.sceneLoadTotal = 0;
    }
}

void ApplySceneNodeModel(AppState& state, Renderer::IRenderer* renderer,
                         Asset::ModelLoadResult result) {
    Scene::Node* node = state.scene.Find(result.nodeId);
    if (node == nullptr) {
        FinishSceneLoadSlot(state);
        return;
    }
    if (!result.ok) {
        node->assetMissing = true;
        state.status = result.error.userMessage.empty() ? "模型加载失败" : result.error.userMessage;
        FinishSceneLoadSlot(state);
        return;
    }
    if (renderer == nullptr) {
        node->assetMissing = false;
        node->hasSkin = result.model.hasSkin;
        FinishSceneLoadSlot(state);
        return;
    }
    auto uploaded = renderer->CreateModel(ToGpuModel(result.model));
    if (!uploaded.IsOk()) {
        node->assetMissing = true;
        state.status = uploaded.GetError().userMessage;
        FinishSceneLoadSlot(state);
        return;
    }
    const std::uint32_t oldId = node->gpuModelId;
    const std::uint32_t newId = uploaded.Value();
    const bool hasSkin = result.model.hasSkin;
    if (oldId == 0) {
        node->gpuModelId = newId;
        node->hasSkin = hasSkin;
        RetainGpuModel(state, newId);
    } else {
        std::vector<std::string> users;
        for (const Scene::Node& item : state.scene.Nodes()) {
            if (item.gpuModelId == oldId) {
                users.push_back(item.id);
            }
        }
        for (const std::string& id : users) {
            if (Scene::Node* shared = state.scene.Find(id)) {
                shared->gpuModelId = newId;
                shared->hasSkin = hasSkin;
                RetainGpuModel(state, newId);
                ReleaseGpuModel(state, renderer, oldId);
            }
        }
    }
    node->assetMissing = false;
    state.storyboard.MarkLinkedStale();
    state.thumbScheduler.NotifyBusy(NowMs());
    FinishSceneLoadSlot(state);
}

bool TakeLoadResult(AppState& state, Core::ResultQueue<Asset::ModelLoadResult>& loadResults,
                    Asset::ModelLoadResult& out) {
    if (state.heldLoadResult.has_value()) {
        out = std::move(*state.heldLoadResult);
        state.heldLoadResult.reset();
        return true;
    }
    return loadResults.TryPop(out);
}

void DrainLoadResults(AppState& state, Renderer::IRenderer* renderer,
                      Core::ResultQueue<Asset::ModelLoadResult>& loadResults) {
    int gpuUploads = 0;
    Asset::ModelLoadResult loaded;
    while (TakeLoadResult(state, loadResults, loaded)) {
        if (loaded.generation != state.projectGeneration) {
            continue;
        }
        const bool needsGpu = loaded.ok && renderer != nullptr;
        if (needsGpu && gpuUploads >= kMaxSceneModelUploadsPerFrame) {
            state.heldLoadResult = std::move(loaded);
            break;
        }
        if (loaded.nodeId.empty()) {
            state.importInProgress = false;
            if (!loaded.ok) {
                state.status = loaded.error.userMessage;
                DD_LOG_ERROR("{}", loaded.error.technicalMessage);
                continue;
            }
            if (renderer == nullptr) {
                continue;
            }
            ApplyLoadedModel(state, *renderer, std::move(loaded));
            state.projectDirty = true;
            state.storyboard.MarkLinkedStale();
            state.thumbScheduler.NotifyBusy(NowMs());
            ++gpuUploads;
            continue;
        }
        if (needsGpu) {
            ++gpuUploads;
        }
        ApplySceneNodeModel(state, renderer, std::move(loaded));
    }
}

Asset::ModelData MakePlaceholderBox() {
    Asset::ModelData model;
    model.name = "missing";
    Asset::Material material;
    material.baseColor = glm::vec4(0.55f, 0.35f, 0.55f, 1.0f);
    model.materials.push_back(material);
    Asset::Primitive primitive;
    primitive.materialIndex = 0;
    const glm::vec3 corners[8] = {
        {-0.4f, 0.0f, -0.4f}, {0.4f, 0.0f, -0.4f}, {0.4f, 0.8f, -0.4f}, {-0.4f, 0.8f, -0.4f},
        {-0.4f, 0.0f, 0.4f},  {0.4f, 0.0f, 0.4f},  {0.4f, 0.8f, 0.4f},  {-0.4f, 0.8f, 0.4f},
    };
    const int faces[12][3] = {{0, 1, 2}, {0, 2, 3}, {1, 5, 6}, {1, 6, 2}, {5, 4, 7}, {5, 7, 6},
                              {4, 0, 3}, {4, 3, 7}, {3, 2, 6}, {3, 6, 7}, {4, 5, 1}, {4, 1, 0}};
    for (const auto& face : faces) {
        for (int corner : face) {
            Asset::Vertex vertex;
            vertex.position = corners[corner];
            primitive.vertices.push_back(vertex);
            primitive.indices.push_back(static_cast<std::uint32_t>(primitive.indices.size()));
        }
    }
    model.primitives.push_back(std::move(primitive));
    return model;
}

void ResetProject(Scene::Document& scene, Camera::CameraManager& cameras, Link::Table& links,
                  Script::Document& script, Renderer::IRenderer& renderer, std::string& projectId,
                  std::string& projectName, std::string& projectPath,
                  std::vector<std::string>& collapsedScenes, bool& projectDirty) {
    ReleaseSceneGpu(scene, renderer);
    scene.Clear();
    cameras.Replace({}, {}, Camera::LightPresetKind::Neutral);
    links.Clear();
    script.Reset();
    projectId = ProjectFile::MakeProjectId();
    projectName = "未命名工程";
    projectPath.clear();
    collapsedScenes.clear();
    projectDirty = false;
}

void PersistUserSettings(AppState& state) {
    if (state.userSettingsPath.empty()) {
        return;
    }
    auto saved = SaveUserSettings(state.userSettingsPath, state.userSettings);
    if (!saved.IsOk()) {
        DD_LOG_WARN("{}", saved.GetError().technicalMessage);
    }
}

void RememberLastProject(AppState& state, const std::string& path) {
    if (path.empty() || path == state.userSettings.lastProjectPath) {
        return;
    }
    state.userSettings.lastProjectPath = path;
    PersistUserSettings(state);
}

bool FinishSaveProject(AppState& state, const std::string& path) {
    if (path.empty()) {
        return false;
    }
    auto snapshot =
        CaptureProject(state.projectId, state.projectName, path, state.scene, state.cameras,
                       state.links, state.script, state.library, state.collapsedScenes,
                       state.storyboardLayout);
    auto saved = ProjectFile::Save(path, snapshot);
    if (!saved.IsOk()) {
        state.status = saved.GetError().userMessage;
        DD_LOG_ERROR("{}", saved.GetError().technicalMessage);
        return false;
    }
    state.projectId = snapshot.projectId;
    state.projectPath = path;
    state.projectDirty = false;
    state.projectSaveInProgress = false;
    state.status = "已保存工程";
    RememberLastProject(state, path);
    return true;
}

SaveProjectStatus RequestSaveProject(AppState& state, const std::string& path,
                                     Platform::Worker* worker,
                                     Core::ResultQueue<SaveHashJobResult>* hashResults) {
    if (path.empty()) {
        return SaveProjectStatus::Failed;
    }
    if (state.projectSaveInProgress) {
        state.projectSaveQueued = true;
        state.projectSavePendingPath = path;
        return SaveProjectStatus::Deferred;
    }
    const std::vector<std::string> stale = CollectUncachedSourcePaths(state.scene, state.library);
    if (stale.empty() || worker == nullptr || hashResults == nullptr) {
        return FinishSaveProject(state, path) ? SaveProjectStatus::Saved
                                              : SaveProjectStatus::Failed;
    }
    state.projectSaveInProgress = true;
    state.projectSavePendingPath = path;
    state.status = "正在校验资产";
    const std::uint64_t generation = state.projectGeneration;
    worker->Submit([stale, generation, path, hashResults]() {
        SaveHashJobResult result;
        result.generation = generation;
        result.savePath = path;
        for (const std::string& sourcePath : stale) {
            FileHashResult item;
            item.path = sourcePath;
            auto hash = ProjectFile::Sha256File(sourcePath);
            item.ok = hash.IsOk();
            if (hash.IsOk()) {
                item.sha256 = hash.Value();
            } else {
                item.message = hash.GetError().userMessage;
            }
            result.hashes.push_back(std::move(item));
        }
        hashResults->Push(std::move(result));
    });
    return SaveProjectStatus::Deferred;
}

void DrainSaveHashResults(AppState& state, Core::ResultQueue<SaveHashJobResult>& hashResults) {
    SaveHashJobResult job;
    while (hashResults.TryPop(job)) {
        if (job.generation != state.projectGeneration) {
            continue;
        }
        for (const FileHashResult& item : job.hashes) {
            if (item.ok) {
                state.library.RecordContentHash(item.path, {}, item.sha256);
            } else if (!item.message.empty()) {
                state.status = item.message;
            }
        }
        state.projectSaveInProgress = false;
        const std::string path = job.savePath.empty() ? state.projectSavePendingPath : job.savePath;
        FinishSaveProject(state, path);
    }
}

bool OpenProjectAt(const std::string& path, AppState& state, Renderer::IRenderer* renderer,
                   Platform::Worker* worker,
                   Core::ResultQueue<Asset::ModelLoadResult>* loadResults) {
    auto loaded = ProjectFile::Load(path);
    if (!loaded.IsOk()) {
        state.status = loaded.GetError().userMessage;
        DD_LOG_ERROR("{}", loaded.GetError().technicalMessage);
        return false;
    }

    Scene::Document nextScene;
    Camera::CameraManager nextCameras;
    Link::Table nextLinks;
    Script::Document nextScript;
    std::vector<std::string> diagnostics;
    auto hydrated = HydrateProject(loaded.Value(), Platform::Paths::Parent(path), nextScene,
                                   nextCameras, nextLinks, nextScript, state.library, diagnostics);
    if (!hydrated.IsOk()) {
        state.status = hydrated.GetError().userMessage;
        DD_LOG_ERROR("{}", hydrated.GetError().technicalMessage);
        return false;
    }

    BeginProjectGeneration(state);
    if (renderer != nullptr) {
        ReleaseSceneGpu(state.scene, *renderer);
    }
    state.scene.ReplaceNodes(nextScene.Nodes(), nextScene.SelectedId());
    state.cameras.Replace(nextCameras.Cameras(), nextCameras.SelectedId(),
                          nextCameras.LightPreset());
    state.links.Replace(nextLinks.All());
    if (nextScript.Path().empty()) {
        state.script.Reset();
    } else {
        const auto loadedScript = state.script.LoadFromPath(nextScript.Path());
        if (!loadedScript.IsOk()) {
            state.script.Reset();
            state.status = loadedScript.GetError().userMessage;
        }
    }
    AttachPlaceholderModels(state, renderer);
    if (worker != nullptr && loadResults != nullptr) {
        QueueSceneModelLoads(state, *worker, *loadResults);
    }
    state.projectId = loaded.Value().projectId;
    state.projectName = loaded.Value().name;
    state.projectPath = path;
    state.collapsedScenes.clear();
    std::unordered_set<std::string> sceneIds;
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& item : state.script.PublishedSnapshot().scenes) {
            sceneIds.insert(item.id);
        }
    }
    for (const std::string& id : loaded.Value().collapsedScenes) {
        if (sceneIds.count(id) != 0) {
            state.collapsedScenes.push_back(id);
        }
    }
    state.storyboardLayout = loaded.Value().storyboardLayout.empty()
                                 ? std::string("grid")
                                 : loaded.Value().storyboardLayout;
    state.projectDirty = false;
    state.status = diagnostics.empty() ? "已打开工程" : diagnostics.front();
    RememberLastProject(state, path);
    return true;
}

std::uint64_t NowMs() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}

Storyboard::StoryboardSourceSnapshot MakeBoardSource(const Script::Document& script,
                                                     const Link::Table& links,
                                                     const Camera::CameraManager& cameras,
                                                     const std::vector<std::string>& collapsed,
                                                     const std::string& projectId) {
    Storyboard::StoryboardSourceSnapshot snapshot;
    snapshot.projectId = projectId;
    snapshot.scriptValid = script.HasPublishedSnapshot();
    snapshot.selectedShotId = script.SelectedShotId();
    snapshot.structureRevision = script.ExternalRevision();
    if (!snapshot.scriptValid) {
        return snapshot;
    }
    snapshot.documentTitle = script.PublishedSnapshot().documentTitle;
    int sceneIndex = 1;
    for (const Script::Scene& scene : script.PublishedSnapshot().scenes) {
        Storyboard::SceneSource item;
        item.id = scene.id;
        item.title = scene.title;
        item.index = sceneIndex++;
        item.collapsed = std::find(collapsed.begin(), collapsed.end(), scene.id) != collapsed.end();
        int shotIndex = 1;
        for (const Script::Shot& shot : scene.shots) {
            Storyboard::ShotSource shotItem;
            shotItem.id = shot.id;
            shotItem.title = shot.title;
            shotItem.indexInScene = shotIndex++;
            if (const std::string* cameraId = links.CameraForShot(shot.id)) {
                shotItem.cameraId = *cameraId;
                shotItem.cameraExists = cameras.Find(*cameraId) != nullptr;
            }
            shotItem.metaLine = Script::ComposeShotMetaLine(shot.meta);
            item.shots.push_back(std::move(shotItem));
        }
        snapshot.scenes.push_back(std::move(item));
    }
    return snapshot;
}

void IndexReadyOfficial(Asset::OfficialCatalog& catalog, Asset::Library& library) {
    for (const Asset::ManifestAsset& asset : catalog.Manifest().assets) {
        const Asset::OfficialAssetState* state = catalog.State(asset.id);
        if (state == nullptr || state->status != Asset::OfficialDownloadStatus::Ready ||
            state->entrypointPath.empty()) {
            continue;
        }
        Asset::LibraryAsset item;
        item.id = asset.id;
        item.name = Asset::PickLocale(asset.name);
        item.sourcePath = state->entrypointPath;
        item.format = asset.format;
        item.origin = Asset::AssetOrigin::OnlineCache;
        item.category = asset.category;
        item.tags = asset.tags;
        item.previewPath = state->previewPath;
        item.version = asset.version;
        item.entrypoint = asset.entrypoint;
        library.Upsert(item);
    }
}

Asset::OfficialEndpoints MakeOfficialEndpoints() {
    Asset::OfficialEndpoints endpoints;
    endpoints.manifestUrl = DD_OFFICIAL_MANIFEST_URL;
    endpoints.assetBaseUrl = DD_OFFICIAL_ASSET_BASE_URL;
    return endpoints;
}

const char* PreviewText(Storyboard::PreviewStatus status) {
    switch (status) {
        case Storyboard::PreviewStatus::Ready:
            return "就绪";
        case Storyboard::PreviewStatus::Stale:
            return "过期";
        case Storyboard::PreviewStatus::Rendering:
            return "渲染中";
        case Storyboard::PreviewStatus::Failed:
            return "失败";
        case Storyboard::PreviewStatus::Missing:
        default:
            return "缺失";
    }
}

const char* WorkspaceModeLabel(const std::string& modeId) {
    if (modeId == "script") {
        return "编剧模式";
    }
    if (modeId == "set") {
        return "置景模式";
    }
    if (modeId == "review") {
        return "审片模式";
    }
    return "掌机模式";
}

bool IsWorkspaceModeId(const std::string& modeId) {
    return modeId == "script" || modeId == "set" || modeId == "shoot" || modeId == "review";
}

std::string FindShotTitle(const Script::Document& script, const std::string& shotId) {
    if (!script.HasPublishedSnapshot()) {
        return shotId;
    }
    for (const Script::Scene& sceneItem : script.PublishedSnapshot().scenes) {
        for (const Script::Shot& shot : sceneItem.shots) {
            if (shot.id == shotId) {
                return shot.title.empty() ? shot.id : shot.title;
            }
        }
    }
    return shotId;
}

void PushExportLog(std::vector<UI::ExportLogView>& log, UI::ExportLogView entry) {
    log.push_back(std::move(entry));
    if (log.size() > 20) {
        log.erase(log.begin());
    }
}

const char* DiagnosticSeverityText(Script::DiagnosticSeverity severity) {
    switch (severity) {
        case Script::DiagnosticSeverity::Error:
            return "错误";
        case Script::DiagnosticSeverity::Warning:
            return "警告";
        case Script::DiagnosticSeverity::Hint:
            return "提示";
    }
    return "提示";
}

void ApplyScriptLoad(Script::Document& script, const std::string& path, std::string& status) {
    auto loaded = script.LoadFromPath(path);
    if (!loaded.IsOk()) {
        status = loaded.GetError().userMessage;
        DD_LOG_ERROR("{}", loaded.GetError().technicalMessage);
        return;
    }
    status = "已打开剧本 " + Platform::Paths::FileName(path);
    DD_LOG_INFO("Loaded script {}", path);
}

void HandleSaveScript(Script::Document& script, std::string& status) {
    if (script.Path().empty()) {
        auto path = Platform::FileDialog::SaveMarkdownFile();
        if (!path.IsOk()) {
            status = path.GetError().userMessage;
            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
            return;
        }
        if (path.Value().empty()) {
            return;
        }
        auto saved = script.SaveToPath(path.Value());
        if (!saved.IsOk()) {
            status = saved.GetError().userMessage;
            DD_LOG_ERROR("{}", saved.GetError().technicalMessage);
            return;
        }
    } else {
        auto saved = script.Save();
        if (!saved.IsOk()) {
            status = saved.GetError().userMessage;
            DD_LOG_ERROR("{}", saved.GetError().technicalMessage);
            return;
        }
    }
    status = "已保存剧本 " + Platform::Paths::FileName(script.Path());
    DD_LOG_INFO("Saved script {}", script.Path());
}

bool ExportShotPng(AppState& state, Renderer::IRenderer& renderer, std::uint32_t fbW,
                   std::uint32_t fbH, const std::string& path, Export::ShotResolution resolution) {
    UI::ExportLogView log;
    log.label = Export::ResolutionId(resolution);
    log.shotId = state.script.SelectedShotId();
    log.shotTitle = FindShotTitle(state.script, state.script.SelectedShotId());
    log.path = path;
    if (state.cameras.Selected() == nullptr) {
        state.status = "没有可导出的相机";
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    const auto target = Export::MakeOffscreenTarget(resolution, state.exportTransparent);
    const float aspect = static_cast<float>(target.width) / static_cast<float>(target.height);
    renderer.BeginFrame(fbW, fbH);
    renderer.RenderScene(BuildSceneView(state.scene, state.cameras.CurrentLight(), false),
                         state.cameras.Selected()->orbit.BuildView(aspect), target);
    auto pixels = renderer.ReadbackTarget(target);
    if (!pixels.IsOk()) {
        state.status = pixels.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    auto written = Renderer::WritePng(pixels.Value(), path);
    if (!written.IsOk()) {
        state.status = written.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    state.storyboard.MarkShotExported(state.script.SelectedShotId());
    state.status = "已导出 " + Platform::Paths::FileName(path);
    log.ok = true;
    PushExportLog(state.exportLog, std::move(log));
    return true;
}

std::string UtcTimestamp() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bool ExportShotPackage(AppState& state, Renderer::IRenderer& renderer, std::uint32_t fbW,
                       std::uint32_t fbH, const std::string& path,
                       Export::ShotResolution resolution, const std::string& shotId) {
    UI::ExportLogView log;
    log.label = "package";
    log.shotId = shotId;
    log.shotTitle = FindShotTitle(state.script, shotId);
    log.path = path;
    const Script::Shot* shot = nullptr;
    const Script::Scene* scene = nullptr;
    if (state.script.HasPublishedSnapshot()) {
        for (const Script::Scene& sceneItem : state.script.PublishedSnapshot().scenes) {
            for (const Script::Shot& shotItem : sceneItem.shots) {
                if (shotItem.id == shotId) {
                    shot = &shotItem;
                    scene = &sceneItem;
                    break;
                }
            }
            if (shot != nullptr) {
                break;
            }
        }
    }
    if (shot == nullptr) {
        state.status = "没有可导出的镜头";
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    const Camera::CameraRig* camera = nullptr;
    if (const std::string* cameraId = state.links.CameraForShot(shotId)) {
        camera = state.cameras.Find(*cameraId);
    }
    if (camera == nullptr) {
        camera = state.cameras.Selected();
    }
    if (camera == nullptr) {
        state.status = "没有可导出的相机";
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    const auto target = Export::MakeOffscreenTarget(resolution, state.exportTransparent);
    const float aspect = static_cast<float>(target.width) / static_cast<float>(target.height);
    renderer.BeginFrame(fbW, fbH);
    renderer.RenderScene(BuildSceneView(state.scene, state.cameras.CurrentLight(), false),
                         camera->orbit.BuildView(aspect), target);
    auto pixels = renderer.ReadbackTarget(target);
    if (!pixels.IsOk()) {
        state.status = pixels.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    const glm::vec3 position = camera->orbit.Position();
    const glm::vec3 lookAt = camera->orbit.Target();
    glm::vec3 forward = lookAt - position;
    if (glm::length(forward) < 1.0e-5f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        forward = glm::normalize(forward);
    }
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(forward, up)) > 0.999f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    const glm::quat rotation = glm::quatLookAt(forward, up);

    Export::ShotPackageInput input;
    input.generatedBy = std::string("DirectorDesk ") + DD_PROJECT_VERSION;
    input.generatedAt = UtcTimestamp();
    input.projectName = state.projectName;
    input.projectId = state.projectId;
    input.sceneId = scene->id;
    input.sceneTitle = scene->title;
    input.sceneBody = scene->body;
    input.shotId = shot->id;
    input.shotTitle = shot->title;
    input.shotBody = shot->body;
    for (const Script::ShotMeta& meta : shot->meta) {
        input.meta.push_back(Export::ShotPackageMeta{meta.key, meta.value});
        if (meta.key == "提示词") {
            input.prompt = meta.value;
        } else if (meta.key == "负面提示词") {
            input.negativePrompt = meta.value;
        }
    }
    input.imageWidth = target.width;
    input.imageHeight = target.height;
    input.transparentBackground = state.exportTransparent;
    input.cameraId = camera->id;
    input.cameraName = camera->name;
    input.cameraPosition = {position.x, position.y, position.z};
    input.cameraRotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    input.cameraLookAt = {lookAt.x, lookAt.y, lookAt.z};
    input.verticalFovDegrees = camera->orbit.FovYDegrees();
    input.aspect = aspect;
    input.cameraPreset = camera->lastPreset;
    input.lightingPreset = Camera::LightPresetId(state.cameras.LightPreset());
    for (const Scene::Node& node : state.scene.Nodes()) {
        if (!node.visible) {
            continue;
        }
        Export::ShotPackageNode packed;
        packed.id = node.id;
        packed.name = node.name;
        packed.assetId = node.libraryAssetId.empty() ? node.assetRef : node.libraryAssetId;
        packed.assetName = node.name;
        if (const Asset::LibraryAsset* asset = state.library.Find(packed.assetId)) {
            packed.assetName = asset->name;
        } else if (const Asset::ManifestAsset* official =
                       state.officialCatalog.FindAsset(packed.assetId)) {
            packed.assetName = Asset::PickLocale(official->name);
        }
        packed.position = {node.transform.position.x, node.transform.position.y,
                           node.transform.position.z};
        packed.rotation = {node.transform.rotation.x, node.transform.rotation.y,
                           node.transform.rotation.z, node.transform.rotation.w};
        packed.scale = {node.transform.scale.x, node.transform.scale.y, node.transform.scale.z};
        packed.visible = true;
        input.sceneNodes.push_back(std::move(packed));
    }

    const std::string directory = Platform::Paths::Parent(path);
    auto writtenPackage = Export::WriteShotPackage(directory, input, pixels.Value());
    if (!writtenPackage.IsOk()) {
        state.status = writtenPackage.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    state.storyboard.MarkShotExported(shotId);
    state.status = "已导出镜头包 " + writtenPackage.Value().pngPath;
    log.path = writtenPackage.Value().jsonPath;
    log.ok = true;
    PushExportLog(state.exportLog, std::move(log));
    return true;
}

bool ExportBoardPng(AppState& state, const std::string& path) {
    UI::ExportLogView log;
    log.label = "board";
    log.path = path;
    Storyboard::BoardComposeRequest request;
    request.layout = state.storyboard.ExportLayout();
    for (const Storyboard::LayoutCard& card : request.layout.cards) {
        if (card.kind != Storyboard::CardKind::Shot) {
            continue;
        }
        if (const Storyboard::ThumbnailRecord* thumb = state.storyboard.Thumbnail(card.shotId)) {
            request.thumbnails[card.shotId] = thumb->pixels;
        }
    }
    auto font = Platform::Paths::UiFontFile();
    if (font.IsOk()) {
        request.fontPath = font.Value();
    }
    auto composed = Storyboard::ComposeBoard(request);
    if (!composed.IsOk()) {
        state.status = composed.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    Renderer::PixelBuffer pixels;
    pixels.width = composed.Value().pixels.width;
    pixels.height = composed.Value().pixels.height;
    pixels.rgba = composed.Value().pixels.rgba;
    auto written = Renderer::WritePng(pixels, path);
    if (!written.IsOk()) {
        state.status = written.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    const std::string imageName = Platform::Paths::FileName(path);
    std::vector<Export::BoardIndexShot> indexShots;
    for (const Storyboard::LayoutCard& card : request.layout.cards) {
        if (card.kind != Storyboard::CardKind::Shot) {
            continue;
        }
        Export::BoardIndexShot item;
        item.id = card.shotId;
        item.title = card.title;
        item.image = imageName;
        item.metaLine = card.metaLine;
        indexShots.push_back(std::move(item));
    }
    const std::string indexPath =
        Platform::Paths::Join(Platform::Paths::Parent(path), "board.json");
    auto indexWritten = Export::WriteBoardIndex(indexPath, imageName, indexShots);
    if (!indexWritten.IsOk()) {
        state.status = indexWritten.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    state.status = composed.Value().scaledToMax ? "已导出分镜总览（已缩放）" : "已导出分镜总览";
    log.ok = true;
    PushExportLog(state.exportLog, std::move(log));
    return true;
}

bool ExportBoardPdf(AppState& state, const std::string& path) {
    UI::ExportLogView log;
    log.label = "pdf";
    log.path = path;
    Storyboard::BoardComposeRequest request;
    request.layout = state.storyboard.ExportLayout();
    for (const Storyboard::LayoutCard& card : request.layout.cards) {
        if (card.kind != Storyboard::CardKind::Shot) {
            continue;
        }
        if (const Storyboard::ThumbnailRecord* thumb = state.storyboard.Thumbnail(card.shotId)) {
            request.thumbnails[card.shotId] = thumb->pixels;
        }
    }
    auto font = Platform::Paths::UiFontFile();
    if (font.IsOk()) {
        request.fontPath = font.Value();
    }
    auto composed = Storyboard::ComposePdfPages(request, 6);
    if (!composed.IsOk()) {
        state.status = composed.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    std::vector<Renderer::PixelBuffer> pages;
    pages.reserve(composed.Value().pages.size());
    for (Storyboard::ImageBuffer& image : composed.Value().pages) {
        Renderer::PixelBuffer pixels;
        pixels.width = image.width;
        pixels.height = image.height;
        pixels.rgba = std::move(image.rgba);
        pages.push_back(std::move(pixels));
    }
    auto written = Export::WriteBoardPdf(path, pages);
    if (!written.IsOk()) {
        state.status = written.GetError().userMessage;
        log.message = state.status;
        PushExportLog(state.exportLog, std::move(log));
        return false;
    }
    state.status = "已导出分镜 PDF " + path;
    log.ok = true;
    PushExportLog(state.exportLog, std::move(log));
    return true;
}

void TickStoryboardGpu(AppState& state, Renderer::IRenderer& renderer) {
    std::vector<std::uint16_t> dropped;
    state.storyboard.TakeDestroyedTextures(dropped);
    for (std::uint16_t id : dropped) {
        renderer.DestroyRgbaTexture(id);
    }
    if (!state.pendingThumbShotId.empty()) {
        auto pixels = renderer.TakeReadback();
        if (pixels.IsOk()) {
            if (Storyboard::ThumbnailRecord* old =
                    state.storyboard.ThumbnailMutable(state.pendingThumbShotId)) {
                if (old->textureIndex != 0xFFFFu) {
                    renderer.DestroyRgbaTexture(old->textureIndex);
                    old->textureIndex = 0xFFFFu;
                }
            }
            Storyboard::ImageBuffer image;
            image.width = pixels.Value().width;
            image.height = pixels.Value().height;
            image.rgba = std::move(pixels.Value().rgba);
            auto texture = renderer.CreateRgbaTexture(image.width, image.height, image.rgba.data());
            state.storyboard.SetThumbnail(state.pendingThumbShotId, std::move(image),
                                          state.storyboard.DirectorRevision());
            if (texture.IsOk()) {
                if (Storyboard::ThumbnailRecord* record =
                        state.storyboard.ThumbnailMutable(state.pendingThumbShotId)) {
                    record->textureIndex = texture.Value();
                }
            }
            state.pendingThumbShotId.clear();
        } else if (!renderer.HasPendingReadback()) {
            state.storyboard.MarkShotFailed(state.pendingThumbShotId);
            state.pendingThumbShotId.clear();
        }
    }
    state.thumbScheduler.BeginFrame();
    std::vector<std::string> keepIds;
    keepIds.push_back(state.script.SelectedShotId());
    std::vector<std::uint16_t> evicted;
    state.storyboard.EvictThumbnails(keepIds, 48, evicted);
    for (std::uint16_t id : evicted) {
        renderer.DestroyRgbaTexture(id);
    }
    state.storyboard.Touch(state.script.SelectedShotId(), state.frameIndex++);
}

void DrainAppQueues(AppState& state, Renderer::IRenderer& renderer,
                    Core::ResultQueue<Asset::ModelLoadResult>& loadResults,
                    Core::ResultQueue<OfficialRefreshResult>& officialRefreshResults,
                    Core::ResultQueue<OfficialDownloadJobResult>& officialDownloadResults,
                    Core::ResultQueue<OfficialProgressUpdate>& officialProgressResults,
                    Core::ResultQueue<SaveHashJobResult>& hashResults,
                    Core::ResultQueue<AiJobResult>* aiResults) {
    DrainLoadResults(state, &renderer, loadResults);
    DrainSaveHashResults(state, hashResults);
    OfficialRefreshResult officialRefresh;
    while (officialRefreshResults.TryPop(officialRefresh)) {
        state.officialRefreshInFlight = false;
        if (officialRefresh.ok) {
            auto applied = state.officialCatalog.ApplyManifestJson(officialRefresh.body);
            if (applied.IsOk()) {
                IndexReadyOfficial(state.officialCatalog, state.library);
                state.officialCatalogStatus = "官方清单已更新";
            } else {
                state.officialCatalog.LoadCache();
                state.officialCatalogStatus = applied.GetError().userMessage;
            }
        } else {
            state.officialCatalog.LoadCache();
            state.officialCatalogStatus = officialRefresh.message.empty()
                                              ? "清单刷新失败，已使用缓存"
                                              : officialRefresh.message;
        }
    }
    OfficialProgressUpdate officialProgress;
    while (officialProgressResults.TryPop(officialProgress)) {
        if (Asset::OfficialAssetState* catalogState =
                state.officialCatalog.MutableState(officialProgress.assetId)) {
            catalogState->progress = officialProgress.progress;
            if (catalogState->status == Asset::OfficialDownloadStatus::NotDownloaded ||
                catalogState->status == Asset::OfficialDownloadStatus::Queued) {
                catalogState->status = Asset::OfficialDownloadStatus::Downloading;
            }
        }
    }
    OfficialDownloadJobResult officialDownload;
    while (officialDownloadResults.TryPop(officialDownload)) {
        state.officialCancels.erase(officialDownload.assetId);
        if (Asset::OfficialAssetState* catalogState =
                state.officialCatalog.MutableState(officialDownload.assetId)) {
            *catalogState = officialDownload.state;
            if (!officialDownload.message.empty()) {
                catalogState->message = officialDownload.message;
            }
        }
        if (officialDownload.ok) {
            IndexReadyOfficial(state.officialCatalog, state.library);
            state.status = "已下载 " + Asset::PickLocale(officialDownload.asset.name);
        } else if (!officialDownload.message.empty()) {
            state.status = officialDownload.message;
        }
    }
    if (aiResults != nullptr) {
        DrainAiResults(state, *aiResults);
    }
}

void MaybeRequestStoryboardThumbnail(AppState& state, Renderer::IRenderer& renderer) {
    if (!state.pendingThumbShotId.empty() || renderer.HasPendingReadback() ||
        !state.thumbScheduler.ShouldRun(NowMs())) {
        return;
    }
    Storyboard::ViewRect view;
    if (state.storyboardViewWidth > 1.0f && state.storyboardViewHeight > 1.0f) {
        view.x = -state.storyboardViewPanX / state.storyboardViewZoom;
        view.y = -state.storyboardViewPanY / state.storyboardViewZoom;
        view.w = state.storyboardViewWidth / state.storyboardViewZoom;
        view.h = state.storyboardViewHeight / state.storyboardViewZoom;
    } else {
        view.w = std::max(state.storyboard.Layout().contentWidth, 1.0f);
        view.h = std::max(state.storyboard.Layout().contentHeight, 1.0f);
    }
    const std::string shotId = state.storyboard.NextThumbnailShot(view);
    if (shotId.empty()) {
        return;
    }
    const std::string* cameraId = state.links.CameraForShot(shotId);
    if (cameraId == nullptr) {
        return;
    }
    Camera::CameraRig* rig = state.cameras.Find(*cameraId);
    if (rig == nullptr) {
        state.storyboard.MarkShotFailed(shotId);
        return;
    }
    state.storyboard.MarkShotRendering(shotId);
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = 320;
    target.height = 180;
    target.transparentBackground = false;
    renderer.RenderScene(BuildSceneView(state.scene, state.cameras.CurrentLight(), false),
                         rig->orbit.BuildView(320.0f / 180.0f), target);
    auto requested = renderer.RequestReadback(target);
    if (requested.IsOk()) {
        state.pendingThumbShotId = shotId;
        state.thumbScheduler.ConsumeFrame();
    } else {
        state.storyboard.MarkShotFailed(shotId);
    }
}

void DestroyLibraryPreviewTextures(AppState& state, Renderer::IRenderer& renderer) {
    for (auto& entry : state.libraryPreviewGpu) {
        if (entry.second.texture != 0xFFFFu) {
            renderer.DestroyRgbaTexture(entry.second.texture);
        }
    }
    state.libraryPreviewGpu.clear();
}

void SubmitOfficialRefresh(AppState& state, Platform::Worker& worker, Platform::IHttpClient* http,
                           Core::ResultQueue<OfficialRefreshResult>& results) {
    if (state.officialRefreshInFlight) {
        return;
    }
    if (!state.officialCatalog.IsConfigured() || http == nullptr) {
        state.officialCatalogStatus =
            state.officialCatalog.Manifest().assets.empty() ? "官方地址未配置" : "正在使用缓存清单";
        return;
    }
    state.officialRefreshInFlight = true;
    state.officialCatalogStatus = "正在刷新官方清单...";
    const std::string url = MakeOfficialEndpoints().manifestUrl;
    worker.Submit([http, url, &results]() {
        OfficialRefreshResult result;
        if (http == nullptr) {
            result.message = "网络未初始化";
            results.Push(std::move(result));
            return;
        }
        Platform::HttpGetRequest request;
        request.url = url;
        auto got = http->Get(request);
        if (!got.IsOk()) {
            result.message = got.GetError().userMessage;
            results.Push(std::move(result));
            return;
        }
        result.status = got.Value().status;
        result.ok = got.Value().status == 200;
        result.body.assign(got.Value().body.begin(), got.Value().body.end());
        if (!result.ok) {
            result.message = "HTTP 错误";
        }
        results.Push(std::move(result));
    });
}

void SubmitOfficialDownload(AppState& state, Platform::Worker& worker, Platform::IHttpClient* http,
                            const std::string& assetId,
                            Core::ResultQueue<OfficialDownloadJobResult>& downloadResults,
                            Core::ResultQueue<OfficialProgressUpdate>& progressResults) {
    const Asset::ManifestAsset* asset = state.officialCatalog.FindAsset(assetId);
    if (asset == nullptr || http == nullptr || state.officialCache.empty()) {
        state.status = "无法下载官方资产";
        return;
    }
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    state.officialCancels[assetId] = cancel;
    if (Asset::OfficialAssetState* catalogState = state.officialCatalog.MutableState(assetId)) {
        catalogState->status = Asset::OfficialDownloadStatus::Queued;
        catalogState->progress = 0.0f;
        catalogState->failure = Asset::OfficialFailureKind::None;
    }
    const Asset::OfficialEndpoints endpoints = MakeOfficialEndpoints();
    const std::string cache = state.officialCache;
    const Asset::ManifestAsset copied = *asset;
    worker.Submit([http, endpoints, cache, copied, cancel, &downloadResults, &progressResults]() {
        OfficialDownloadJobResult result;
        result.assetId = copied.id;
        result.asset = copied;
        if (http == nullptr) {
            result.message = "网络未初始化";
            downloadResults.Push(std::move(result));
            return;
        }
        auto done = Asset::DownloadOfficialFiles(
            cache, endpoints, copied, *http, cancel.get(),
            [&](float value) { progressResults.Push({copied.id, value}); });
        if (done.IsOk()) {
            result.ok = true;
            result.state = done.Value();
        } else {
            result.message = done.GetError().userMessage;
            result.state.status = result.message == Asset::OfficialCatalog::FailureMessage(
                                                        Asset::OfficialFailureKind::Cancelled)
                                      ? Asset::OfficialDownloadStatus::Cancelled
                                      : Asset::OfficialDownloadStatus::Failed;
        }
        downloadResults.Push(std::move(result));
    });
    state.status = "开始下载 " + Asset::PickLocale(asset->name);
}

} // namespace DirectorDesk::App
