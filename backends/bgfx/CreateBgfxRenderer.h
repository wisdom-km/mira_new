// CreateBgfxRenderer: Public or internal interface for the DirectorDesk bgfx module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Renderer/IRenderer.h"

#include <memory>

namespace DirectorDesk::Backends {

std::unique_ptr<Renderer::IRenderer> CreateBgfxRenderer();

} // namespace DirectorDesk::Backends
