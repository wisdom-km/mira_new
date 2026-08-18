// Log: Public or internal interface for the DirectorDesk Core module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"

#include <fmt/format.h>

#include <string>
#include <utility>

namespace DirectorDesk::Core {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
};

class Log {
public:
    // Creates <utf8LogDirectory> if needed and writes rotating daily files plus console output.
    static Result<void> Init(const std::string& utf8LogDirectory);
    static void Shutdown();
    [[nodiscard]] static bool IsInitialized();

    static void WriteFormatted(
        LogLevel level,
        const char* sourceFile,
        int line,
        const std::string& message);

    template<typename... Args>
    static void Write(
        LogLevel level,
        const char* sourceFile,
        int line,
        const char* format,
        Args&&... args) {
        WriteFormatted(
            level,
            sourceFile,
            line,
            fmt::format(fmt::runtime(format), std::forward<Args>(args)...));
    }
};

} // namespace DirectorDesk::Core

#define DD_LOG_TRACE(...)                                                                          \
    ::DirectorDesk::Core::Log::Write(                                                              \
        ::DirectorDesk::Core::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define DD_LOG_DEBUG(...)                                                                          \
    ::DirectorDesk::Core::Log::Write(                                                              \
        ::DirectorDesk::Core::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define DD_LOG_INFO(...)                                                                           \
    ::DirectorDesk::Core::Log::Write(                                                              \
        ::DirectorDesk::Core::LogLevel::Info, __FILE__, __LINE__, __VA_ARGS__)
#define DD_LOG_WARN(...)                                                                           \
    ::DirectorDesk::Core::Log::Write(                                                              \
        ::DirectorDesk::Core::LogLevel::Warn, __FILE__, __LINE__, __VA_ARGS__)
#define DD_LOG_ERROR(...)                                                                          \
    ::DirectorDesk::Core::Log::Write(                                                              \
        ::DirectorDesk::Core::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
