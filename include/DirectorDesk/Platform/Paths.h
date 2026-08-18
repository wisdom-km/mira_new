#pragma once

#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Platform {

class Paths {
public:
    static std::string Join(const std::string& leftUtf8, const std::string& rightUtf8);
    static Core::Result<void> CreateDirectories(const std::string& utf8Path);
    [[nodiscard]] static bool Exists(const std::string& utf8Path);
    [[nodiscard]] static bool IsDirectory(const std::string& utf8Path);

    // Platform user-data root plus "/DirectorDesk".
    static Core::Result<std::string> UserDataDirectory();
    static Core::Result<std::string> LogDirectory();
    static Core::Result<std::string> TemporaryDirectory();

    static std::string FileName(const std::string& utf8Path);
};

} // namespace DirectorDesk::Platform
