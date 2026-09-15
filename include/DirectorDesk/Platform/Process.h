// Process: Run a local subprocess and wait for it. UTF-8 paths; no Win32 types in this header.

#pragma once

#include "DirectorDesk/Core/Result.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace DirectorDesk::Platform {

struct ProcessRequest {
    std::vector<std::string> argv;
    std::string workingDirectory;
    std::uint32_t timeoutMs = 60000;
    const std::atomic<bool>* cancel = nullptr;
};

struct ProcessResult {
    int exitCode = 0;
};

Core::Result<ProcessResult> RunProcess(const ProcessRequest& request);

} // namespace DirectorDesk::Platform
