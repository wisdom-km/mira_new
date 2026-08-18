#pragma once

#include "DirectorDesk/Core/Result.h"

#include <string>
#include <vector>

namespace DirectorDesk::App {

enum class StoredPathKind {
    ProjectRelative,
    Absolute,
};

enum class ProjectAssetSource {
    Project,
    UserLibrary,
    Official,
};

struct StoredPath {
    StoredPathKind kind = StoredPathKind::Absolute;
    std::string value;
};

struct ProjectAssetRef {
    std::string refId;
    ProjectAssetSource source = ProjectAssetSource::UserLibrary;
    std::string assetId;
    std::string version;
    std::string entrypoint;
    std::string path;
    std::string sha256;
};

struct ProjectNode {
    std::string id;
    std::string name;
    std::string assetRef;
    std::string parent;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    bool visible = true;
};

struct ProjectCamera {
    std::string id;
    std::string name;
    std::string projection = "perspective";
    float position[3] = {0.0f, 1.6f, 5.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float verticalFovDegrees = 50.0f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
    float orbitTarget[3] = {0.0f, 0.5f, 0.0f};
    float orbitDistance = 6.0f;
    float orbitYaw = 40.0f;
    float orbitPitch = 25.0f;
    bool hasOrbitNumbers = true;
    std::string preset;
};

struct ProjectShotLink {
    std::string shotId;
    std::string cameraId;
};

struct ProjectSnapshot {
    std::string projectId;
    std::string name = "未命名工程";
    StoredPath script;
    std::vector<ProjectAssetRef> assets;
    std::vector<ProjectNode> nodes;
    std::vector<ProjectCamera> cameras;
    std::string activeCamera;
    std::vector<ProjectShotLink> shotLinks;
    std::string lightingPreset = "neutral";
    std::string storyboardLayout = "left-to-right";
    std::vector<std::string> collapsedScenes;
    std::vector<std::string> diagnostics;
};

class ProjectFile {
public:
    static std::string MakeProjectId();
    static Core::Result<StoredPath> MakeStoredPath(const std::string& projectDir,
                                                   const std::string& utf8Path);
    static Core::Result<std::string> ResolveStoredPath(const std::string& projectDir,
                                                       const StoredPath& path);
    static Core::Result<std::string> Sha256File(const std::string& utf8Path);

    static Core::Result<ProjectSnapshot> Parse(const std::string& jsonText,
                                               const std::string& projectDir);
    static Core::Result<std::string> Serialize(const ProjectSnapshot& snapshot);
    static Core::Result<ProjectSnapshot> Load(const std::string& utf8Path);
    static Core::Result<void> Save(const std::string& utf8Path, const ProjectSnapshot& snapshot);
};

} // namespace DirectorDesk::App
