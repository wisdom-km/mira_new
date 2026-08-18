#pragma once

#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Platform {

class FileDialog {
public:
    // Empty path means the user cancelled. Failure is a dialog/system error.
    static Core::Result<std::string> OpenModelFile();
    static Core::Result<std::string> OpenMarkdownFile();
    static Core::Result<std::string> SaveMarkdownFile();
    static Core::Result<std::string> OpenProjectFile();
    static Core::Result<std::string> SaveProjectFile();
    static Core::Result<std::string> SavePngFile(const std::string& defaultName);
};

} // namespace DirectorDesk::Platform
