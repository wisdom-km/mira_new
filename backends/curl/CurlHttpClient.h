// CurlHttpClient: Public or internal interface for the DirectorDesk curl module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Platform/IHttpClient.h"

#include <memory>

namespace DirectorDesk::Backends {

std::unique_ptr<DirectorDesk::Platform::IHttpClient> CreateCurlHttpClient();

} // namespace DirectorDesk::Backends
