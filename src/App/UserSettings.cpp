// UserSettings: Implementation for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/App/UserSettings.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <cmath>
#include <nlohmann/json.hpp>

namespace DirectorDesk::App {
namespace {

bool ReadBool(const nlohmann::json& root, const char* key, bool fallback) {
    if (!root.contains(key) || !root[key].is_boolean()) {
        return fallback;
    }
    return root[key].get<bool>();
}

std::string ReadString(const nlohmann::json& root, const char* key, const std::string& fallback) {
    if (!root.contains(key) || !root[key].is_string()) {
        return fallback;
    }
    return root[key].get<std::string>();
}

std::string ReadProvider(const nlohmann::json& root) {
    const std::string id = ReadString(root, "aiProvider", "openai-compat");
    if (id == "mock") {
        return "mock";
    }
    return "openai-compat";
}

std::string ReadBackground(const nlohmann::json& root) {
    if (!root.contains("viewportBackground") || !root["viewportBackground"].is_string()) {
        return "neutral";
    }
    const std::string id = root["viewportBackground"].get<std::string>();
    if (id == "dark" || id == "neutral") {
        return id;
    }
    return "neutral";
}

std::string ReadExportResolution(const nlohmann::json& root) {
    const std::string id = ReadString(root, "exportResolutionId", "1080p");
    if (id == "2k") {
        return "2k";
    }
    return "1080p";
}

} // namespace

float SanitizeUiScale(float value) {
    if (value <= 0.0f) {
        return 0.0f;
    }
    const float allowed[] = {1.0f, 1.25f, 1.5f, 2.0f};
    float best = 1.0f;
    float bestDist = 1.0e9f;
    for (float allowedValue : allowed) {
        const float dist = std::fabs(allowedValue - value);
        if (dist < bestDist) {
            bestDist = dist;
            best = allowedValue;
        }
    }
    return best;
}

float ResolveUiScale(float stored, unsigned windowWidth, float contentScale) {
    const float sanitized = SanitizeUiScale(stored);
    if (sanitized > 0.0f) {
        return sanitized;
    }
    if (windowWidth >= 2400 && contentScale > 0.95f && contentScale < 1.05f) {
        return 1.25f;
    }
    return 1.0f;
}

UserSettings DefaultUserSettings() {
    return {};
}

Core::Result<UserSettings> LoadUserSettings(const std::string& utf8Path) {
    if (utf8Path.empty()) {
        return Core::Result<UserSettings>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "empty settings path", "设置路径无效"));
    }
    if (!Platform::Paths::Exists(utf8Path)) {
        return Core::Result<UserSettings>::Ok(DefaultUserSettings());
    }
    auto text = Platform::Paths::ReadTextFile(utf8Path);
    if (!text.IsOk()) {
        return Core::Result<UserSettings>::Fail(text.GetError());
    }
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(text.Value());
    } catch (const nlohmann::json::exception& ex) {
        return Core::Result<UserSettings>::Fail(
            Core::Error::Make(Core::ErrorCode::InvalidArgument, ex.what(), "设置文件无法解析"));
    }
    if (!root.is_object()) {
        return Core::Result<UserSettings>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "settings root is not an object", "设置文件无效"));
    }
    UserSettings settings = DefaultUserSettings();
    settings.lockExportAspect = ReadBool(root, "lockExportAspect", settings.lockExportAspect);
    settings.leftFoldExplicit = ReadBool(root, "leftFoldExplicit", settings.leftFoldExplicit);
    settings.leftFolded = ReadBool(root, "leftFolded", settings.leftFolded);
    settings.showGroundGrid = ReadBool(root, "showGroundGrid", settings.showGroundGrid);
    settings.showGroundAxes = ReadBool(root, "showGroundAxes", settings.showGroundAxes);
    settings.showThirds = ReadBool(root, "showThirds", settings.showThirds);
    settings.showSafeFrame = ReadBool(root, "showSafeFrame", settings.showSafeFrame);
    settings.viewportBackground = ReadBackground(root);
    settings.aiProvider = ReadProvider(root);
    settings.aiBaseUrl = ReadString(root, "aiBaseUrl", settings.aiBaseUrl);
    settings.aiApiKey = ReadString(root, "aiApiKey", settings.aiApiKey);
    settings.aiImageModel = ReadString(root, "aiImageModel", settings.aiImageModel);
    settings.aiVideoModel = ReadString(root, "aiVideoModel", settings.aiVideoModel);
    settings.aiChatModel = ReadString(root, "aiChatModel", settings.aiChatModel);
    if (root.contains("uiScale") && root["uiScale"].is_number()) {
        settings.uiScale = SanitizeUiScale(root["uiScale"].get<float>());
    }
    settings.openLastProject = ReadBool(root, "openLastProject", settings.openLastProject);
    settings.lastProjectPath = ReadString(root, "lastProjectPath", settings.lastProjectPath);
    settings.defaultExportDirectory =
        ReadString(root, "defaultExportDirectory", settings.defaultExportDirectory);
    settings.exportResolutionId = ReadExportResolution(root);
    settings.exportTransparent = ReadBool(root, "exportTransparent", settings.exportTransparent);
    settings.defaultSkillId = ReadString(root, "defaultSkillId", settings.defaultSkillId);
    return Core::Result<UserSettings>::Ok(settings);
}

