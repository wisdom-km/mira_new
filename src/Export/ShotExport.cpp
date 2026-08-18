// ShotExport: Implementation for the DirectorDesk Export module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Export/ShotExport.h"

namespace DirectorDesk::Export {

ShotSize SizeFor(ShotResolution resolution) {
    if (resolution == ShotResolution::Uhd2k) {
        return ShotSize{2560, 1440};
    }
    return ShotSize{1920, 1080};
}

const char* ResolutionId(ShotResolution resolution) {
    return resolution == ShotResolution::Uhd2k ? "2k" : "1080p";
}

bool TryParseResolution(const std::string& id, ShotResolution& out) {
    if (id == "2k" || id == "2560x1440") {
        out = ShotResolution::Uhd2k;
        return true;
    }
    if (id == "1080p" || id == "1920x1080") {
        out = ShotResolution::Hd1080;
        return true;
    }
    return false;
}

std::string DefaultShotFileName(const std::string& projectName, const std::string& shotId,
                                ShotResolution resolution, bool transparent) {
    const ShotSize size = SizeFor(resolution);
    std::string stem = projectName.empty() ? "DirectorDesk" : projectName;
    for (char& ch : stem) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' ||
            ch == '<' || ch == '>' || ch == '|') {
            ch = '_';
        }
    }
    return stem + "-" + (shotId.empty() ? "shot" : shotId) + "-" +
           std::to_string(size.width) + "x" + std::to_string(size.height) +
           (transparent ? "-alpha.png" : ".png");
}

Renderer::RenderTargetDesc MakeOffscreenTarget(ShotResolution resolution, bool transparent) {
    const ShotSize size = SizeFor(resolution);
    Renderer::RenderTargetDesc target;
    target.kind = Renderer::RenderTargetKind::Offscreen;
    target.width = size.width;
    target.height = size.height;
    target.transparentBackground = transparent;
    return target;
}

} // namespace DirectorDesk::Export
