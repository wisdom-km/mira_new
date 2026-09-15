// Process: Implementation for the DirectorDesk Platform module.

#include "DirectorDesk/Platform/Process.h"

#include "DirectorDesk/Core/Error.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#endif

namespace DirectorDesk::Platform {
namespace {

Core::Error InvalidArgv() {
    return Core::Error::Make(Core::ErrorCode::InvalidArgument, "empty argv", "运行命令无效");
}

#ifdef _WIN32
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

std::wstring QuoteArg(const std::string& utf8) {
    std::wstring wide = Utf8ToWide(utf8);
    bool needQuotes = false;
    for (wchar_t ch : wide) {
        if (ch == L' ' || ch == L'\t' || ch == L'"') {
            needQuotes = true;
            break;
        }
    }
    if (!needQuotes) {
        return wide;
    }
    std::wstring quoted = L"\"";
    for (wchar_t ch : wide) {
        if (ch == L'"') {
            quoted += L"\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += L'"';
    return quoted;
}
#endif

} // namespace

Core::Result<ProcessResult> RunProcess(const ProcessRequest& request) {
    if (request.argv.empty() || request.argv[0].empty()) {
        return Core::Result<ProcessResult>::Fail(InvalidArgv());
    }
#ifdef _WIN32
    std::wstring commandLine;
    for (std::size_t i = 0; i < request.argv.size(); ++i) {
        if (i > 0) {
            commandLine += L' ';
        }
        commandLine += QuoteArg(request.argv[i]);
    }
    std::wstring cwd;
    if (!request.workingDirectory.empty()) {
        cwd = Utf8ToWide(request.workingDirectory);
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    std::vector<wchar_t> mutableLine(commandLine.begin(), commandLine.end());
    mutableLine.push_back(L'\0');
    const BOOL ok = CreateProcessW(nullptr, mutableLine.data(), nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, cwd.empty() ? nullptr : cwd.c_str(),
                                   &startup, &info);
    if (!ok) {
        return Core::Result<ProcessResult>::Fail(
            Core::Error::Make(Core::ErrorCode::IoFailure, "CreateProcessW failed", "无法启动进程"));
    }
    const DWORD timeout = request.timeoutMs == 0 ? INFINITE : request.timeoutMs;
    DWORD wait = WaitForSingleObject(info.hProcess, timeout);
    if (request.cancel != nullptr && request.cancel->load()) {
        TerminateProcess(info.hProcess, 1);
        wait = WAIT_OBJECT_0;
    }
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(info.hProcess, 1);
        CloseHandle(info.hThread);
        CloseHandle(info.hProcess);
        return Core::Result<ProcessResult>::Fail(
            Core::Error::Make(Core::ErrorCode::IoFailure, "process timeout", "Skill 运行超时"));
    }
    DWORD code = 1;
    GetExitCodeProcess(info.hProcess, &code);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    ProcessResult result;
    result.exitCode = static_cast<int>(code);
    if (result.exitCode != 0) {
        return Core::Result<ProcessResult>::Fail(Core::Error::Make(
            Core::ErrorCode::IoFailure, "process exit " + std::to_string(result.exitCode),
            "Skill 进程失败"));
    }
    return Core::Result<ProcessResult>::Ok(result);
#else
    std::vector<char*> args;
    args.reserve(request.argv.size() + 1);
    std::vector<std::string> storage = request.argv;
    for (std::string& item : storage) {
        args.push_back(item.data());
    }
    args.push_back(nullptr);
    const pid_t pid = fork();
    if (pid < 0) {
        return Core::Result<ProcessResult>::Fail(
            Core::Error::Make(Core::ErrorCode::IoFailure, "fork failed", "无法启动进程"));
    }
    if (pid == 0) {
        if (!request.workingDirectory.empty()) {
            if (chdir(request.workingDirectory.c_str()) != 0) {
                _exit(127);
            }
        }
        execvp(args[0], args.data());
        _exit(127);
    }
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(request.timeoutMs == 0 ? 60000 : request.timeoutMs);
    int status = 0;
    while (true) {
        if (request.cancel != nullptr && request.cancel->load()) {
            kill(pid, SIGTERM);
        }
        const pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            break;
        }
        if (std::chrono::steady_clock::now() > deadline) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return Core::Result<ProcessResult>::Fail(
                Core::Error::Make(Core::ErrorCode::IoFailure, "process timeout", "Skill 运行超时"));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ProcessResult result;
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    if (result.exitCode != 0) {
        return Core::Result<ProcessResult>::Fail(Core::Error::Make(
            Core::ErrorCode::IoFailure, "process exit " + std::to_string(result.exitCode),
            "Skill 进程失败"));
    }
    return Core::Result<ProcessResult>::Ok(result);
#endif
}

} // namespace DirectorDesk::Platform
