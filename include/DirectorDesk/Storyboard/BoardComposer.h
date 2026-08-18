#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Storyboard/Types.h"

#include <cstdint>
#include <string>
#include <unordered_map>

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

Core::Result<BoardComposeResult> ComposeBoard(const BoardComposeRequest& request);

} // namespace DirectorDesk::Storyboard
