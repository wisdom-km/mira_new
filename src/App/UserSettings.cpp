// UserSettings: Implementation for the DirectorDesk App module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/App/UserSettings.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <nlohmann/json.hpp>

namespace DirectorDesk::App {
namespace {

bool ReadBool(const nlohmann::json& root, const char* key, bool fallback) {
    if (!root.contains(key) || !root[key].is_boolean()) {
        return fallback;
    }
    return root[key].get<bool>();
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

} // namespace

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
