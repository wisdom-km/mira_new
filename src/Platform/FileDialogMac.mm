#include "DirectorDesk/Platform/FileDialog.h"

#include "DirectorDesk/Core/Error.h"

#import <AppKit/AppKit.h>

#include <string>

namespace DirectorDesk::Platform {

Core::Result<std::string> FileDialog::OpenModelFile() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[ @"glb", @"obj" ];
        panel.title = @"Import Model";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        NSURL* url = panel.URL;
        if (url == nil || url.path == nil) {
            return Core::Result<std::string>::Fail(Core::Error::Make(
                Core::ErrorCode::Internal, "NSOpenPanel returned an empty path", "无法读取所选文件"));
        }
        return Core::Result<std::string>::Ok(std::string([url.path UTF8String]));
    }
}

} // namespace DirectorDesk::Platform
