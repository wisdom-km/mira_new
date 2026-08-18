// ShotExport: Public or internal interface for the DirectorDesk Export module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Renderer/Types.h"

#include <cstdint>
#include <string>

namespace DirectorDesk::Export {

enum class ShotResolution {
    Hd1080,
    Uhd2k,
};

struct ShotExportOptions {
    ShotResolution resolution = ShotResolution::Hd1080;
    bool transparentBackground = true;
    std::string outputPath;
};

struct ShotSize {
    std::uint32_t width = 1920;
    std::uint32_t height = 1080;
};

[[nodiscard]] ShotSize SizeFor(ShotResolution resolution);
[[nodiscard]] const char* ResolutionId(ShotResolution resolution);
[[nodiscard]] bool TryParseResolution(const std::string& id, ShotResolution& out);
[[nodiscard]] std::string DefaultShotFileName(const std::string& projectName,
                                              const std::string& shotId,
                                              ShotResolution resolution, bool transparent);
[[nodiscard]] Renderer::RenderTargetDesc MakeOffscreenTarget(ShotResolution resolution,
                                                             bool transparent);

} // namespace DirectorDesk::Export
