# DirectorDesk

3D 导演台：用剧本、资源库和预设机位，帮助非 3D 专业用户制作可控的 AI 视频分镜。

P0 已走通：**Markdown 剧本 → 本地/官方资源 → 预设机位 → 镜头关联 → 分镜画布 → PNG 导出**。权威需求与架构见 [`docs/dev-map/`](docs/dev-map/)。

Windows 用户可直接从 [Releases](https://github.com/wisdom-km/mira_new/releases) 下载 `DirectorDesk-0.1.0-windows-x64.exe` 安装包。

## 要求

- Windows 10/11 或 macOS
- CMake ≥ 3.24
- C++17 编译器：MSVC 或 Apple Clang
- [vcpkg](https://github.com/microsoft/vcpkg)
- Windows 本地 Ninja 构建还需 Ninja；也可用 Visual Studio 生成器

## 构建

完整步骤见 [docs/BUILD.md](docs/BUILD.md)。Windows 最快路径：

```powershell
git clone https://github.com/wisdom-km/mira_new.git
cd mira_new
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug --output-on-failure
.\build\windows-debug\DirectorDesk.exe --project .\examples\cafe.ddproj
```

macOS：

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug --output-on-failure
./build/macos-debug/DirectorDesk --project ./examples/cafe.ddproj
```

首次配置会由 vcpkg 编译依赖，需要联网。

## 使用

见 [docs/USER-GUIDE.md](docs/USER-GUIDE.md)。核心路径：

1. 打开 `examples/cafe.ddproj` 或自己的 Markdown 剧本
2. 从资源库导入本地模型，或在「在线」页下载官方 CC0 立方体
3. 用预设机位摆镜头，并关联到剧本 Shot
4. 在分镜画布总览，导出 1080p/2K 单镜头或分镜总览 PNG

## 文档

| 文档 | 内容 |
|------|------|
| [docs/BUILD.md](docs/BUILD.md) | Windows / macOS 构建 |
| [docs/USER-GUIDE.md](docs/USER-GUIDE.md) | 用户路径与快捷键 |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 贡献约定 |
| [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md) | 第三方与资产许可证 |
| [docs/RELEASE-CHECKLIST.md](docs/RELEASE-CHECKLIST.md) | P0 发布检查清单 |
| [docs/dev-map/](docs/dev-map/) | 愿景、架构、路线图 |

shader 由 CMake 调用 shaderc 编译，不要提交生成的二进制。

许可证：MIT。
