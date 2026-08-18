#include "DirectorDesk/Core/Log.h"

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

namespace DirectorDesk::Core {
namespace {

constexpr const char* kLoggerName = "DirectorDesk";
std::mutex g_logMutex;
bool g_initialized = false;
std::shared_ptr<std::ofstream> g_logFile;

spdlog::level::level_enum ToSpdlog(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warn:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
    }
    return spdlog::level::info;
}

std::string LocalDateString() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d");
    return output.str();
}

} // namespace

Result<void> Log::Init(const std::string& utf8LogDirectory) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_initialized) {
        return Result<void>::Fail(Error::Make(
            ErrorCode::AlreadyInitialized,
            "Log::Init called more than once",
            "日志系统已经初始化"));
    }

    std::error_code ec;
    const std::filesystem::path logDir = std::filesystem::u8path(utf8LogDirectory);
    std::filesystem::create_directories(logDir, ec);
    if (ec) {
        return Result<void>::Fail(Error::Make(
            ErrorCode::IoFailure,
            "Failed to create log directory: " + ec.message(),
            "无法创建日志目录"));
    }

    try {
        const std::filesystem::path filePath =
            logDir / std::filesystem::u8path("directordesk-" + LocalDateString() + ".log");
        g_logFile = std::make_shared<std::ofstream>(filePath, std::ios::out | std::ios::app);
        if (!g_logFile->is_open()) {
            g_logFile.reset();
            return Result<void>::Fail(Error::Make(
                ErrorCode::IoFailure,
                "Failed to open log file",
                "无法打开日志文件"));
        }

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::ostream_sink_mt>(*g_logFile));

        auto logger = std::make_shared<spdlog::logger>(kLoggerName, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        logger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(logger);
        g_initialized = true;
        logger->info("Log initialized. directory={}", utf8LogDirectory);
        logger->flush();
        return Result<void>::Ok();
    } catch (const spdlog::spdlog_ex& ex) {
        g_logFile.reset();
        return Result<void>::Fail(Error::Make(
            ErrorCode::IoFailure,
            std::string("spdlog init failed: ") + ex.what(),
            "日志系统初始化失败"));
    }
}

void Log::Shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_initialized) {
        return;
    }
    spdlog::shutdown();
    if (g_logFile) {
        g_logFile->flush();
        g_logFile->close();
        g_logFile.reset();
    }
    g_initialized = false;
}

bool Log::IsInitialized() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return g_initialized;
}

void Log::WriteFormatted(
    LogLevel level,
    const char* sourceFile,
    int line,
    const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_initialized) {
        return;
    }
    spdlog::source_loc location{sourceFile, line, ""};
    spdlog::log(location, ToSpdlog(level), message);
}

} // namespace DirectorDesk::Core
