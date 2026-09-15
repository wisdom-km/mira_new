// BoardComposer: Public or internal interface for the DirectorDesk Storyboard module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Storyboard/Types.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace DirectorDesk::Storyboard {

struct BoardComposeRequest {
    LayoutResult layout;
    std::unordered_map<std::string, ImageBuffer> thumbnails;
    std::string fontPath;
    std::uint32_t maxEdge = 8192;
};

struct BoardComposeResult {
    ImageBuffer pixels;
    bool scaledToMax = false;
};

struct BoardPdfResult {
    std::vector<ImageBuffer> pages;
};

[[nodiscard]] Core::Result<BoardComposeResult> ComposeBoard(const BoardComposeRequest& request);
[[nodiscard]] Core::Result<BoardPdfResult> ComposePdfPages(const BoardComposeRequest& request,
                                                           int cellsPerPage = 6);

} // namespace DirectorDesk::Storyboard
