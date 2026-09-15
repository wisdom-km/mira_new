// ImportStoryboard: JSON storyboard import to script-format 1.1 Markdown (FND-30).
#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Script/Types.h"

#include <string>
#include <vector>

namespace DirectorDesk::Script {

enum class StoryboardImportMode {
    Replace,
    Append,
};

struct ImportStoryboardResult {
    std::string markdown;
    std::vector<std::string> diagnostics;
    int sceneCount = 0;
    int shotCount = 0;
};

[[nodiscard]] Core::Result<ImportStoryboardResult> ImportStoryboard(
    const std::string& jsonText, StoryboardImportMode mode, const Snapshot* existingSnapshot);

} // namespace DirectorDesk::Script
