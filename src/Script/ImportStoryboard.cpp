// ImportStoryboard: Implementation for storyboard-import 1 (FND-30).
#include "DirectorDesk/Script/ImportStoryboard.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Script/Ids.h"

#include <nlohmann/json.hpp>

#include <unordered_set>
#include <utility>

namespace DirectorDesk::Script {
namespace {

using OrderedJson = nlohmann::ordered_json;

void CollectIds(const Snapshot& snapshot, std::unordered_set<std::string>& sceneIds,
                std::unordered_set<std::string>& shotIds) {
    for (const Scene& scene : snapshot.scenes) {
        if (!scene.id.empty()) {
            sceneIds.insert(scene.id);
        }
        for (const Shot& shot : scene.shots) {
            if (!shot.id.empty()) {
                shotIds.insert(shot.id);
            }
        }
    }
}

std::string JsonToString(const OrderedJson& value) {
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_null()) {
        return {};
    }
    if (value.is_boolean() || value.is_number()) {
        return value.dump();
    }
    return value.dump();
}

std::string ResolveId(const std::string& requested, bool scene, std::unordered_set<std::string>& used,
                      std::vector<std::string>& diagnostics) {
    if (IsValidId(requested) && used.count(requested) == 0) {
        used.insert(requested);
        return requested;
    }
    std::string generated = scene ? GenerateSceneId() : GenerateShotId();
    while (used.count(generated) != 0) {
        generated = scene ? GenerateSceneId() : GenerateShotId();
    }
    used.insert(generated);
    if (requested.empty()) {
        diagnostics.push_back(std::string("已为") + (scene ? "场次" : "镜头") + "生成 ID " + generated);
    } else if (!IsValidId(requested)) {
        diagnostics.push_back("非法 ID \"" + requested + "\" 已替换为 " + generated);
    } else {
        diagnostics.push_back("冲突 ID \"" + requested + "\" 已替换为 " + generated);
    }
    return generated;
}

void AppendMeta(std::string& markdown, const OrderedJson& meta) {
    if (!meta.is_object()) {
        return;
    }
    for (auto it = meta.begin(); it != meta.end(); ++it) {
        markdown += "> " + it.key() + ": " + JsonToString(it.value()) + "\n";
    }
}

} // namespace

Core::Result<ImportStoryboardResult> ImportStoryboard(const std::string& jsonText,
                                                      StoryboardImportMode mode,
                                                      const Snapshot* existingSnapshot) {
    OrderedJson root;
    try {
        root = OrderedJson::parse(jsonText);
    } catch (const nlohmann::json::exception& ex) {
        return Core::Result<ImportStoryboardResult>::Fail(Core::Error::Make(
            Core::ErrorCode::ParseFailure, ex.what(), "分镜导入 JSON 无法解析"));
    }
    if (!root.is_object() || !root.contains("format") || !root["format"].is_string() ||
        root["format"].get<std::string>() != "DirectorDeskStoryboardImport") {
        return Core::Result<ImportStoryboardResult>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "Unexpected storyboard-import format",
            "不是 DirectorDesk 分镜导入文件"));
    }
    if (!root.contains("formatVersion") || !root["formatVersion"].is_number_integer() ||
        root["formatVersion"].get<int>() != 1) {
        return Core::Result<ImportStoryboardResult>::Fail(Core::Error::Make(
            Core::ErrorCode::Unsupported, "Unsupported storyboard-import version",
            "不支持的分镜导入版本"));
    }
    if (!root.contains("scenes") || !root["scenes"].is_array()) {
        return Core::Result<ImportStoryboardResult>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "storyboard-import missing scenes",
            "分镜导入缺少 scenes"));
    }

    ImportStoryboardResult result;
    std::string title = "未命名";
    if (root.contains("title") && root["title"].is_string() &&
        !root["title"].get<std::string>().empty()) {
        title = root["title"].get<std::string>();
    } else {
        result.diagnostics.emplace_back("标题缺失，已使用「未命名」");
    }
    if (root.contains("source") && root["source"].is_object() && root["source"].contains("tool") &&
        root["source"]["tool"].is_string()) {
        result.diagnostics.push_back("来源 " + root["source"]["tool"].get<std::string>());
    }

    std::unordered_set<std::string> sceneIds;
    std::unordered_set<std::string> shotIds;
    if (mode == StoryboardImportMode::Append && existingSnapshot != nullptr) {
        CollectIds(*existingSnapshot, sceneIds, shotIds);
    }

    if (mode == StoryboardImportMode::Replace) {
        result.markdown += "# " + title + "\n\n";
    }

    for (const OrderedJson& sceneJson : root["scenes"]) {
        if (!sceneJson.is_object()) {
            result.diagnostics.emplace_back("已跳过非法场次");
            continue;
        }
        const std::string requestedSceneId =
            sceneJson.contains("id") && sceneJson["id"].is_string() ? sceneJson["id"].get<std::string>()
                                                                    : std::string();
        const std::string sceneId = ResolveId(requestedSceneId, true, sceneIds, result.diagnostics);
        std::string sceneTitle = "未命名";
        if (sceneJson.contains("title") && sceneJson["title"].is_string() &&
            !sceneJson["title"].get<std::string>().empty()) {
            sceneTitle = sceneJson["title"].get<std::string>();
        } else {
            result.diagnostics.emplace_back("场次标题缺失，已使用「未命名」");
        }
        std::string sceneBody;
        if (sceneJson.contains("body") && sceneJson["body"].is_string()) {
            sceneBody = sceneJson["body"].get<std::string>();
        }
        result.markdown += "## [scene:" + sceneId + "] " + sceneTitle + "\n\n";
        if (!sceneBody.empty()) {
            result.markdown += sceneBody;
            if (sceneBody.back() != '\n') {
                result.markdown += '\n';
            }
            result.markdown += '\n';
        }
        ++result.sceneCount;
        if (!sceneJson.contains("shots") || !sceneJson["shots"].is_array()) {
            continue;
        }
        for (const OrderedJson& shotJson : sceneJson["shots"]) {
            if (!shotJson.is_object()) {
                result.diagnostics.emplace_back("已跳过非法镜头");
                continue;
            }
            const std::string requestedShotId =
                shotJson.contains("id") && shotJson["id"].is_string() ? shotJson["id"].get<std::string>()
                                                                      : std::string();
            const std::string shotId = ResolveId(requestedShotId, false, shotIds, result.diagnostics);
            std::string shotTitle = "未命名";
            if (shotJson.contains("title") && shotJson["title"].is_string() &&
                !shotJson["title"].get<std::string>().empty()) {
                shotTitle = shotJson["title"].get<std::string>();
            } else {
                result.diagnostics.emplace_back("镜头标题缺失，已使用「未命名」");
            }
            std::string shotBody;
            if (shotJson.contains("body") && shotJson["body"].is_string()) {
                shotBody = shotJson["body"].get<std::string>();
            }
            result.markdown += "### [shot:" + shotId + "] " + shotTitle + "\n";
            if (shotJson.contains("meta")) {
                AppendMeta(result.markdown, shotJson["meta"]);
            }
            result.markdown += '\n';
            if (!shotBody.empty()) {
                result.markdown += shotBody;
                if (shotBody.back() != '\n') {
                    result.markdown += '\n';
                }
            }
            result.markdown += '\n';
            ++result.shotCount;
        }
    }
    return Core::Result<ImportStoryboardResult>::Ok(std::move(result));
}

} // namespace DirectorDesk::Script
