#include "DirectorDesk/Platform/Paths.h"

#include "DirectorDesk/Core/Error.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ShlObj.h>
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <vector>
#endif

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
        return Core::Result<void>::Fail(
            Core::Error::Make(Core::ErrorCode::InvalidArgument,
                              "CreateDirectories received an empty path", "路径不能为空"));
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
        return Core::Result<std::string>::Fail(IoError("HOME is not set", "无法定位用户数据目录"));
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

Core::Result<std::string> Paths::ExecutableDirectory() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return Core::Result<std::string>::Fail(
            IoError("GetModuleFileNameW failed", "无法定位可执行文件目录"));
    }
    return Core::Result<std::string>::Ok(FromPath(ToPath(WideToUtf8(buffer)).parent_path()));
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1u);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return Core::Result<std::string>::Fail(
            IoError("_NSGetExecutablePath failed", "无法定位可执行文件目录"));
    }
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(ToPath(buffer.data()), ec);
    if (ec) {
        return Core::Result<std::string>::Fail(
            IoError("canonical executable path failed: " + ec.message(), "无法定位可执行文件目录"));
    }
    return Core::Result<std::string>::Ok(FromPath(canonical.parent_path()));
#else
    return Core::Result<std::string>::Fail(IoError(
        "ExecutableDirectory is not implemented on this platform", "无法定位可执行文件目录"));
#endif
}

Core::Result<std::vector<std::uint8_t>> Paths::ReadBinaryFile(const std::string& utf8Path) {
    std::ifstream input(ToPath(utf8Path), std::ios::binary);
    if (!input) {
        return Core::Result<std::vector<std::uint8_t>>::Fail(
            Core::Error::Make(Core::ErrorCode::NotFound, "Failed to open file", "无法打开文件"));
    }
    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0) {
        return Core::Result<std::vector<std::uint8_t>>::Fail(
            IoError("tellg failed", "无法读取文件"));
    }
    input.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    if (end > 0) {
        input.read(reinterpret_cast<char*>(bytes.data()), end);
        if (!input) {
            return Core::Result<std::vector<std::uint8_t>>::Fail(
                IoError("read failed", "无法读取文件"));
        }
    }
    return Core::Result<std::vector<std::uint8_t>>::Ok(std::move(bytes));
}

Core::Result<void> Paths::WriteBinaryFile(const std::string& utf8Path, const std::uint8_t* data,
                                          std::size_t size) {
    if (data == nullptr && size > 0) {
        return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::InvalidArgument,
                                                          "WriteBinaryFile received a null buffer",
                                                          "写入数据无效"));
    }

    const auto parent = ToPath(utf8Path).parent_path();
    if (!parent.empty()) {
        auto created = CreateDirectories(FromPath(parent));
        if (!created.IsOk()) {
            return created;
        }
    }

    const auto tempPath = ToPath(utf8Path + ".tmp");
    {
        std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            return Core::Result<void>::Fail(IoError("Failed to open temp file", "无法写入文件"));
        }
        if (size > 0) {
            output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        }
        output.flush();
        if (!output) {
            return Core::Result<void>::Fail(IoError("Failed to write temp file", "无法写入文件"));
        }
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, ToPath(utf8Path), ec);
    if (ec) {
        std::filesystem::remove(ToPath(utf8Path), ec);
        std::filesystem::rename(tempPath, ToPath(utf8Path), ec);
        if (ec) {
            return Core::Result<void>::Fail(
                IoError("Failed to replace file: " + ec.message(), "无法写入文件"));
        }
    }
    return Core::Result<void>::Ok();
}

} // namespace DirectorDesk::Platform
