#pragma once

#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Platform {

class FileDialog {
public:
    // Empty path means the user cancelled. Failure is a dialog/system error.
    static Core::Result<std::string> OpenModelFile();
};

} // namespace DirectorDesk::Platform
