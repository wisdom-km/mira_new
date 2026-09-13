// RevealPathMac: Platform implementation for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Core/Result.h"
#include "DirectorDesk/Platform/Paths.h"

#import <AppKit/AppKit.h>

#include <string>

namespace DirectorDesk::Platform {
namespace detail {

Core::Result<void> RevealPathNative(const std::string& utf8Path, bool folder) {
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:utf8Path.c_str()];
        if (path == nil || path.length == 0) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::IoFailure, "RevealPath native open failed", "无法打开路径"));
        }
        NSURL* url = [NSURL fileURLWithPath:path];
        if (url == nil) {
            return Core::Result<void>::Fail(Core::Error::Make(
                Core::ErrorCode::IoFailure, "RevealPath native open failed", "无法打开路径"));
        }
        if (folder) {
            [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[ url ]];
            return Core::Result<void>::Ok();
        }
        const BOOL opened = [[NSWorkspace sharedWorkspace] openURL:url];
        if (!opened) {
            const std::string parent = Paths::Parent(utf8Path);
            if (parent.empty()) {
                return Core::Result<void>::Fail(Core::Error::Make(
                    Core::ErrorCode::IoFailure, "RevealPath native open failed", "无法打开路径"));
            }
            NSString* parentPath = [NSString stringWithUTF8String:parent.c_str()];
            NSURL* parentUrl = parentPath == nil ? nil : [NSURL fileURLWithPath:parentPath];
            if (parentUrl == nil || ![[NSWorkspace sharedWorkspace] openURL:parentUrl]) {
                return Core::Result<void>::Fail(Core::Error::Make(
                    Core::ErrorCode::IoFailure, "RevealPath native open failed", "无法打开路径"));
            }
        }
        return Core::Result<void>::Ok();
    }
}

} // namespace detail
} // namespace DirectorDesk::Platform
