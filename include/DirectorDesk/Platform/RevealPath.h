// RevealPath: Public or internal interface for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"

#include <string>

namespace DirectorDesk::Platform {

// Empty paths, non-absolute paths, and any path containing "://" are rejected.
[[nodiscard]] bool RevealPathIsBlocked(const std::string& utf8Path);

// folder=false opens the file with the system default handler.
// folder=true opens the parent folder and selects the file when possible.
Core::Result<void> RevealPath(const std::string& utf8Path, bool folder);

} // namespace DirectorDesk::Platform
