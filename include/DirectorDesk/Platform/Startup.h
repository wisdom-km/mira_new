#pragma once

namespace DirectorDesk::Platform {

// UTF-8 console output and any other process-wide platform bootstrap.
void InitializeProcess();
void ShutdownProcess();

} // namespace DirectorDesk::Platform
