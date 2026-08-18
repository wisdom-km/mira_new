#include "DirectorDesk/Platform/Paths.h"

#include "DirectorDesk/Core/Error.h"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ShlObj.h>
#include <Windows.h>
#endif

namespace DirectorDesk::Platform {
namespace {

std::filesystem::path ToPath(const std::string& utf8Path) {
    return std::filesystem::u8path(utf8Path);
}

std::string FromPath(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

Core::Error IoError(const std::string& technical, const std::string& user) {
    return Core::Error::Make(Core::ErrorCode::IoFailure, technical, user);
}

#ifdef _WIN32
std::string WideToUtf8(const wchar_t* wide) {
    if (wide == nullptr || wide[0] == L'\0') {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
}
#endif

} // namespace

std::string Paths::Join(const std::string& leftUtf8, const std::string& rightUtf8) {
    return FromPath(ToPath(leftUtf8) / ToPath(rightUtf8));
}

Core::Result<void> Paths::CreateDirectories(const std::string& utf8Path) {
    if (utf8Path.empty()) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument,
            "CreateDirectories received an empty path",
            "路径不能为空"));
    }

    std::error_code ec;
    std::filesystem::create_directories(ToPath(utf8Path), ec);
    if (ec) {
        return Core::Result<void>::Fail(
            IoError("create_directories failed: " + ec.message(), "无法创建目录"));
    }
    return Core::Result<void>::Ok();
}

bool Paths::Exists(const std::string& utf8Path) {
    std::error_code ec;
    const bool exists = std::filesystem::exists(ToPath(utf8Path), ec);
    return !ec && exists;
}

bool Paths::IsDirectory(const std::string& utf8Path) {
    std::error_code ec;
    const bool isDir = std::filesystem::is_directory(ToPath(utf8Path), ec);
    return !ec && isDir;
}

Core::Result<std::string> Paths::UserDataDirectory() {
#ifdef _WIN32
    PWSTR widePath = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &widePath);
    if (FAILED(hr) || widePath == nullptr) {
        if (widePath != nullptr) {
            CoTaskMemFree(widePath);
        }
        return Core::Result<std::string>::Fail(
            IoError("SHGetKnownFolderPath failed", "无法定位用户数据目录"));
    }
    const std::string appData = WideToUtf8(widePath);
    CoTaskMemFree(widePath);
    return Core::Result<std::string>::Ok(Join(appData, "DirectorDesk"));
#else
    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0') {
        return Core::Result<std::string>::Fail(
            IoError("HOME is not set", "无法定位用户数据目录"));
    }
    return Core::Result<std::string>::Ok(
        Join(Join(Join(home, "Library"), "Application Support"), "DirectorDesk"));
#endif
}

Core::Result<std::string> Paths::LogDirectory() {
    auto userData = UserDataDirectory();
    if (!userData.IsOk()) {
        return userData;
    }
    return Core::Result<std::string>::Ok(Join(userData.Value(), "logs"));
}

Core::Result<std::string> Paths::TemporaryDirectory() {
    std::error_code ec;
    const auto temp = std::filesystem::temp_directory_path(ec);
    if (ec) {
        return Core::Result<std::string>::Fail(
            IoError("temp_directory_path failed: " + ec.message(), "无法定位临时目录"));
    }
    return Core::Result<std::string>::Ok(FromPath(temp));
}

std::string Paths::FileName(const std::string& utf8Path) {
    return FromPath(ToPath(utf8Path).filename());
}

} // namespace DirectorDesk::Platform
