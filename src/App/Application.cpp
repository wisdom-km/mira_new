#include "DirectorDesk/App/Application.h"

#include "DirectorDesk/App/ProjectBinding.h"
#include "DirectorDesk/App/ProjectFile.h"
#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Asset/LoaderRegistry.h"
#include "DirectorDesk/Asset/ModelLoadResult.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/CommandQueue.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Platform/FileDialog.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Startup.h"
#include "DirectorDesk/Platform/Window.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/Renderer/IRenderer.h"
#include "DirectorDesk/Renderer/PngWriter.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"
#include "DirectorDesk/UI/LibraryPanel.h"
#include "DirectorDesk/UI/ScriptPanel.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include "CreateBgfxRenderer.h"
#include "ImGuiGlfwBackend.h"

#include <cstring>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

namespace DirectorDesk::App {
namespace {

struct LaunchOptions {
    bool exportAndQuit = false;
    std::string importPath;
    std::string scriptPath;
    std::string projectPath;
};

enum class PendingProjectAction {
    None,
    Quit,
    New,
    Open,
};

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

Camera::SubjectFrame SubjectFromScene(const Scene::Document& scene) {
    if (const Scene::Node* node = scene.Selected()) {
        return Camera::MakeSubject(node->transform.position, node->transform.scale);
    }
    return Camera::FallbackSubject();
}

Renderer::RenderSceneView BuildSceneView(const Scene::Document& scene,
                                         const Camera::LightState& light, bool showGroundGrid) {
    Renderer::RenderSceneView view;
    view.showTestMesh = scene.IsEmpty();
    view.showGroundGrid = showGroundGrid;
    view.light.direction = light.direction;
    view.light.color = light.color;
    for (const Scene::Node& node : scene.Nodes()) {
        Renderer::RenderMeshInstance instance;
        instance.modelId = node.gpuModelId;
        instance.world = node.transform.ToMatrix();
        instance.visible = node.visible;
        view.instances.push_back(instance);
    }
    return view;
}

bool HandleExportTestPng(Renderer::IRenderer& renderer, const Camera::OrbitCamera& camera,
                         const Renderer::RenderSceneView& sceneView, std::string& status) {
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = 1280;
    target.height = 720;
    target.transparentBackground = true;

    const float aspect = static_cast<float>(target.width) / static_cast<float>(target.height);
    renderer.BeginFrame(target.width, target.height);
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

void SubmitImport(Platform::Worker& worker, const Asset::LoaderRegistry& registry,
                  Core::ResultQueue<Asset::ModelLoadResult>& results, const std::string& path,
                  bool& importInProgress, std::string& status) {
    if (path.empty() || importInProgress) {
        if (importInProgress) {
            status = "A model is already loading";
        }
        return;
    }
    importInProgress = true;
    status = "Loading model...";
    worker.Submit([&registry, &results, path]() {
        Asset::ModelLoadResult result;
        result.sourcePath = path;
        auto loaded = registry.Load(path);
        result.ok = loaded.IsOk();
        if (loaded.IsOk()) {
            result.model = std::move(loaded.Value());
        } else {
            result.error = loaded.GetError();
        }
        results.Push(std::move(result));
    });
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

void IndexLibraryPath(Asset::Library& library, const std::string& path,
                      Asset::AssetOrigin origin) {
    if (path.empty() || !Platform::Paths::Exists(path)) {
        return;
    }
    auto imported = library.Import(path, origin);
    if (imported.IsOk()) {
        WriteLibraryPlaceholder(library, imported.Value());
    }
}

void ApplyLoadedModel(Asset::ModelLoadResult result, Scene::Document& scene,
                      Renderer::IRenderer& renderer, Asset::Library& library, std::string& status) {
    if (!result.ok) {
        status = result.error.userMessage;
        DD_LOG_ERROR("{}", result.error.technicalMessage);
        return;
    }

    auto uploaded = renderer.CreateModel(ToGpuModel(result.model));
    if (!uploaded.IsOk()) {
        status = uploaded.GetError().userMessage;
        DD_LOG_ERROR("{}", uploaded.GetError().technicalMessage);
        return;
    }

    Scene::Node node;
    node.id = scene.NextNodeId();
    node.name = result.model.name.empty() ? Platform::Paths::FileName(result.sourcePath)
                                          : result.model.name;
    node.gpuModelId = uploaded.Value();
    node.sourcePath = result.sourcePath;
    node.libraryAssetId = Asset::Library::MakeId(result.sourcePath);
    scene.Add(std::move(node));
    IndexLibraryPath(library, result.sourcePath, Asset::AssetOrigin::User);
    status = result.model.warnings.empty() ? "Imported " + scene.Selected()->name
                                           : result.model.warnings.front();
    DD_LOG_INFO("Imported model {} as {}", result.sourcePath, scene.Selected()->name);
}

void ReleaseSceneGpu(Scene::Document& scene, Renderer::IRenderer& renderer) {
    for (const Scene::Node& node : scene.Nodes()) {
        if (node.gpuModelId != 0) {
            renderer.DestroyModel(node.gpuModelId);
        }
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

void UploadSceneModels(Scene::Document& scene, Renderer::IRenderer& renderer,
                       const Asset::LoaderRegistry& registry, std::string& status) {
    for (const Scene::Node& item : scene.Nodes()) {
        Scene::Node* node = scene.Find(item.id);
        if (node == nullptr) {
            continue;
        }
        Asset::ModelData model;
        if (node->assetMissing || node->sourcePath.empty()) {
            model = MakePlaceholderBox();
            node->assetMissing = true;
        } else {
            auto loaded = registry.Load(node->sourcePath);
            if (!loaded.IsOk()) {
                model = MakePlaceholderBox();
                node->assetMissing = true;
                status = loaded.GetError().userMessage;
            } else {
                model = std::move(loaded.Value());
            }
        }
        auto uploaded = renderer.CreateModel(ToGpuModel(model));
        if (uploaded.IsOk()) {
            node->gpuModelId = uploaded.Value();
        }
    }
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

bool SaveProjectTo(const std::string& path, std::string& projectId, std::string& projectName,
                   std::string& projectPath, const Scene::Document& scene,
                   const Camera::CameraManager& cameras, const Link::Table& links,
                   const Script::Document& script, const Asset::Library& library,
                   const std::vector<std::string>& collapsedScenes, bool& projectDirty,
                   std::string& status) {
    if (path.empty()) {
        return false;
    }
    auto snapshot = CaptureProject(projectId, projectName, path, scene, cameras, links, script,
                                   library, collapsedScenes);
    auto saved = ProjectFile::Save(path, snapshot);
    if (!saved.IsOk()) {
        status = saved.GetError().userMessage;
        DD_LOG_ERROR("{}", saved.GetError().technicalMessage);
        return false;
    }
    projectId = snapshot.projectId;
    projectPath = path;
    projectDirty = false;
    status = "已保存工程";
    return true;
}

bool OpenProjectAt(const std::string& path, Scene::Document& scene, Camera::CameraManager& cameras,
                   Link::Table& links, Script::Document& script, Asset::Library& library,
                   Renderer::IRenderer& renderer, const Asset::LoaderRegistry& registry,
                   std::string& projectId, std::string& projectName, std::string& projectPath,
                   std::vector<std::string>& collapsedScenes, bool& projectDirty,
                   std::string& status) {
    auto loaded = ProjectFile::Load(path);
    if (!loaded.IsOk()) {
        status = loaded.GetError().userMessage;
        DD_LOG_ERROR("{}", loaded.GetError().technicalMessage);
        return false;
    }

    Scene::Document nextScene;
    Camera::CameraManager nextCameras;
    Link::Table nextLinks;
    Script::Document nextScript;
    std::vector<std::string> diagnostics;
    auto hydrated = HydrateProject(loaded.Value(), Platform::Paths::Parent(path), nextScene,
                                   nextCameras, nextLinks, nextScript, library, diagnostics);
    if (!hydrated.IsOk()) {
        status = hydrated.GetError().userMessage;
        DD_LOG_ERROR("{}", hydrated.GetError().technicalMessage);
        return false;
    }

    ReleaseSceneGpu(scene, renderer);
    scene.ReplaceNodes(nextScene.Nodes(), nextScene.SelectedId());
    cameras.Replace(nextCameras.Cameras(), nextCameras.SelectedId(), nextCameras.LightPreset());
    links.Replace(nextLinks.All());
    if (nextScript.Path().empty()) {
        script.Reset();
    } else {
        const auto loadedScript = script.LoadFromPath(nextScript.Path());
        if (!loadedScript.IsOk()) {
            script.Reset();
            status = loadedScript.GetError().userMessage;
        }
    }
    UploadSceneModels(scene, renderer, registry, status);
    projectId = loaded.Value().projectId;
    projectName = loaded.Value().name;
    projectPath = path;
    collapsedScenes.clear();
    std::unordered_set<std::string> sceneIds;
    if (script.HasPublishedSnapshot()) {
        for (const Script::Scene& item : script.PublishedSnapshot().scenes) {
            sceneIds.insert(item.id);
        }
    }
    for (const std::string& id : loaded.Value().collapsedScenes) {
        if (sceneIds.count(id) != 0) {
            collapsedScenes.push_back(id);
        }
    }
    projectDirty = false;
    status = diagnostics.empty() ? "已打开工程" : diagnostics.front();
    return true;
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
    status = "Loaded script " + Platform::Paths::FileName(path);
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
    status = "Saved script " + Platform::Paths::FileName(script.Path());
    DD_LOG_INFO("Saved script {}", script.Path());
}

} // namespace

int Application::Run(int argc, char** argv) {
    Platform::InitializeProcess();
    const LaunchOptions options = ParseOptions(argc, argv);

    auto logDir = Platform::Paths::LogDirectory();
    if (!logDir.IsOk()) {
        Platform::ShutdownProcess();
        return 1;
    }
    const auto created = Platform::Paths::CreateDirectories(logDir.Value());
    if (!created.IsOk()) {
        Platform::ShutdownProcess();
        return 1;
    }
    auto logInit = Core::Log::Init(logDir.Value());
    if (!logInit.IsOk()) {
        Platform::ShutdownProcess();
        return 1;
    }

    DD_LOG_INFO("DirectorDesk starting (Phase 5 local library)");

    auto exeDir = Platform::Paths::ExecutableDirectory();
    if (!exeDir.IsOk()) {
        DD_LOG_ERROR("{}", exeDir.GetError().technicalMessage);
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }
    const std::string shaderDirectory = Platform::Paths::Join(exeDir.Value(), "shaders");
    const std::string exampleObj =
        Platform::Paths::Join(Platform::Paths::Join(exeDir.Value(), "examples/models"), "cube.obj");
    const std::string exampleGlb =
        Platform::Paths::Join(Platform::Paths::Join(exeDir.Value(), "examples/models"), "cube.glb");
    const std::string exampleScript =
        Platform::Paths::Join(Platform::Paths::Join(exeDir.Value(), "examples/scripts"), "cafe.md");

    Platform::Window window;
    auto windowResult = window.Create(Platform::WindowDesc{});
    if (!windowResult.IsOk()) {
        DD_LOG_ERROR("{}", windowResult.GetError().technicalMessage);
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
    auto rendererInit = renderer->Init(rendererDesc);
    if (!rendererInit.IsOk()) {
        DD_LOG_ERROR("{}", rendererInit.GetError().technicalMessage);
        window.Destroy();
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }

    Camera::CameraManager cameras;
    Scene::Document scene;
    Script::Document script;
    Link::Table links;
    Asset::Library library;
    Asset::LoaderRegistry registry = Asset::CreateDefaultRegistry();
    auto libraryDir = Platform::Paths::LibraryDirectory();
    if (libraryDir.IsOk()) {
        auto opened = library.Open(libraryDir.Value());
        if (!opened.IsOk()) {
            DD_LOG_ERROR("{}", opened.GetError().technicalMessage);
        } else if (library.RecoveredFromCorruptIndex()) {
            DD_LOG_WARN("Library index was corrupt and was recovered");
        }
        IndexLibraryPath(library, exampleObj, Asset::AssetOrigin::Builtin);
        IndexLibraryPath(library, exampleGlb, Asset::AssetOrigin::Builtin);
    }
    if (options.exportAndQuit && options.importPath.empty()) {
        std::string status;
        const bool ok = HandleExportTestPng(
            *renderer, cameras.Selected()->orbit,
            BuildSceneView(scene, cameras.CurrentLight(), false), status);
        renderer->Shutdown();
        window.Destroy();
        DD_LOG_INFO("DirectorDesk exiting");
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return ok ? 0 : 1;
    }

    Backends::ImGuiGlfwBackend imgui;
    auto imguiResult = imgui.Init(window, shaderDirectory, 255);
    if (!imguiResult.IsOk()) {
        DD_LOG_ERROR("{}", imguiResult.GetError().technicalMessage);
        renderer->Shutdown();
        window.Destroy();
        Core::Log::Shutdown();
        Platform::ShutdownProcess();
        return 1;
    }

    Platform::Worker worker;
    worker.Start();
    Core::ResultQueue<Asset::ModelLoadResult> loadResults;
    UI::WorkspacePanel workspace;
    UI::ScriptPanel scriptPanel;
    UI::LibraryPanel libraryPanel;
    Core::CommandQueue commands;
    UI::AppViewState viewState;
    std::vector<UI::NodeView> nodeViews;
    std::vector<UI::ScriptSceneView> scriptScenes;
    std::vector<UI::ScriptDiagnosticView> scriptDiagnostics;
    std::vector<UI::CameraItemView> cameraViews;
    std::vector<UI::LibraryAssetView> libraryViews;
    std::string status;
    std::string librarySearch;
    std::string libraryOriginFilter = "all";
    std::string libraryViewMode = "list";
    std::string selectedLibraryAssetId;
    std::string projectId = ProjectFile::MakeProjectId();
    std::string projectName = "未命名工程";
    std::string projectPath;
    std::vector<std::string> collapsedScenes;
    std::string pendingOpenPath;
    PendingProjectAction pendingAction = PendingProjectAction::None;
    bool projectDirty = false;
    bool importInProgress = false;

    if (!options.importPath.empty()) {
        SubmitImport(worker, registry, loadResults, options.importPath, importInProgress, status);
    }
    if (!options.scriptPath.empty()) {
        ApplyScriptLoad(script, options.scriptPath, status);
    }
    if (!options.projectPath.empty()) {
        OpenProjectAt(options.projectPath, scene, cameras, links, script, library, *renderer,
                      registry, projectId, projectName, projectPath, collapsedScenes, projectDirty,
                      status);
    }

    const auto proceedPending = [&]() {
        const PendingProjectAction action = pendingAction;
        const std::string openPath = pendingOpenPath;
        pendingAction = PendingProjectAction::None;
        pendingOpenPath.clear();
        if (action == PendingProjectAction::New) {
            ResetProject(scene, cameras, links, script, *renderer, projectId, projectName,
                         projectPath, collapsedScenes, projectDirty);
            status = "已新建工程";
        } else if (action == PendingProjectAction::Open) {
            OpenProjectAt(openPath, scene, cameras, links, script, library, *renderer, registry,
                          projectId, projectName, projectPath, collapsedScenes, projectDirty,
                          status);
        } else if (action == PendingProjectAction::Quit) {
            window.RequestClose();
        }
    };

    const auto requestIfDirty = [&](PendingProjectAction action, std::string openPath = {}) {
        if (projectDirty) {
            pendingAction = action;
            pendingOpenPath = std::move(openPath);
            return;
        }
        pendingAction = action;
        pendingOpenPath = std::move(openPath);
        proceedPending();
    };

    while (true) {
        window.PollEvents();
        if (window.ShouldClose()) {
            if (projectDirty && pendingAction != PendingProjectAction::Quit) {
                window.CancelClose();
                pendingAction = PendingProjectAction::Quit;
            } else {
                break;
            }
        }

        Asset::ModelLoadResult loaded;
        while (loadResults.TryPop(loaded)) {
            importInProgress = false;
            ApplyLoadedModel(std::move(loaded), scene, *renderer, library, status);
            projectDirty = true;
        }

        Core::Command command;
        while (commands.TryPop(command)) {
            std::visit(
                [&](const auto& typed) {
                    using T = std::decay_t<decltype(typed)>;
                    if constexpr (std::is_same_v<T, Core::QuitCommand>) {
                        requestIfDirty(PendingProjectAction::Quit);
                    } else if constexpr (std::is_same_v<T, Core::ViewportResizeCommand>) {
                        renderer->SetViewportSize(typed.width, typed.height);
                    } else if constexpr (std::is_same_v<T, Core::OrbitDeltaCommand>) {
                        if (Camera::CameraRig* rig = cameras.Selected()) {
                            rig->orbit.Rotate(typed.rotateYaw, typed.rotatePitch);
                            rig->orbit.Pan(typed.panX, typed.panY);
                            rig->orbit.Zoom(typed.zoom);
                            projectDirty = true;
                        }
                    } else if constexpr (std::is_same_v<T, Core::ExportTestPngCommand>) {
                        if (const Camera::CameraRig* rig = cameras.Selected()) {
                            HandleExportTestPng(
                                *renderer, rig->orbit,
                                BuildSceneView(scene, cameras.CurrentLight(), false), status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::ImportModelCommand>) {
                        auto path = Platform::FileDialog::OpenModelFile();
                        if (!path.IsOk()) {
                            status = path.GetError().userMessage;
                            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
                        } else if (!path.Value().empty()) {
                            SubmitImport(worker, registry, loadResults, path.Value(),
                                         importInProgress, status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::ImportModelFromPathCommand>) {
                        SubmitImport(worker, registry, loadResults, typed.utf8Path,
                                     importInProgress, status);
                    } else if constexpr (std::is_same_v<T, Core::SelectNodeCommand>) {
                        scene.SetSelectedId(typed.nodeId);
                    } else if constexpr (std::is_same_v<T, Core::SetNodeTransformCommand>) {
                        if (Scene::Node* node = scene.Find(typed.nodeId)) {
                            node->transform.position =
                                glm::vec3(typed.position[0], typed.position[1], typed.position[2]);
                            node->transform.SetEulerDegrees(glm::vec3(typed.eulerDegrees[0],
                                                                      typed.eulerDegrees[1],
                                                                      typed.eulerDegrees[2]));
                            node->transform.scale =
                                glm::vec3(typed.scale[0], typed.scale[1], typed.scale[2]);
                            projectDirty = true;
                        }
                    } else if constexpr (std::is_same_v<T, Core::LoadScriptCommand>) {
                        auto path = Platform::FileDialog::OpenMarkdownFile();
                        if (!path.IsOk()) {
                            status = path.GetError().userMessage;
                            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
                        } else if (!path.Value().empty()) {
                            ApplyScriptLoad(script, path.Value(), status);
                            projectDirty = true;
                        }
                    } else if constexpr (std::is_same_v<T, Core::LoadScriptFromPathCommand>) {
                        ApplyScriptLoad(script, typed.utf8Path, status);
                        projectDirty = true;
                    } else if constexpr (std::is_same_v<T, Core::SaveScriptCommand>) {
                        HandleSaveScript(script, status);
                    } else if constexpr (std::is_same_v<T, Core::SetScriptTextCommand>) {
                        script.SetText(typed.text);
                    } else if constexpr (std::is_same_v<T, Core::InsertSceneCommand>) {
                        script.InsertScene();
                        status = "Added scene";
                    } else if constexpr (std::is_same_v<T, Core::InsertShotCommand>) {
                        script.InsertShot();
                        status = "Added shot";
                    } else if constexpr (std::is_same_v<T, Core::SelectShotCommand>) {
                        script.SelectShot(typed.shotId);
                        if (const std::string* cameraId = links.CameraForShot(typed.shotId)) {
                            if (cameras.Find(*cameraId) != nullptr) {
                                cameras.Select(*cameraId);
                            } else {
                                status = "关联相机已不存在";
                            }
                        }
                    } else if constexpr (std::is_same_v<T, Core::ApplyCameraPresetCommand>) {
                        Camera::CameraPresetKind kind = Camera::CameraPresetKind::Front;
                        if (Camera::TryParseCameraPreset(typed.presetId, kind)) {
                            cameras.ApplyPreset(kind, SubjectFromScene(scene));
                            projectDirty = true;
                            status = std::string("Applied camera preset ") + typed.presetId;
                        } else {
                            status = "Unknown camera preset";
                        }
                    } else if constexpr (std::is_same_v<T, Core::AddCameraCommand>) {
                        cameras.Add();
                        projectDirty = true;
                        status = "Added " + cameras.Selected()->name;
                    } else if constexpr (std::is_same_v<T, Core::RemoveCameraCommand>) {
                        if (!cameras.Remove(typed.cameraId)) {
                            status = "Cannot remove the last camera";
                        } else {
                            projectDirty = true;
                        }
                    } else if constexpr (std::is_same_v<T, Core::RenameCameraCommand>) {
                        cameras.Rename(typed.cameraId, typed.name);
                        projectDirty = true;
                    } else if constexpr (std::is_same_v<T, Core::SelectCameraCommand>) {
                        cameras.Select(typed.cameraId);
                    } else if constexpr (std::is_same_v<T, Core::SetLightPresetCommand>) {
                        Camera::LightPresetKind kind = Camera::LightPresetKind::Neutral;
                        if (Camera::TryParseLightPreset(typed.presetId, kind)) {
                            cameras.SetLightPreset(kind);
                            projectDirty = true;
                            status = std::string("Light preset ") + typed.presetId;
                        }
                    } else if constexpr (std::is_same_v<T, Core::AddLibraryAssetToSceneCommand>) {
                        const Asset::LibraryAsset* asset = library.Find(typed.assetId);
                        if (asset == nullptr) {
                            status = "Asset not found";
                        } else if (!asset->sourceExists || !Platform::Paths::Exists(asset->sourcePath)) {
                            status = "源文件已丢失";
                        } else {
                            selectedLibraryAssetId = asset->id;
                            SubmitImport(worker, registry, loadResults, asset->sourcePath,
                                         importInProgress, status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::SetLibrarySearchCommand>) {
                        librarySearch = typed.text;
                    } else if constexpr (std::is_same_v<T, Core::SetLibraryOriginFilterCommand>) {
                        libraryOriginFilter = typed.originFilter;
                    } else if constexpr (std::is_same_v<T, Core::SetLibraryViewModeCommand>) {
                        libraryViewMode = typed.viewMode;
                    } else if constexpr (std::is_same_v<T, Core::SelectLibraryAssetCommand>) {
                        selectedLibraryAssetId = typed.assetId;
                    } else if constexpr (std::is_same_v<T, Core::RefreshLibraryCommand>) {
                        library.Refresh();
                        status = "Library refreshed";
                    } else if constexpr (std::is_same_v<T, Core::NewProjectCommand>) {
                        requestIfDirty(PendingProjectAction::New);
                    } else if constexpr (std::is_same_v<T, Core::OpenProjectCommand>) {
                        auto path = Platform::FileDialog::OpenProjectFile();
                        if (!path.IsOk()) {
                            status = path.GetError().userMessage;
                        } else if (!path.Value().empty()) {
                            requestIfDirty(PendingProjectAction::Open, path.Value());
                        }
                    } else if constexpr (std::is_same_v<T, Core::OpenProjectFromPathCommand>) {
                        requestIfDirty(PendingProjectAction::Open, typed.utf8Path);
                    } else if constexpr (std::is_same_v<T, Core::SaveProjectCommand>) {
                        if (projectPath.empty()) {
                            auto path = Platform::FileDialog::SaveProjectFile();
                            if (path.IsOk() && !path.Value().empty()) {
                                SaveProjectTo(path.Value(), projectId, projectName, projectPath,
                                              scene, cameras, links, script, library,
                                              collapsedScenes, projectDirty, status);
                            }
                        } else {
                            SaveProjectTo(projectPath, projectId, projectName, projectPath, scene,
                                          cameras, links, script, library, collapsedScenes,
                                          projectDirty, status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::SaveProjectAsCommand>) {
                        auto path = Platform::FileDialog::SaveProjectFile();
                        if (path.IsOk() && !path.Value().empty()) {
                            SaveProjectTo(path.Value(), projectId, projectName, projectPath, scene,
                                          cameras, links, script, library, collapsedScenes,
                                          projectDirty, status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::LinkShotToCameraCommand>) {
                        const std::string shotId =
                            typed.shotId.empty() ? script.SelectedShotId() : typed.shotId;
                        const std::string cameraId =
                            typed.cameraId.empty() ? cameras.SelectedId() : typed.cameraId;
                        if (shotId.empty() || cameraId.empty()) {
                            status = "请先选择镜头和相机";
                        } else {
                            links.Set(shotId, cameraId);
                            projectDirty = true;
                            status = "已关联镜头与相机";
                        }
                    } else if constexpr (std::is_same_v<T, Core::UnlinkShotCommand>) {
                        const std::string shotId =
                            typed.shotId.empty() ? script.SelectedShotId() : typed.shotId;
                        links.ClearShot(shotId);
                        projectDirty = true;
                        status = "已取消镜头关联";
                    } else if constexpr (std::is_same_v<T, Core::ConfirmSaveProjectCommand>) {
                        bool saved = false;
                        if (projectPath.empty()) {
                            auto path = Platform::FileDialog::SaveProjectFile();
                            saved = path.IsOk() && !path.Value().empty() &&
                                    SaveProjectTo(path.Value(), projectId, projectName, projectPath,
                                                  scene, cameras, links, script, library,
                                                  collapsedScenes, projectDirty, status);
                        } else {
                            saved = SaveProjectTo(projectPath, projectId, projectName, projectPath,
                                                  scene, cameras, links, script, library,
                                                  collapsedScenes, projectDirty, status);
                        }
                        if (saved) {
                            proceedPending();
                        }
                    } else if constexpr (std::is_same_v<T, Core::DiscardProjectCommand>) {
                        projectDirty = false;
                        proceedPending();
                    } else if constexpr (std::is_same_v<T, Core::CancelProjectPromptCommand>) {
                        pendingAction = PendingProjectAction::None;
                        pendingOpenPath.clear();
                    }
                },
                command);
        }

        nodeViews.clear();
        for (const Scene::Node& node : scene.Nodes()) {
            UI::NodeView item;
            item.id = node.id;
            item.name = node.name;
            item.position[0] = node.transform.position.x;
            item.position[1] = node.transform.position.y;
            item.position[2] = node.transform.position.z;
            const glm::vec3 euler = node.transform.EulerDegrees();
            item.eulerDegrees[0] = euler.x;
            item.eulerDegrees[1] = euler.y;
            item.eulerDegrees[2] = euler.z;
            item.scale[0] = node.transform.scale.x;
            item.scale[1] = node.transform.scale.y;
            item.scale[2] = node.transform.scale.z;
            item.selected = node.id == scene.SelectedId();
            nodeViews.push_back(std::move(item));
        }

        const auto size = window.GetFramebufferSize();
        viewState.windowWidth = size.width;
        viewState.windowHeight = size.height;
        viewState.viewportTextureIndex = renderer->ViewportTextureIndex();
        viewState.viewportTextureWidth = renderer->ViewportWidth();
        viewState.viewportTextureHeight = renderer->ViewportHeight();
        viewState.statusText = status.c_str();
        viewState.importInProgress = importInProgress;
        viewState.nodes = &nodeViews;
        viewState.exampleObjPath = Platform::Paths::Exists(exampleObj) ? exampleObj.c_str() : "";
        viewState.exampleGlbPath = Platform::Paths::Exists(exampleGlb) ? exampleGlb.c_str() : "";
        viewState.exampleScriptPath =
            Platform::Paths::Exists(exampleScript) ? exampleScript.c_str() : "";

        scriptScenes.clear();
        if (script.HasPublishedSnapshot()) {
            for (const Script::Scene& sceneItem : script.PublishedSnapshot().scenes) {
                UI::ScriptSceneView sceneView;
                sceneView.id = sceneItem.id;
                sceneView.title = sceneItem.title;
                for (const Script::Shot& shot : sceneItem.shots) {
                    UI::ScriptShotView shotView;
                    shotView.id = shot.id;
                    shotView.title = shot.title;
                    shotView.selected = shot.id == script.SelectedShotId();
                    if (const std::string* cameraId = links.CameraForShot(shot.id)) {
                        shotView.linkedCameraId = *cameraId;
                        if (const Camera::CameraRig* rig = cameras.Find(*cameraId)) {
                            shotView.linkedCameraName = rig->name;
                        } else {
                            shotView.linkedCameraName = *cameraId;
                            shotView.linkedMissing = true;
                        }
                    }
                    sceneView.shots.push_back(std::move(shotView));
                }
                scriptScenes.push_back(std::move(sceneView));
            }
        }
        scriptDiagnostics.clear();
        for (const Script::Diagnostic& diagnostic : script.Diagnostics()) {
            UI::ScriptDiagnosticView item;
            item.severity = DiagnosticSeverityText(diagnostic.severity);
            item.line = diagnostic.line;
            item.code = diagnostic.code.c_str();
            item.message = diagnostic.message.c_str();
            scriptDiagnostics.push_back(item);
        }

        viewState.scriptText = script.Text().c_str();
        viewState.scriptPath = script.Path().c_str();
        viewState.scriptDirty = script.IsDirty();
        viewState.scriptHasSnapshot = script.HasPublishedSnapshot();
        viewState.scriptExternalRevision = script.ExternalRevision();
        viewState.scriptScenes = &scriptScenes;
        viewState.scriptDiagnostics = &scriptDiagnostics;

        cameraViews.clear();
        for (const Camera::CameraRig& rig : cameras.Cameras()) {
            UI::CameraItemView item;
            item.id = rig.id;
            item.name = rig.name;
            item.selected = rig.id == cameras.SelectedId();
            cameraViews.push_back(std::move(item));
        }
        viewState.cameras = &cameraViews;
        viewState.lightPresetId = Camera::LightPresetId(cameras.LightPreset());

        libraryViews.clear();
        for (const Asset::LibraryAsset& asset : library.Query(librarySearch, libraryOriginFilter)) {
            UI::LibraryAssetView item;
            item.id = asset.id;
            item.name = asset.name;
            item.format = asset.format;
            item.origin = Asset::Library::OriginId(asset.origin);
            item.missing = !asset.sourceExists;
            item.status = asset.sourceExists ? "就绪" : "缺失";
            item.selected = asset.id == selectedLibraryAssetId;
            libraryViews.push_back(std::move(item));
        }
        viewState.libraryAssets = &libraryViews;
        viewState.librarySearch = librarySearch.c_str();
        viewState.libraryOriginFilter = libraryOriginFilter.c_str();
        viewState.libraryViewMode = libraryViewMode.c_str();
        viewState.projectName = projectName.c_str();
        viewState.projectPath = projectPath.c_str();
        viewState.projectDirty = projectDirty;
        viewState.projectPromptVisible = pendingAction != PendingProjectAction::None;
        if (const std::string* cameraId = links.CameraForShot(script.SelectedShotId())) {
            if (const Camera::CameraRig* rig = cameras.Find(*cameraId)) {
                viewState.selectedShotLinkedCamera = rig->name.c_str();
            } else {
                viewState.selectedShotLinkedCamera = cameraId->c_str();
            }
        } else {
            viewState.selectedShotLinkedCamera = "";
        }

        const float aspect = renderer->ViewportHeight() == 0
                                 ? 1.0f
                                 : static_cast<float>(renderer->ViewportWidth()) /
                                       static_cast<float>(renderer->ViewportHeight());
        renderer->BeginFrame(size.width, size.height);
        renderer->RenderScene(BuildSceneView(scene, cameras.CurrentLight(), true),
                              cameras.Selected()->orbit.BuildView(aspect),
                              Renderer::RenderTargetDesc{});
        imgui.BeginFrame();
        workspace.Draw(viewState, commands);
        scriptPanel.Draw(viewState, commands);
        libraryPanel.Draw(viewState, commands);
        imgui.Submit(size.width, size.height);
        renderer->EndFrame();
    }

    worker.Shutdown();
    imgui.Shutdown();
    renderer->Shutdown();
    window.Destroy();
    DD_LOG_INFO("DirectorDesk exiting");
    Core::Log::Shutdown();
    Platform::ShutdownProcess();
    return 0;
}

} // namespace DirectorDesk::App
