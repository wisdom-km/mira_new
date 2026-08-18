# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 9 已完成，准备进入 Phase 10
- **当前状态**：AI 接口骨架已合入；Wisdom 授权提交。下一步是体验打磨与发布准备
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`phase-9-ai-interfaces`（本提交）
- **下一个允许执行的工作**：Phase 10：体验打磨、发布准备与文档。不得接入真实 AI 服务商

## 已完成

- [x] Phase 0–8：骨架到官方在线资产库
- [x] Phase 9：AI 接口骨架；tag `phase-9-ai-interfaces`

## 进行中

无。下一步是 Phase 10。

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
| AI 接口 | 供应商无关；参考图只接受本地路径或 RGBA；P0 无密钥 UI、无真实调用 |

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

- Windows MSVC 19.44 + Ninja Debug 构建成功
- `DirectorDeskTests` 全部通过：103 cases / 554 assertions
- 未开始 Phase 10

## 下一步清单

1. 统一空状态、错误提示和快捷键
2. 完善对外文档、许可证与示例工程
3. Windows 回归；macOS 依赖 CI
4. 形成 P0 发布检查清单

## 工作日志

### 2026-08-19：Phase 9 AI 接口骨架

- 定义图像/视频生成请求、进度、取消和结果。
- Null 明确拒绝；Mock 用 `Pump` 验证成功、失败、取消。
- 参考图只接受本地路径；镜头 ID/标题/文本随请求传递。
- 未增加 UI、供应商 SDK 或网络调用。

### 2026-08-19：Phase 8 官方资产库

- 官方仓库 `wisdom-km/obj-3d-models`；资源库「本地 / 在线」页签。
- 已提交 `fe9ca08` 并推送 tag `phase-8-online-assets`。

### 2026-08-19：Phase 7 分镜画布与正式导出

- 确定性 LTR 布局、防抖缩略图、1080p/2K 与分镜总览 PNG。
- 已提交 `a76c9b8` 并推送 tag `phase-7-core-loop`。

### 2026-08-19：Phase 6–0

- 工程文件、本地资源库、预设机位、剧本、模型导入与开发地图。
