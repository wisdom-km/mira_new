#pragma once

#include "DirectorDesk/Platform/IHttpClient.h"

#include <memory>

namespace DirectorDesk::Backends {

std::unique_ptr<DirectorDesk::Platform::IHttpClient> CreateCurlHttpClient();

} // namespace DirectorDesk::Backends
