#include "DirectorDesk/App/ProjectBinding.h"

#include "DirectorDesk/Asset/Library.h"
#include "DirectorDesk/Camera/CameraManager.h"
#include "DirectorDesk/Camera/Presets.h"
#include "DirectorDesk/Link/ShotLink.h"
#include "DirectorDesk/Platform/Paths.h"
#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Script/Document.h"

#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <unordered_set>

namespace DirectorDesk::App {
namespace {

void Copy3(float out[3], const glm::vec3& value) {
    out[0] = value.x;
    out[1] = value.y;
    out[2] = value.z;
}

void CopyQuat(float out[4], const glm::quat& value) {
    out[0] = value.x;
    out[1] = value.y;
    out[2] = value.z;
    out[3] = value.w;
}

std::string MakeAssetRefId(std::uint32_t& index) {
    return "assetref-" + std::to_string(index++);
}

const ProjectAssetRef* FindAsset(const ProjectSnapshot& snapshot, const std::string& refId) {
    for (const ProjectAssetRef& asset : snapshot.assets) {
        if (asset.refId == refId) {
            return &asset;
        }
    }
    return nullptr;
}

} // namespace

ProjectSnapshot CaptureProject(const std::string& projectId, const std::string& name,
                               const std::string& projectPath, const Scene::Document& scene,
                               const Camera::CameraManager& cameras, const Link::Table& links,
                               const Script::Document& script, const Asset::Library& library,
                               const std::vector<std::string>& collapsedScenes) {
    ProjectSnapshot snapshot;
    snapshot.projectId = projectId.empty() ? ProjectFile::MakeProjectId() : projectId;
    snapshot.name = name.empty() ? "未命名工程" : name;
    snapshot.collapsedScenes = collapsedScenes;
    snapshot.lightingPreset = Camera::LightPresetId(cameras.LightPreset());
    snapshot.activeCamera = cameras.SelectedId();
    const std::string projectDir =
        projectPath.empty() ? std::string() : Platform::Paths::Parent(projectPath);

    if (!script.Path().empty()) {
        auto stored = ProjectFile::MakeStoredPath(projectDir, script.Path());
        if (stored.IsOk()) {
            snapshot.script = stored.Value();
        }
    }

    std::unordered_map<std::string, std::string> sourceToRef;
    std::uint32_t assetIndex = 1;
    for (const Scene::Node& node : scene.Nodes()) {
        const std::string key =
            !node.libraryAssetId.empty()
                ? node.libraryAssetId
                : (!node.sourcePath.empty() ? Platform::Paths::StableKey(node.sourcePath)
                                            : node.assetRef);
        if (key.empty()) {
            continue;
        }
        if (sourceToRef.count(key) != 0) {
            continue;
        }
        ProjectAssetRef asset;
        asset.refId = node.assetRef.empty() ? MakeAssetRefId(assetIndex) : node.assetRef;
        if (!projectDir.empty() && !node.sourcePath.empty() &&
            Platform::Paths::IsWithin(projectDir, node.sourcePath)) {
            auto relative = Platform::Paths::RelativeTo(projectDir, node.sourcePath);
            auto hash = ProjectFile::Sha256File(node.sourcePath);
            if (relative.IsOk() && hash.IsOk()) {
                asset.source = ProjectAssetSource::Project;
                asset.path = relative.Value();
                asset.sha256 = hash.Value();
                sourceToRef[key] = asset.refId;
                snapshot.assets.push_back(std::move(asset));
                continue;
            }
        }
        asset.source = ProjectAssetSource::UserLibrary;
        asset.assetId =
            node.libraryAssetId.empty() ? Asset::Library::MakeId(node.sourcePath) : node.libraryAssetId;
        if (!node.sourcePath.empty()) {
            auto hash = ProjectFile::Sha256File(node.sourcePath);
            if (hash.IsOk()) {
                asset.sha256 = hash.Value();
            }
            asset.path = Platform::Paths::NormalizeSlashes(node.sourcePath);
        }
        if (library.Find(asset.assetId) == nullptr && node.sourcePath.empty()) {
            asset.source = ProjectAssetSource::Official;
            asset.version = "0.0.0";
            asset.entrypoint = "model/missing.glb";
        }
        sourceToRef[key] = asset.refId;
        snapshot.assets.push_back(std::move(asset));
    }

    for (const Scene::Node& node : scene.Nodes()) {
        ProjectNode item;
        item.id = node.id;
        item.name = node.name;
        item.parent = node.parent;
        item.visible = node.visible;
        const std::string key =
            !node.libraryAssetId.empty()
                ? node.libraryAssetId
                : (!node.sourcePath.empty() ? Platform::Paths::StableKey(node.sourcePath)
                                            : node.assetRef);
        if (sourceToRef.count(key) != 0) {
            item.assetRef = sourceToRef[key];
        } else {
            item.assetRef = node.assetRef;
        }
        Copy3(item.position, node.transform.position);
        CopyQuat(item.rotation, node.transform.rotation);
        Copy3(item.scale, node.transform.scale);
        snapshot.nodes.push_back(std::move(item));
    }

    for (const Camera::CameraRig& rig : cameras.Cameras()) {
        ProjectCamera camera;
        camera.id = rig.id;
        camera.name = rig.name;
        camera.preset = rig.lastPreset;
        const glm::vec3 position = rig.orbit.Position();
        const glm::vec3 target = rig.orbit.Target();
        Copy3(camera.position, position);
        Copy3(camera.orbitTarget, target);
        camera.verticalFovDegrees = rig.orbit.FovYDegrees();
        camera.nearPlane = rig.orbit.NearPlane();
        camera.farPlane = rig.orbit.FarPlane();
        camera.orbitDistance = rig.orbit.Distance();
        camera.orbitYaw = rig.orbit.YawDegrees();
        camera.orbitPitch = rig.orbit.PitchDegrees();
        camera.hasOrbitNumbers = true;
        snapshot.cameras.push_back(std::move(camera));
    }

    for (const Link::ShotLink& link : links.All()) {
        snapshot.shotLinks.push_back(ProjectShotLink{link.shotId, link.cameraId});
    }
    return snapshot;
}

Core::Result<void> HydrateProject(const ProjectSnapshot& snapshot, const std::string& projectDir,
                                  Scene::Document& scene, Camera::CameraManager& cameras,
                                  Link::Table& links, Script::Document& script,
                                  const Asset::Library& library, std::vector<std::string>& diagnostics) {
    diagnostics = snapshot.diagnostics;
    script.Reset();
    if (!snapshot.script.value.empty()) {
        auto resolved = ProjectFile::ResolveStoredPath(projectDir, snapshot.script);
        if (!resolved.IsOk() || resolved.Value().empty() ||
            !Platform::Paths::Exists(resolved.Value())) {
            diagnostics.emplace_back("剧本文件缺失，工程仍可打开");
        } else {
            const auto loaded = script.LoadFromPath(resolved.Value());
            if (!loaded.IsOk()) {
                script.Reset();
                diagnostics.push_back(loaded.GetError().userMessage);
            }
        }
    }

    std::unordered_set<std::string> sceneIds;
    if (script.HasPublishedSnapshot()) {
        for (const Script::Scene& item : script.PublishedSnapshot().scenes) {
            sceneIds.insert(item.id);
        }
    }

    std::vector<Scene::Node> nodes;
    for (const ProjectNode& item : snapshot.nodes) {
        Scene::Node node;
        node.id = item.id;
        node.name = item.name;
        node.parent = item.parent;
        node.assetRef = item.assetRef;
        node.visible = item.visible;
        node.transform.position = glm::vec3(item.position[0], item.position[1], item.position[2]);
        node.transform.rotation =
            glm::quat(item.rotation[3], item.rotation[0], item.rotation[1], item.rotation[2]);
        node.transform.scale = glm::vec3(item.scale[0], item.scale[1], item.scale[2]);
        if (const ProjectAssetRef* asset = FindAsset(snapshot, item.assetRef)) {
            if (asset->source == ProjectAssetSource::Project) {
                StoredPath path;
                path.kind = StoredPathKind::ProjectRelative;
                path.value = asset->path;
                auto resolved = ProjectFile::ResolveStoredPath(projectDir, path);
                if (resolved.IsOk() && Platform::Paths::Exists(resolved.Value())) {
                    node.sourcePath = resolved.Value();
                } else {
                    node.assetMissing = true;
                    diagnostics.emplace_back("工程资产缺失：" + asset->path);
                }
            } else if (asset->source == ProjectAssetSource::UserLibrary) {
                node.libraryAssetId = asset->assetId;
                if (const Asset::LibraryAsset* found = library.Find(asset->assetId)) {
                    if (found->sourceExists && Platform::Paths::Exists(found->sourcePath)) {
                        node.sourcePath = found->sourcePath;
                    } else {
                        node.assetMissing = true;
                        diagnostics.emplace_back("资源库资产缺失：" + asset->assetId);
                    }
                } else if (!asset->path.empty() && Platform::Paths::Exists(asset->path)) {
                    node.sourcePath = asset->path;
                } else {
                    node.assetMissing = true;
                    diagnostics.emplace_back("资源库资产缺失：" + asset->assetId);
                }
            } else {
                node.assetMissing = true;
                diagnostics.emplace_back("官方资产尚未下载：" + asset->assetId);
            }
        } else if (!item.assetRef.empty()) {
            node.assetMissing = true;
            diagnostics.emplace_back("资产引用不存在：" + item.assetRef);
        }
        nodes.push_back(std::move(node));
    }
    scene.ReplaceNodes(std::move(nodes), {});

    std::vector<Camera::CameraRig> rigs;
    for (const ProjectCamera& item : snapshot.cameras) {
        Camera::CameraRig rig;
        rig.id = item.id;
        rig.name = item.name;
        rig.lastPreset = item.preset;
        if (!rig.orbit.Restore(glm::vec3(item.orbitTarget[0], item.orbitTarget[1], item.orbitTarget[2]),
                               glm::vec3(item.position[0], item.position[1], item.position[2]),
                               item.verticalFovDegrees, item.nearPlane, item.farPlane,
                               item.orbitDistance, item.orbitYaw, item.orbitPitch,
                               item.hasOrbitNumbers)) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::ParseFailure, "Camera restore failed", "无法恢复相机"));
        }
        rigs.push_back(std::move(rig));
    }
    Camera::LightPresetKind light = Camera::LightPresetKind::Neutral;
    if (!Camera::TryParseLightPreset(snapshot.lightingPreset, light)) {
        light = Camera::LightPresetKind::Neutral;
    }
    cameras.Replace(std::move(rigs), snapshot.activeCamera, light);

    std::vector<Link::ShotLink> restored;
    for (const ProjectShotLink& item : snapshot.shotLinks) {
        restored.push_back(Link::ShotLink{item.shotId, item.cameraId});
    }
    links.Replace(std::move(restored));
    (void)sceneIds;
    return Core::Result<void>::Ok();
}

} // namespace DirectorDesk::App
