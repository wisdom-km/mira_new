# DirectorDesk

3D 导演台：用剧本、资源库和预设机位，帮助非 3D 专业用户制作可控的 AI 视频分镜。

当前进度：**Phase 1（bgfx 最小渲染 + 轨道相机）进行中**。权威需求与架构见 [`docs/dev-map/`](docs/dev-map/)。

## 要求

- Windows 10/11 或 macOS
- CMake ≥ 3.24
- C++17 编译器：MSVC 或 Apple Clang
- [vcpkg](https://github.com/microsoft/vcpkg)
- Windows 本地 Ninja 构建还需 Ninja；也可用 Visual Studio 生成器

## 构建（Windows 优先）

```powershell
git clone https://github.com/wisdom-km/mira_new.git
cd mira_new
$env:VCPKG_ROOT = "C:\path\to\vcpkg"   # 改成你的 vcpkg 路径
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug --output-on-failure
.\build\windows-debug\DirectorDesk.exe
# 可选：导出一张透明测试 PNG 后退出
.\build\windows-debug\DirectorDesk.exe --export-test-png
```

没有 Ninja 时：

```powershell
cmake -B build/vs -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/vs --config Debug
ctest --test-dir build/vs -C Debug --output-on-failure
```

## 构建（macOS）

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug --output-on-failure
./build/macos-debug/DirectorDesk
```

首次配置会由 vcpkg 编译 spdlog、GLFW、Dear ImGui（docking）和 Catch2，需要联网。

## Phase 1 包含什么

- bgfx 渲染测试立方体、基础灯光
- 轨道相机（视口内拖拽/滚轮）
- ImGui docking 与 3D 视口组合
- 离屏透明 PNG 技术验证（File / 按钮 / `--export-test-png`）

shader 由 CMake 调用 shaderc 编译，不要提交生成的二进制。

## 开发约定

请先阅读：

1. [`docs/dev-map/00-VISION-AND-CONSTRAINTS.md`](docs/dev-map/00-VISION-AND-CONSTRAINTS.md)
2. [`docs/dev-map/01-ARCHITECTURE-MAP.md`](docs/dev-map/01-ARCHITECTURE-MAP.md)
3. [`docs/dev-map/02-ROADMAP.md`](docs/dev-map/02-ROADMAP.md)
4. [`docs/dev-map/03-CURRENT-STATUS.md`](docs/dev-map/03-CURRENT-STATUS.md)

许可证：MIT。
