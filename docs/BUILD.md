# 构建指南

DirectorDesk 使用 CMake ≥ 3.24 与 vcpkg manifest。`vcpkg.json` 锁定 baseline `c5a15727ee70fddf0296f0d8aafc3f58916fefac`。

## 准备

1. 安装 Git、CMake ≥ 3.24、C++17 编译器（Windows：Visual Studio 2022；macOS：Xcode / Apple Clang）
2. 安装 [vcpkg](https://github.com/microsoft/vcpkg)，并设置环境变量 `VCPKG_ROOT`
3. Windows 若用 Ninja preset，还需安装 Ninja

首次配置会编译 bgfx、ImGui、libcurl 等依赖，需要联网。

## Windows（优先）

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug --output-on-failure
.\build\windows-debug\DirectorDesk.exe --project .\examples\cafe.ddproj
```

没有 Ninja 时：

```powershell
cmake -B build/vs -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/vs --config Debug
ctest --test-dir build/vs -C Debug --output-on-failure
```

仓库内 `build\rebuild-local.bat` 适用于已配置过 `build/windows-debug` 的本机开发。

## macOS

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug --output-on-failure
./build/macos-debug/DirectorDesk --project ./examples/cafe.ddproj
```

## 命令行参数

| 参数 | 作用 |
|------|------|
| `--project <path>` | 打开 `.ddproj` |
| `--script <path>` | 打开 Markdown 剧本 |
| `--import <path>` | 导入 OBJ/GLB |
| `--export-test-png` | 开发用离屏 PNG 自检后退出 |

## Windows 安装包

在已配置 VS + vcpkg 的机器上：

```powershell
winget install --id JRSoftware.InnoSetup -e
powershell -ExecutionPolicy Bypass -File packaging\windows\build-installer.ps1
```

产物在 `dist/`：

- `DirectorDesk-0.1.0-windows-x64.exe`：Inno Setup 安装包
- `DirectorDesk-0.1.0-windows-x64.zip`：便携目录

不要把 `dist/` 或 `packaging/stage/` 提交进仓库。

## 常见问题

- **找不到 vcpkg**：确认 `VCPKG_ROOT` 指向 vcpkg 根目录，且该目录含 `scripts/buildsystems/vcpkg.cmake`
- **shader 报错**：不要手工提交 `build/**/shaders`；CMake 会调用 shaderc
- **中文路径**：内部一律 UTF-8；请用本机中文用户目录做一次导入/保存/导出回归
- **官方在线清单失败**：部分网络无法访问 `raw.githubusercontent.com`，应用会使用最后有效缓存
