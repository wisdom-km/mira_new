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

} // namespace

Core::Result<std::string> FileDialog::OpenModelFile() {
    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr) {
        return Core::Result<std::string>::Fail(
            Core::Error::Make(Core::ErrorCode::Internal, "CoCreateInstance IFileOpenDialog failed",
                              "无法打开文件对话框"));
    }

    COMDLG_FILTERSPEC filters[] = {
        {L"3D Models (*.glb; *.obj)", L"*.glb;*.obj"},
        {L"All files", L"*.*"},
    };
    dialog->SetFileTypes(2, filters);
    dialog->SetTitle(L"Import Model");

    hr = dialog->Show(nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        return Core::Result<std::string>::Ok(std::string{});
    }
    if (FAILED(hr)) {
        dialog->Release();
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "IFileOpenDialog::Show failed", "无法打开文件对话框"));
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    dialog->Release();
    if (FAILED(hr) || item == nullptr) {
        return Core::Result<std::string>::Fail(Core::Error::Make(
            Core::ErrorCode::Internal, "IFileOpenDialog::GetResult failed", "无法读取所选文件"));
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

} // namespace DirectorDesk::Platform
