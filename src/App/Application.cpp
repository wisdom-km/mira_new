#include "DirectorDesk/App/Application.h"

#include "DirectorDesk/Asset/LoaderRegistry.h"
#include "DirectorDesk/Asset/ModelLoadResult.h"
#include "DirectorDesk/Camera/OrbitCamera.h"
#include "DirectorDesk/Core/Command.h"
#include "DirectorDesk/Core/CommandQueue.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Core/ResultQueue.h"
#include "DirectorDesk/Platform/FileDialog.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Platform/Startup.h"
#include "DirectorDesk/Platform/Window.h"
#include "DirectorDesk/Platform/Worker.h"
#include "DirectorDesk/Renderer/IRenderer.h"
#include "DirectorDesk/Renderer/PngWriter.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"
#include "DirectorDesk/UI/ScriptPanel.h"
#include "DirectorDesk/UI/WorkspacePanel.h"

#include "CreateBgfxRenderer.h"
#include "ImGuiGlfwBackend.h"

#include <cstring>
#include <glm/vec3.hpp>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace DirectorDesk::App {
namespace {

struct LaunchOptions {
    bool exportAndQuit = false;
    std::string importPath;
    std::string scriptPath;
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

Renderer::RenderSceneView BuildSceneView(const Scene::Document& scene) {
    Renderer::RenderSceneView view;
    view.showTestMesh = scene.IsEmpty();
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

void ApplyLoadedModel(Asset::ModelLoadResult result, Scene::Document& scene,
                      Renderer::IRenderer& renderer, std::string& status) {
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
    scene.Add(std::move(node));
    status = result.model.warnings.empty() ? "Imported " + scene.Selected()->name
                                           : result.model.warnings.front();
    DD_LOG_INFO("Imported model {} as {}", result.sourcePath, scene.Selected()->name);
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

    DD_LOG_INFO("DirectorDesk starting (Phase 3 script)");

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

    Camera::OrbitCamera camera;
    Scene::Document scene;
    Script::Document script;
    Asset::LoaderRegistry registry = Asset::CreateDefaultRegistry();
    if (options.exportAndQuit && options.importPath.empty()) {
        std::string status;
        const bool ok = HandleExportTestPng(*renderer, camera, BuildSceneView(scene), status);
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
    Core::CommandQueue commands;
    UI::AppViewState viewState;
    std::vector<UI::NodeView> nodeViews;
    std::vector<UI::ScriptSceneView> scriptScenes;
    std::vector<UI::ScriptDiagnosticView> scriptDiagnostics;
    std::string status;
    bool importInProgress = false;

    if (!options.importPath.empty()) {
        SubmitImport(worker, registry, loadResults, options.importPath, importInProgress, status);
    }
    if (!options.scriptPath.empty()) {
        ApplyScriptLoad(script, options.scriptPath, status);
    }

    while (!window.ShouldClose()) {
        window.PollEvents();

        Asset::ModelLoadResult loaded;
        while (loadResults.TryPop(loaded)) {
            importInProgress = false;
            ApplyLoadedModel(std::move(loaded), scene, *renderer, status);
        }

        Core::Command command;
        while (commands.TryPop(command)) {
            std::visit(
                [&](const auto& typed) {
                    using T = std::decay_t<decltype(typed)>;
                    if constexpr (std::is_same_v<T, Core::QuitCommand>) {
                        window.RequestClose();
                    } else if constexpr (std::is_same_v<T, Core::ViewportResizeCommand>) {
                        renderer->SetViewportSize(typed.width, typed.height);
                    } else if constexpr (std::is_same_v<T, Core::OrbitDeltaCommand>) {
                        camera.Rotate(typed.rotateYaw, typed.rotatePitch);
                        camera.Pan(typed.panX, typed.panY);
                        camera.Zoom(typed.zoom);
                    } else if constexpr (std::is_same_v<T, Core::ExportTestPngCommand>) {
                        HandleExportTestPng(*renderer, camera, BuildSceneView(scene), status);
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
                        }
                    } else if constexpr (std::is_same_v<T, Core::LoadScriptCommand>) {
                        auto path = Platform::FileDialog::OpenMarkdownFile();
                        if (!path.IsOk()) {
                            status = path.GetError().userMessage;
                            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
                        } else if (!path.Value().empty()) {
                            ApplyScriptLoad(script, path.Value(), status);
                        }
                    } else if constexpr (std::is_same_v<T, Core::LoadScriptFromPathCommand>) {
                        ApplyScriptLoad(script, typed.utf8Path, status);
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

        const float aspect = renderer->ViewportHeight() == 0
                                 ? 1.0f
                                 : static_cast<float>(renderer->ViewportWidth()) /
                                       static_cast<float>(renderer->ViewportHeight());
        renderer->BeginFrame(size.width, size.height);
        renderer->RenderScene(BuildSceneView(scene), camera.BuildView(aspect),
                              Renderer::RenderTargetDesc{});
        imgui.BeginFrame();
        workspace.Draw(viewState, commands);
        scriptPanel.Draw(viewState, commands);
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
