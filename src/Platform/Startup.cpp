// Startup: Implementation for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Platform/Startup.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Objbase.h>
#include <Windows.h>
#endif

namespace DirectorDesk::Platform {

void InitializeProcess() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
}

void ShutdownProcess() {
#ifdef _WIN32
    CoUninitialize();
#endif
}

} // namespace DirectorDesk::Platform
