# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 0（实现完成，等待用户授权提交/tag）
- **当前状态**：Windows 本地构建、测试和应用启动已通过；macOS 待 CI 验证
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：无
- **下一个允许执行的工作**：用户确认后提交 Phase 0；打 `phase-0-skeleton` tag 前不得开始 Phase 1

## 已完成

- [x] 创建 `docs/dev-map/` 核心开发地图
- [x] Phase 0 强制目录、CMake、CMake Presets、vcpkg manifest
- [x] `Core::Log`、`Result`/`Error`、最小 Command 队列
- [x] Platform UTF-8 路径、用户数据目录、GLFW 窗口封装
- [x] Dear ImGui docking 空工作区（临时 OpenGL3 后端）
- [x] Catch2 单元测试（含中文路径）
- [x] Windows / macOS GitHub Actions CI
- [x] MIT LICENSE、README、CONTRIBUTING、clang-format/tidy

## 进行中

无。

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
| Phase 0 ImGui 后端 | 临时 GLFW + OpenGL3；Phase 1 替换为 bgfx |
| 模型加载 | cgltf（GLB）+ tinyobjloader（OBJ），通过 `IModelLoader` 扩展 |
| 网络与 JSON | libcurl + nlohmann-json |
| 完整性校验 | picosha2（SHA-256） |
| 测试 | Catch2 v3 |
| 开发顺序 | 先走通含分镜画布的本地资产核心闭环，再实现在线资产库 |
| 项目持久化 | 版本化 JSON `.ddproj` |
| 分镜画布 | 剧本是结构唯一来源；自动布局、不可自由连线；导演台变化后按需刷新缩略图 |
| 分镜导出 | 支持单镜头参考图和完整分镜总览 PNG |
| vcpkg baseline | `c5a15727ee70fddf0296f0d8aafc3f58916fefac` |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc，禁止手工产物 | Phase 1 |
| 透明离屏渲染/回读可能因后端差异失败 | 在正式 Export 前完成技术切片 | Phase 1 |
| Windows 中文路径与编码 | Platform 统一 UTF-8 边界并加入测试 | Phase 0 已测 / Phase 2/3 |
| GLB 特性范围失控 | P0 限基础静态网格/材质，不做骨骼动画 | Phase 2 |
| GitHub 在部分网络环境不可用 | 缓存最后有效清单；镜像源仅列后续 | Phase 8 |
| 大型剧本的布局和缩略图刷新造成卡顿 | 确定性布局、可见区裁剪、防抖、单帧单任务和缓存上限 | Phase 7 |
| 多模型协作导致架构漂移 | 强制读地图、更新状态、执行 Phase 门禁 | 全程 |

## 本次验证

- Windows MSVC 19.44 + Ninja Debug 配置与构建成功
- `ctest`：`DirectorDeskTests` 通过（7 cases / 23 assertions）
- 启动 `DirectorDesk.exe`：创建 1280×720 窗口，ImGui docking 后端初始化，日志写入 `%APPDATA%\DirectorDesk\logs`

## 下一步清单

1. 用户确认后提交 Phase 0 代码（如需同步远程，再 push）
2. 用户明确要求后再打 `phase-0-skeleton`
3. 之后才能开始 Phase 1：bgfx 最小渲染、轨道相机、离屏 PNG 回读

## 工作日志

### 2026-08-19：Phase 0 骨架实现

- 建立 include/src/backends/tests/CMake/vcpkg/CI 骨架。
- 实现日志、UTF-8 路径、GLFW 窗口和 ImGui docking 空工作区。
- 未实现场景、模型、剧本、bgfx 或在线资产库。
- Windows 本地构建/测试/启动通过。

### 2026-08-19：加入剧本驱动的分镜画布

- 将分镜画布加入核心用户路径和 P0 范围。
- 明确剧本是 Scene/Shot 结构的唯一来源，画布自动布局且不支持任意连线。
- 明确导演台变化后的缩略图失效、主线程防抖刷新和选择同步规则。
- 增加完整分镜总览 PNG 导出，并更新 Phase 3、6、7 验收项。

### 2026-08-19：开发地图初始化

- 创建愿景红线、架构地图、路线图、状态、交接规则、编码规范、持续开发 Prompt 和模块契约。
- 将原三平台目标调整为 P0 Windows + macOS，Windows 优先。
