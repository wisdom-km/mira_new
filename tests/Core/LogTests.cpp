#include "DirectorDesk/Core/Log.h"
#include "DirectorDesk/Platform/Paths.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

namespace {

std::string UniqueChineseLogDir() {
    auto temp = DirectorDesk::Platform::Paths::TemporaryDirectory();
    REQUIRE(temp.IsOk());
    const auto base = DirectorDesk::Platform::Paths::Join(temp.Value(), "导演台日志_DirectorDesk");
    return DirectorDesk::Platform::Paths::Join(base, "case-log");
}

} // namespace

TEST_CASE("Log writes a rolling file under a Chinese path", "[core][log]") {
    if (DirectorDesk::Core::Log::IsInitialized()) {
        DirectorDesk::Core::Log::Shutdown();
    }

    const std::string logDir = UniqueChineseLogDir();
    auto created = DirectorDesk::Platform::Paths::CreateDirectories(logDir);
    REQUIRE(created.IsOk());

    auto init = DirectorDesk::Core::Log::Init(logDir);
    REQUIRE(init.IsOk());
    REQUIRE(DirectorDesk::Core::Log::IsInitialized());

    DD_LOG_INFO("phase-0 log test {}", "ok");
    DirectorDesk::Core::Log::Shutdown();
    REQUIRE_FALSE(DirectorDesk::Core::Log::IsInitialized());

    bool foundLog = false;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(logDir), ec)) {
        if (ec) {
            break;
        }
        const auto name = entry.path().filename().u8string();
        const std::string fileName(name.begin(), name.end());
        if (fileName.find("directordesk") != std::string::npos) {
            foundLog = true;
            break;
        }
    }
    REQUIRE(foundLog);
}
