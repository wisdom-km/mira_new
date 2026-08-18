#pragma once

#include "DirectorDesk/Renderer/IRenderer.h"

#include <memory>

namespace DirectorDesk::Backends {

std::unique_ptr<Renderer::IRenderer> CreateBgfxRenderer();

} // namespace DirectorDesk::Backends
