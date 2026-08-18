# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 2（Windows 本地验收已通过）
- **当前状态**：可导入 GLB/OBJ，后台解析，视口显示并可用数值编辑 Transform；macOS 待 CI 验证
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`phase-1-render-camera`（`460911a`）
- **下一个允许执行的工作**：用户明确要求后再打 `phase-2-model-import`。打 tag 前不得开始 Phase 3

## 已完成

- [x] Phase 0 骨架、日志、GLFW、ImGui docking、CI
- [x] Phase 1 bgfx、轨道相机、离屏透明 PNG
- [x] `IModelLoader`、`LoaderRegistry`、`ModelData`
- [x] GLB 加载（cgltf）：网格、索引、节点变换、基础色与嵌入纹理
- [x] OBJ 加载（tinyobjloader）：网格、法线、UV、MTL 基础材质
- [x] 缺少法线时生成；损坏/不支持文件返回错误且不崩溃
- [x] Scene Node 与 Transform（位置/旋转/缩放）
- [x] 原生文件对话框、导入 Command、后台 Worker + 主线程结果队列
- [x] UI 选择节点与数值 Transform 编辑

## 进行中

无。Phase 2 功能在 Windows 上已验收。

## 阻塞项

无。macOS 实机未在本机验证，依赖 CI。文件对话框在 macOS 使用 `NSOpenPanel`，本机未跑。

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
| 完整性校验 | picosha2（SHA-256） |
| 测试 | Catch2 v3 |
| 开发顺序 | 先走通含分镜画布的本地资产核心闭环，再实现在线资产库 |
| 项目持久化 | 版本化 JSON `.ddproj` |
| 分镜画布 | 剧本是结构唯一来源；自动布局、不可自由连线；导演台变化后按需刷新缩略图 |
| 分镜导出 | 支持单镜头参考图和完整分镜总览 PNG |
| vcpkg baseline | `c5a15727ee70fddf0296f0d8aafc3f58916fefac` |
| Phase 2 变换入口 | 数值 DragFloat3；不引入 ImGuizmo（未锁定进技术栈） |
| 外部 .gltf + 分离 bin/uri | P0 只保证 `.glb`；外部 URI 纹理降级为纯色并警告 |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc，禁止手工产物 | Phase 1 已在 Windows 验证 |
| 透明离屏渲染/回读可能因后端差异失败 | 在正式 Export 前完成技术切片 | Phase 1 Windows 已通过；macOS 待 CI |
| Windows 中文路径与编码 | Platform 统一 UTF-8 边界并加入测试 | Phase 0/2 已测 |
| GLB 特性范围失控 | P0 限基础静态网格/材质，忽略骨骼和动画 | Phase 2 |
| GitHub 在部分网络环境不可用 | 缓存最后有效清单；镜像源仅列后续 | Phase 8 |
| 大型剧本的布局和缩略图刷新造成卡顿 | 确定性布局、可见区裁剪、防抖、单帧单任务和缓存上限 | Phase 7 |
| 多模型协作导致架构漂移 | 强制读地图、更新状态、执行 Phase 门禁 | 全程 |

## 本次验证

- Windows MSVC 19.44 + Ninja Debug 配置与构建成功
- `DirectorDeskTests`：21 cases / 76 assertions 通过（含中文路径 GLB/OBJ、损坏文件、不支持扩展名、Transform）
- `--import examples/models/cube.glb`：日志 `Imported model ... as cube`，视口显示带基础色的立方体，Workspace 可选中并编辑 Position/Rotation/Scale

## 下一步清单

1. 用户明确要求后再打 `phase-2-model-import`
2. 之后才能开始 Phase 3：Markdown 剧本与镜头列表

## 工作日志

### 2026-08-19：Phase 2 模型导入

- 将 Scene/Asset 从接口库落地为实现库。
- 实现 LoaderRegistry、GLB/OBJ 加载器、后台 Worker、Windows/macOS 文件对话框。
- 导入结果只经主线程结果队列写入 Scene，再由 Renderer 上传 GPU。
- 示例模型：`examples/models/cube.obj`、`cube.glb`。
- 未实现剧本、预设机位、工程文件或在线资产库。

### 2026-08-19：Phase 1 渲染、相机与离屏 PNG

- 引入 bgfx、glm、stb；ImGui 去掉 OpenGL3/glad，改为 `InitForOther` + bgfx 提交。
- GLFW 窗口改为 `GLFW_NO_API`，用原生 HWND/NSWindow 初始化 bgfx。
- 实现 `IRenderer` / `BgfxRenderer`、测试立方体、shaderc、轨道相机、视口纹理。
- 实现离屏透明 PNG 回读；UI 只发 `ExportTestPngCommand`。
- 默认停靠：Workspace 左栏、Viewport 中央。未实现模型加载、剧本或分镜画布。

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
