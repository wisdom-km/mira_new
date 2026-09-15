// DispatchScene: Scene / import Command family (FND-10).
#include "DirectorDesk/App/CommandDispatch.h"

#include "AppInternals.h"
#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"

#include <cstdint>
#include <glm/vec3.hpp>
#include <variant>

namespace DirectorDesk::App {

bool TryDispatchScene(AppState& state, const Core::Command& command, DispatchServices& services) {
    if (std::holds_alternative<Core::ImportModelCommand>(command)) {
        if (!services.openModelFile) {
            return true;
        }
        auto path = services.openModelFile();
        if (!path.IsOk()) {
            state.status = path.GetError().userMessage;
            DD_LOG_ERROR("{}", path.GetError().technicalMessage);
        } else if (!path.Value().empty()) {
            if (IsSkillPath(path.Value())) {
                InstallSkill(state, path.Value());
            } else if (services.submitImport) {
                services.submitImport(path.Value());
            }
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::ImportModelFromPathCommand>(&command)) {
        if (IsSkillPath(typed->utf8Path)) {
            InstallSkill(state, typed->utf8Path);
        } else if (services.submitImport) {
            services.submitImport(typed->utf8Path);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SelectNodeCommand>(&command)) {
        state.scene.SetSelectedId(typed->nodeId);
        state.selectedLibraryAssetId.clear();
        state.selectionKind = "node";
        state.selectionId = typed->nodeId;
        state.selectionLabel = typed->nodeId;
        if (const Scene::Node* node = state.scene.Find(typed->nodeId)) {
            state.selectionLabel = node->name;
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetNodeTransformCommand>(&command)) {
        if (Scene::Node* node = state.scene.Find(typed->nodeId)) {
            node->transform.position =
                glm::vec3(typed->position[0], typed->position[1], typed->position[2]);
            node->transform.SetEulerDegrees(glm::vec3(typed->eulerDegrees[0], typed->eulerDegrees[1],
                                                        typed->eulerDegrees[2]));
            node->transform.scale = glm::vec3(typed->scale[0], typed->scale[1], typed->scale[2]);
            state.projectDirty = true;
            state.storyboard.MarkLinkedStale();
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::DeleteNodeCommand>(&command)) {
        Scene::Node* node = state.scene.Find(typed->nodeId);
        if (node == nullptr) {
            state.status = "找不到该对象";
            return true;
        }
        const std::string name = node->name;
        const bool wasSelected = state.scene.SelectedId() == typed->nodeId ||
                                 (state.selectionKind == "node" && state.selectionId == typed->nodeId);
        ReleaseGpuModel(state, services.renderer, node->gpuModelId);
        if (!state.scene.Remove(typed->nodeId)) {
            return true;
        }
        state.projectDirty = true;
        state.storyboard.MarkLinkedStale();
        state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
        if (wasSelected) {
            state.selectionKind = "none";
            state.selectionId.clear();
            state.selectionLabel.clear();
        }
        state.status = "已删除 " + name;
        return true;
    }
    if (const auto* typed = std::get_if<Core::DuplicateNodeCommand>(&command)) {
        const Scene::Node* source = state.scene.Find(typed->nodeId);
        if (source == nullptr) {
            state.status = "找不到该对象";
            return true;
        }
        const std::uint32_t sharedGpu = source->gpuModelId;
        const std::string newId = state.scene.Duplicate(typed->nodeId);
        if (newId.empty()) {
            return true;
        }
        RetainGpuModel(state, sharedGpu);
        state.projectDirty = true;
        state.storyboard.MarkLinkedStale();
        state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
        state.selectedLibraryAssetId.clear();
        state.selectionKind = "node";
        state.selectionId = newId;
        state.selectionLabel = newId;
        if (const Scene::Node* copy = state.scene.Find(newId)) {
            state.selectionLabel = copy->name;
        }
        state.status = "已复制为 " + state.selectionLabel;
        return true;
    }
    if (const auto* typed = std::get_if<Core::SetNodeVisibleCommand>(&command)) {
        if (state.scene.SetVisible(typed->nodeId, typed->visible)) {
            state.projectDirty = true;
            state.storyboard.MarkLinkedStale();
            state.thumbScheduler.NotifyBusy(services.nowMs ? services.nowMs() : 0);
        }
        return true;
    }
    if (const auto* typed = std::get_if<Core::AddLibraryAssetToSceneCommand>(&command)) {
        const Asset::LibraryAsset* asset = state.library.Find(typed->assetId);
        if (asset == nullptr) {
            state.status = "找不到该资产";
        } else if (asset->format == "skill") {
            state.status = "Skill 不是模型";
        } else if (const Asset::ManifestAsset* official =
                       state.officialCatalog.FindAsset(typed->assetId);
                   official != nullptr && official->kind == "skill") {
            state.status = "Skill 不是模型";
        } else if (!asset->sourceExists || !Platform::Paths::Exists(asset->sourcePath)) {
            state.status = "源文件已丢失";
        } else {
            state.selectedLibraryAssetId = asset->id;
            if (services.submitImport) {
                services.submitImport(asset->sourcePath);
            }
        }
        return true;
    }
    return false;
}

} // namespace DirectorDesk::App
