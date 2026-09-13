// RevealPath: Implementation for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Platform/RevealPath.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

namespace DirectorDesk::Platform {
namespace detail {
Core::Result<void> RevealPathNative(const std::string& utf8Path, bool folder);
#if !defined(_WIN32) && !defined(__APPLE__)
Core::Result<void> RevealPathNative(const std::string&, bool) {
    return Core::Result<void>::Fail(Core::Error::Make(Core::ErrorCode::Unsupported,
                                                      "RevealPath is not implemented",
                                                      "无法打开路径"));
}
#endif
} // namespace detail

bool RevealPathIsBlocked(const std::string& utf8Path) {
    return utf8Path.empty() || utf8Path.find("://") != std::string::npos ||
           !Paths::IsAbsolute(utf8Path);
}

Core::Result<void> RevealPath(const std::string& utf8Path, bool folder) {
    if (RevealPathIsBlocked(utf8Path)) {
        return Core::Result<void>::Fail(Core::Error::Make(
            Core::ErrorCode::InvalidArgument, "RevealPath rejected path", "无法打开路径"));
    }
    return detail::RevealPathNative(utf8Path, folder);
}

} // namespace DirectorDesk::Platform
