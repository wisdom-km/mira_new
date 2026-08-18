// Ids: Public or internal interface for the DirectorDesk Script module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <string>

namespace DirectorDesk::Script {

[[nodiscard]] bool IsValidId(const std::string& id);
[[nodiscard]] std::string GenerateSceneId();
[[nodiscard]] std::string GenerateShotId();

} // namespace DirectorDesk::Script
