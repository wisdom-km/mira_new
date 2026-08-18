# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 8 已完成，准备进入 Phase 9
- **当前状态**：官方在线资产库已接通 `wisdom-km/obj-3d-models`；Wisdom 已在 Windows 上确认 Online 链路
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`phase-8-online-assets`（本提交）
- **下一个允许执行的工作**：Phase 9：AI 接口骨架。不得接入具体服务商、密钥 UI 或真实网络调用

## 已完成

- [x] Phase 0–6：骨架到工程文件/镜头关联
- [x] Phase 7：分镜画布与正式导出；tag `phase-7-core-loop`（`a76c9b8`）
- [x] Phase 8：官方在线资产库；tag `phase-8-online-assets`

## 进行中

无。下一步是 Phase 9。

## 阻塞项

无。macOS 实机未在本机验证，依赖 CI。

## 已确认决策

| 决策 | 结论 |
|------|------|
| P0 平台 | Windows + macOS；先开发、验证 Windows |
| Linux | P0 不支持，后续演进 |
| 代码许可证 | MIT |
| 官方资产许可证 | 优先 CC0，允许明确标注的 CC-BY |
| C++ 标准 | C++17 |
| 构建与依赖 | CMake + vcpkg manifest |
| 渲染/UI/窗口 | bgfx / Dear ImGui docking / GLFW |
| ImGui 后端 | GLFW + bgfx |
| 窗口图形 API | `GLFW_NO_API`，由 bgfx 创建交换链 |
| 模型加载 | cgltf（GLB）+ tinyobjloader（OBJ），通过 `IModelLoader` 扩展 |
| 网络与 JSON | libcurl + nlohmann-json |
| 完整性校验 | picosha2（SHA-256）；工程文件仍用既有内置哈希 |
| 测试 | Catch2 v3 |
| 开发顺序 | 先走通含分镜画布的本地资产核心闭环，再实现在线资产库 |
| 项目持久化 | 版本化 JSON `.ddproj` |
| 分镜画布 | 剧本是 Scene/Shot 结构的唯一来源；自动布局、不可自由连线 |
| 分镜导出 | 支持单镜头参考图和完整分镜总览 PNG |
| vcpkg baseline | `c5a15727ee70fddf0296f0d8aafc3f58916fefac` |
| Phase 2 变换入口 | 数值 DragFloat3；不引入 ImGuizmo |
| 外部 .gltf + 分离 bin/uri | P0 只保证 `.glb` |
| 地面格网 | 视口绘制、导出不含；批准者 Wisdom |
| 模型线框叠加 | P0 不做 |
| 预设机位朝向约定 | 物体默认朝 +Z；无选中对象时目标为 `(0, 0.5, 0)`、半径 `1` |
| 本地资源预览 | sidecar PNG 或纯色占位 |
| 镜头 3D 缩略图 | Phase 7 由 App 主线程离屏渲染 |
| 官方在线源 | 编译期固定唯一地址；无自定义源、无上传 |
| 官方地址 | 仓库 https://github.com/wisdom-km/obj-3d-models ；清单 `https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/manifest.json` ；资源基地址 `https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/` ；批准者 Wisdom |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc | Phase 1 Windows 已验证 |
| 透明离屏渲染/回读 | Phase 1 技术切片 | Phase 1/7 Windows 已通过；macOS 待 CI |
| Windows 中文路径 | Platform UTF-8 边界 | Phase 0–7 已测 |
| 大型剧本卡顿 | 防抖、可见区、缓存上限 | Phase 7 |
| raw.githubusercontent.com 在部分网络环境不可用 | 清单刷新失败时使用最后有效缓存 | Phase 8 |
| 源文件移动后 ID 变化 | 稳定路径键 | Phase 5 |

## 本次验证

- Wisdom 在 Windows 上确认资源库「在线」页可浏览官方清单
- Windows MSVC 19.44 + Ninja Debug 构建成功
- `DirectorDeskTests` 全部通过：95 cases / 512 assertions
- 未开始 Phase 9

## 下一步清单

1. 定义 `IImageGenService` / `IVideoGenService` 及请求、结果、进度、取消、错误类型
2. 用 Null/Mock 验证异步结果和错误流
3. 不扩大 P0 UI，不接入供应商 SDK

## 工作日志

### 2026-08-19：Phase 8 官方资产库

- 增加 `IHttpClient`、curl 后端、picosha2。
- 实现清单解析、相对路径/主机校验、原子缓存、SHA-256 与大小校验。
- 资源库用「本地 / 在线」页签展示官方资产；无第三方源入口。
- 官方仓库：https://github.com/wisdom-km/obj-3d-models
- 已发布 `manifest.json` 与 CC0 起步立方体 `basic-cube@1.0.0`。

### 2026-08-19：Phase 7 分镜画布与正式导出

- 确定性 LTR 布局、防抖缩略图、1080p/2K 与分镜总览 PNG。
- 已提交 `a76c9b8` 并推送 tag `phase-7-core-loop`。

### 2026-08-19：Phase 6 工程文件与镜头关联

- `.ddproj` 版本 1、镜头关联、脏工程提示。已打 `phase-6-project-link`（`929725d`）。

### 2026-08-19：Phase 5 本地资源库

- `Asset::Library` 与 sidecar/占位预览。已打 `phase-5-local-assets`（`f78f190`）。

### 2026-08-19：Phase 4–0

- 预设机位、剧本、模型导入、渲染骨架与开发地图。
