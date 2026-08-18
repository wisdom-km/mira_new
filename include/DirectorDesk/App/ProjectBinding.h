#pragma once

#include "DirectorDesk/App/ProjectFile.h"
#include "DirectorDesk/Core/Result.h"

#include <string>
#include <vector>

namespace DirectorDesk::Asset {
class Library;
}

namespace DirectorDesk::Camera {
class CameraManager;
}

namespace DirectorDesk::Link {
class Table;
}

namespace DirectorDesk::Scene {
class Document;
}

namespace DirectorDesk::Script {
class Document;
}

namespace DirectorDesk::App {

ProjectSnapshot CaptureProject(const std::string& projectId, const std::string& name,
                               const std::string& projectPath, const Scene::Document& scene,
                               const Camera::CameraManager& cameras, const Link::Table& links,
                               const Script::Document& script, const Asset::Library& library,
                               const std::vector<std::string>& collapsedScenes);

Core::Result<void> HydrateProject(const ProjectSnapshot& snapshot, const std::string& projectDir,
                                  Scene::Document& scene, Camera::CameraManager& cameras,
                                  Link::Table& links, Script::Document& script,
                                  const Asset::Library& library, std::vector<std::string>& diagnostics);

} // namespace DirectorDesk::App
