// RevealPathWin32: Implementation for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Platform/Paths.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ShlObj.h>
#include <Shellapi.h>
#include <Windows.h>

#include <string>

namespace DirectorDesk::Platform {
namespace detail {
namespace {

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 1) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), size);
    return wide;
}

Core::Result<void> FailOpen() {
    return Core::Result<void>::Fail(Core::Error::Make(
        Core::ErrorCode::IoFailure, "RevealPath native open failed", "无法打开路径"));
}

} // namespace

Core::Result<void> RevealPathNative(const std::string& utf8Path, bool folder) {
    const std::wstring wide = Utf8ToWide(utf8Path);
    if (wide.empty()) {
        return FailOpen();
    }
    if (folder) {
        PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(wide.c_str());
        if (pidl == nullptr) {
            const std::string parent = Paths::Parent(utf8Path);
            if (parent.empty() || !Paths::Exists(parent)) {
                return FailOpen();
            }
            const std::wstring parentWide = Utf8ToWide(parent);
            pidl = ILCreateFromPathW(parentWide.c_str());
            if (pidl == nullptr) {
                return FailOpen();
            }
        }
        const HRESULT hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ILFree(pidl);
        if (FAILED(hr)) {
            return FailOpen();
        }
        return Core::Result<void>::Ok();
    }
    const INT_PTR launched = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (launched <= 32) {
        return FailOpen();
    }
    return Core::Result<void>::Ok();
}

} // namespace detail
} // namespace DirectorDesk::Platform
