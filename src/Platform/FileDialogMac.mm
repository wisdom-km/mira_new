// FileDialogMac: Platform implementation for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#include "DirectorDesk/Platform/FileDialog.h"

#include "DirectorDesk/Core/Error.h"

#import <AppKit/AppKit.h>

#include <string>

namespace DirectorDesk::Platform {
namespace {

Core::Result<std::string> PathFromUrl(NSURL* url) {
    if (url == nil || url.path == nil) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "File panel returned an empty path", "无法读取所选文件"));
    }
    return Core::Result<std::string>::Ok(std::string([url.path UTF8String]));
}

} // namespace

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
        return PathFromUrl(panel.URL);
    }
}

Core::Result<std::string> FileDialog::OpenMarkdownFile() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[ @"md" ];
        panel.title = @"Open Script";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        return PathFromUrl(panel.URL);
    }
}

Core::Result<std::string> FileDialog::SaveMarkdownFile() {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.allowedFileTypes = @[ @"md" ];
        panel.title = @"Save Script";
        panel.nameFieldStringValue = @"script.md";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        return PathFromUrl(panel.URL);
    }
}

Core::Result<std::string> FileDialog::OpenProjectFile() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[ @"ddproj" ];
        panel.title = @"Open Project";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        return PathFromUrl(panel.URL);
    }
}

Core::Result<std::string> FileDialog::SavePngFile(const std::string& defaultName) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.allowedFileTypes = @[ @"png" ];
        panel.title = @"Export PNG";
        if (!defaultName.empty()) {
            panel.nameFieldStringValue = [NSString stringWithUTF8String:defaultName.c_str()];
        } else {
            panel.nameFieldStringValue = @"export.png";
        }
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        return PathFromUrl(panel.URL);
    }
}

Core::Result<std::string> FileDialog::SaveProjectFile() {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.allowedFileTypes = @[ @"ddproj" ];
        panel.title = @"Save Project";
        panel.nameFieldStringValue = @"project.ddproj";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return Core::Result<std::string>::Ok(std::string{});
        }
        return PathFromUrl(panel.URL);
    }
}

} // namespace DirectorDesk::Platform