Core::Result<void> SaveUserSettings(const std::string& utf8Path, const UserSettings& settings) {
    if (utf8Path.empty() || !Platform::Paths::IsAbsolute(utf8Path)) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "settings path must be absolute", "设置路径无效"));
    }
    const std::string parent = Platform::Paths::Parent(utf8Path);
    auto created = Platform::Paths::CreateDirectories(parent);
    if (!created.IsOk()) {
        return created;
    }
    nlohmann::json root;
    root["version"] = 1;
    root["lockExportAspect"] = settings.lockExportAspect;
    root["leftFoldExplicit"] = settings.leftFoldExplicit;
    root["leftFolded"] = settings.leftFolded;
    root["showGroundGrid"] = settings.showGroundGrid;
    root["showGroundAxes"] = settings.showGroundAxes;
    root["showThirds"] = settings.showThirds;
    root["showSafeFrame"] = settings.showSafeFrame;
    root["viewportBackground"] =
        settings.viewportBackground == "dark" ? "dark" : "neutral";
    root["aiProvider"] = settings.aiProvider == "mock" ? "mock" : "openai-compat";
    root["aiBaseUrl"] = settings.aiBaseUrl;
    root["aiApiKey"] = settings.aiApiKey;
    root["aiImageModel"] = settings.aiImageModel;
    root["aiVideoModel"] = settings.aiVideoModel;
    root["aiChatModel"] = settings.aiChatModel;
    root["uiScale"] = SanitizeUiScale(settings.uiScale > 0.0f ? settings.uiScale : 1.0f);
    root["openLastProject"] = settings.openLastProject;
    root["lastProjectPath"] = settings.lastProjectPath;
    root["defaultExportDirectory"] = settings.defaultExportDirectory;
    root["exportResolutionId"] =
        settings.exportResolutionId == "2k" ? "2k" : "1080p";
    root["exportTransparent"] = settings.exportTransparent;
    root["defaultSkillId"] = settings.defaultSkillId;
    const std::string tempPath = utf8Path + ".tmp";
    auto written = Platform::Paths::WriteTextFile(tempPath, root.dump(2));
    if (!written.IsOk()) {
        return written;
    }
    return Platform::Paths::AtomicReplace(tempPath, utf8Path);
}

std::uint32_t ViewportClearRgba(const std::string& backgroundId) {
    if (backgroundId == "dark") {
        return 0x1a1a1dff;
    }
    return 0x2b2b2eff;
}

} // namespace DirectorDesk::App
