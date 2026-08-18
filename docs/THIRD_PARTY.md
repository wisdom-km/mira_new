# 第三方依赖与资产许可证

DirectorDesk 本体为 MIT，见仓库根目录 `LICENSE`。

## 构建依赖（vcpkg）

| 组件 | 用途 | 许可证 |
|------|------|--------|
| spdlog / fmt | 日志 | MIT |
| GLFW | 窗口与输入 | zlib/libpng |
| glm | 数学 | MIT / Happy Bunny |
| Dear ImGui（docking） | 界面 | MIT |
| bgfx / bx / bimg | 渲染与 shaderc | BSD-2-Clause |
| stb | 图像编解码 | 公共域 / MIT |
| cgltf | GLB | MIT |
| tinyobjloader | OBJ | MIT |
| nlohmann-json | JSON | MIT |
| libcurl | HTTPS | curl |
| zlib | curl 依赖 | zlib |
| picosha2 | SHA-256 | MIT |
| Catch2 | 测试 | BSL-1.0 |

具体版本以 `vcpkg.json` 的 `builtin-baseline` 为准。

## 示例与官方资产

| 资产 | 位置 | 许可证 |
|------|------|--------|
| 示例立方体 OBJ/GLB | `examples/models/` | 仓库随 DirectorDesk MIT 提供，无第三方网格 |
| 示例剧本 | `examples/scripts/cafe.md` | MIT |
| 示例工程 | `examples/cafe.ddproj` | MIT |
| 官方在线立方体 `basic-cube` | https://github.com/wisdom-km/obj-3d-models | CC0-1.0 |

官方在线清单只接受 `CC0-1.0` 或带署名的 `CC-BY-4.0`。用户自行导入的模型版权由用户负责。

## 运行时缓存

用户数据目录（Windows：`%APPDATA%\DirectorDesk`）中的日志、导出和官方下载缓存不属于本仓库，分发构建物时不要打包他人缓存。
