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
};

UserSettings DefaultUserSettings();
Core::Result<UserSettings> LoadUserSettings(const std::string& utf8Path);
Core::Result<void> SaveUserSettings(const std::string& utf8Path, const UserSettings& settings);
std::uint32_t ViewportClearRgba(const std::string& backgroundId);

} // namespace DirectorDesk::App
