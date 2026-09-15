// UserSettings: UI preferences in the user-data directory (UIC-34). Not part of .ddproj.
#pragma once

#include "DirectorDesk/Core/Result.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::App {

struct UserSettings {
    bool lockExportAspect = true;
    bool leftFoldExplicit = false;
    bool leftFolded = false;
    bool showGroundGrid = true;
    bool showGroundAxes = true;
    bool showThirds = false;
    bool showSafeFrame = false;
    std::string viewportBackground = "neutral";
    std::string aiProvider = "openai-compat";
    std::string aiBaseUrl = "https://api.openai.com";
    std::string aiApiKey;
    std::string aiImageModel = "gpt-image-1";
    std::string aiVideoModel = "sora-2";
    std::string aiChatModel = "gpt-4o-mini";
    float uiScale = 0.0f;
    bool openLastProject = false;
    std::string lastProjectPath;
    std::string defaultExportDirectory;
    std::string exportResolutionId = "1080p";
    bool exportTransparent = true;
    std::string defaultSkillId;
};

UserSettings DefaultUserSettings();
float SanitizeUiScale(float value);
float ResolveUiScale(float stored, unsigned windowWidth, float contentScale);
Core::Result<UserSettings> LoadUserSettings(const std::string& utf8Path);
Core::Result<void> SaveUserSettings(const std::string& utf8Path, const UserSettings& settings);
std::uint32_t ViewportClearRgba(const std::string& backgroundId);

} // namespace DirectorDesk::App
