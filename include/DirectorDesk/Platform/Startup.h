// Startup: Public or internal interface for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

namespace DirectorDesk::Platform {

// UTF-8 console output and any other process-wide platform bootstrap.
void InitializeProcess();
void ShutdownProcess();

} // namespace DirectorDesk::Platform
