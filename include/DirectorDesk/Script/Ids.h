#pragma once

#include <string>

namespace DirectorDesk::Script {

[[nodiscard]] bool IsValidId(const std::string& id);
[[nodiscard]] std::string GenerateSceneId();
[[nodiscard]] std::string GenerateShotId();

} // namespace DirectorDesk::Script
