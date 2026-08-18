#include "DirectorDesk/Platform/FileDialog.h"

#include "DirectorDesk/Core/Error.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ShObjIdl.h>
#include <Windows.h>

#include <string>

namespace DirectorDesk::Platform {
namespace {

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

Core::Result<std::string> PathFromDialog(IFileDialog* dialog) {
    IShellItem* item = nullptr;
    HRESULT hr = dialog->GetResult(&item);
    if (FAILED(hr) || item == nullptr) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "IFileDialog::GetResult failed", "无法读取所选文件"));
    }

    PWSTR widePath = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &widePath);
    item->Release();
    if (FAILED(hr) || widePath == nullptr) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "GetDisplayName failed", "无法读取所选文件"));
    }

    std::string utf8 = WideToUtf8(widePath);
    CoTaskMemFree(widePath);
    return Core::Result<std::string>::Ok(std::move(utf8));
}

template<typename Dialog>
Core::Result<std::string> ShowDialog(Dialog* dialog, const wchar_t* title,
                                     const COMDLG_FILTERSPEC* filters, UINT filterCount,
                                     const wchar_t* defaultExtension) {
    if (dialog == nullptr) {
        return Core::Result<std::string>::Fail(
            Core::Error::Make(Core::ErrorCode::Internal, "CoCreateInstance IFileDialog failed",
                              "无法打开文件对话框"));
    }
    dialog->SetFileTypes(filterCount, filters);
    dialog->SetTitle(title);
    if (defaultExtension != nullptr) {
        dialog->SetDefaultExtension(defaultExtension);
    }

    const HRESULT hr = dialog->Show(nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        return Core::Result<std::string>::Ok(std::string{});
    }
    if (FAILED(hr)) {
        dialog->Release();
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "IFileDialog::Show failed", "无法打开文件对话框"));
    }

    auto path = PathFromDialog(dialog);
    dialog->Release();
    return path;
}

Core::Result<std::string> ShowOpenDialog(const wchar_t* title, const COMDLG_FILTERSPEC* filters,
                                         UINT filterCount) {
    IFileOpenDialog* dialog = nullptr;
    CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    return ShowDialog(dialog, title, filters, filterCount, nullptr);
}

Core::Result<std::string> ShowSaveDialog(const wchar_t* title, const COMDLG_FILTERSPEC* filters,
                                         UINT filterCount, const wchar_t* defaultExtension) {
    IFileSaveDialog* dialog = nullptr;
    CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    return ShowDialog(dialog, title, filters, filterCount, defaultExtension);
}

} // namespace

Core::Result<std::string> FileDialog::OpenModelFile() {
    const COMDLG_FILTERSPEC filters[] = {
        {L"3D Models (*.glb; *.obj)", L"*.glb;*.obj"},
        {L"All files", L"*.*"},
    };
    return ShowOpenDialog(L"Import Model", filters, 2);
}

Core::Result<std::string> FileDialog::OpenMarkdownFile() {
    const COMDLG_FILTERSPEC filters[] = {
        {L"Markdown (*.md)", L"*.md"},
        {L"All files", L"*.*"},
    };
    return ShowOpenDialog(L"Open Script", filters, 2);
}

Core::Result<std::string> FileDialog::SaveMarkdownFile() {
    const COMDLG_FILTERSPEC filters[] = {
        {L"Markdown (*.md)", L"*.md"},
        {L"All files", L"*.*"},
    };
    return ShowSaveDialog(L"Save Script", filters, 2, L"md");
}

} // namespace DirectorDesk::Platform
