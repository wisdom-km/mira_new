#pragma once

#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Renderer/Types.h"

#include <string>

namespace DirectorDesk::Renderer {

Core::Result<void> WritePng(const PixelBuffer& pixels, const std::string& utf8Path);

} // namespace DirectorDesk::Renderer
