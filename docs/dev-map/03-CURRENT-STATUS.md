# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 6（Windows 本地验收已通过，正在提交并合入 main）
- **当前状态**：`.ddproj` 原子保存/打开、镜头↔相机关联、新建/打开/保存/另存为；脏工程退出会提示
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`cursor/phase-4-camera-presets` → `main`
- **最近完成 tag**：`phase-5-local-assets`（`f78f190`）；本提交后打 `phase-6-project-link`
- **下一个允许执行的工作**：合入并推送 `main`、打 tag 后开始 Phase 7

## 已完成

- [x] Phase 0 骨架、日志、GLFW、ImGui docking、CI
- [x] Phase 1 bgfx、轨道相机、离屏透明 PNG
- [x] Phase 2 GLB/OBJ 导入、Scene Node、后台加载、数值 Transform
- [x] Phase 3 Markdown 剧本、镜头列表、CJK 字体
- [x] Phase 4：CameraManager、五种构图预设、三种灯光预设、视口地面格网（`55ed3a7`，未单独打 tag）
- [x] Phase 5：本地资源库；tag `phase-5-local-assets`（`f78f190`）已推送到 `origin/cursor/phase-4-camera-presets`
- [x] `.ddproj` 版本 1 读写、原子保存、未知字段容忍、过高版本拒绝
- [x] 工程相对路径优先；移动工程目录后相对资产仍可解析
- [x] 镜头关联到相机；选择镜头时跳转已关联相机
- [x] 分镜 Scene 折叠状态写入工程文件（画布 UI 仍属 Phase 7）
- [x] New / Open / Save / Save As Command；脏工程提示保存/放弃/取消
- [x] 损坏或校验失败的工程不覆盖当前内存状态
- [x] 缺失资产显示占位盒并记录诊断，不阻止打开

## 进行中

无。Phase 6 功能在 Windows 上已验收，等待用户授权提交。

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
| 网络与 JSON | libcurl + nlohmann-json（Phase 5/6 已实际使用 nlohmann-json） |
| 完整性校验 | picosha2（SHA-256）；Phase 6 工程内资产哈希用内置 SHA-256，picosha2 仍留给 Phase 8 |
| 测试 | Catch2 v3 |
| 开发顺序 | 先走通含分镜画布的本地资产核心闭环，再实现在线资产库 |
| 项目持久化 | 版本化 JSON `.ddproj` |
| 分镜画布 | 剧本是 Scene/Shot 结构的唯一来源；自动布局、不可自由连线；导演台变化后按需刷新缩略图 |
| 分镜导出 | 支持单镜头参考图和完整分镜总览 PNG |
| vcpkg baseline | `c5a15727ee70fddf0296f0d8aafc3f58916fefac` |
| Phase 2 变换入口 | 数值 DragFloat3；不引入 ImGuizmo（未锁定进技术栈） |
| 外部 .gltf + 分离 bin/uri | P0 只保证 `.glb`；外部 URI 纹理降级为纯色并警告 |
| 地面格网 | 写入 Phase 4；视口绘制、导出不含；批准者 Wisdom |
| 模型线框叠加 | P0 不做 |
| 预设机位朝向约定 | 物体默认朝 +Z；无选中对象时目标为 `(0, 0.5, 0)`、半径 `1` |
| 本地资源预览 | sidecar PNG 或纯色占位；Asset 不依赖 Renderer，不做离屏 3D 缩略图（属 Phase 7） |
| 在线资产 | Phase 5 仅占位来源 `online`；不下载、不请求网络（属 Phase 8） |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc，禁止手工产物 | Phase 1 已在 Windows 验证 |
| 透明离屏渲染/回读可能因后端差异失败 | 在正式 Export 前完成技术切片 | Phase 1 Windows 已通过；macOS 待 CI |
| Windows 中文路径与编码 | Platform 统一 UTF-8 边界并加入测试 | Phase 0/2/3/5/6 已测 |
| GLB 特性范围失控 | P0 限基础静态网格/材质，忽略骨骼和动画 | Phase 2 |
| GitHub 在部分网络环境不可用 | 缓存最后有效清单；镜像源仅列后续 | Phase 8 |
| 大型剧本的布局和缩略图刷新造成卡顿 | 确定性布局、可见区裁剪、防抖、单帧单任务和缓存上限 | Phase 7 |
| 多模型协作导致架构漂移 | 强制读地图、更新状态、执行 Phase 门禁 | 全程 |
| 源文件移动后 ID 随路径变化 | ID 绑定稳定路径键；缺失只标状态，不删索引 | Phase 5 |
| 另存为后工程外资产变为绝对路径 | UI 状态提示不可移植；优先 project-relative / user-library | Phase 6 |

## 本次验证

- Windows MSVC 19.44 + Ninja Debug 配置与构建成功
- `DirectorDeskTests` 全部通过：69 cases / 388 assertions（含工程 round-trip、中文路径、目录移动、缺失资产、损坏/高版本拒绝、成环拒绝、中断保存、镜头关联）
- 未实现分镜画布、正式 1080p/2K 导出、在线下载

## 下一步清单

1. 用户明确要求后提交 Phase 6 代码
2. 用户明确要求后再打 `phase-6-project-link` 或 push
3. 之后才能开始 Phase 7：分镜画布与正式导出

## 工作日志

### 2026-08-19：Phase 6 工程文件与镜头关联

- 实现 `.ddproj` 版本 1：原子保存、路径规则、未知字段忽略、过高版本拒绝。
- `Link::Table` 维护镜头↔相机；选择镜头跳转已关联相机。
- File 菜单：新建/打开/保存/另存为；脏工程提示保存、放弃或取消。
- 缺失资产用占位盒；折叠状态写入工程，画布仍留 Phase 7。

### 2026-08-19：Phase 5 本地资源库

- `Asset::Library` 持久化 `index.json`；稳定 ID、来源、sidecar/占位预览。
- Library 面板只发 Command；视口可拖入资产。
- 已提交 `f78f190` 并推送 tag `phase-5-local-assets`。

### 2026-08-19：Phase 4 预设机位与地面格网

- 实现 CameraManager、五种构图预设和三种灯光预设。
- 视口绘制 XZ 格网与红/蓝原点轴；离屏导出不画格网。
- 已提交 `55ed3a7`（分支 `cursor/phase-4-camera-presets`）。

### 2026-08-19：Phase 3 Markdown 剧本与镜头列表

- 将 Script 从接口库落地为实现库：Parser、Document、稳定 ID。
- UI 只发加载、编辑、保存、插入场次/镜头、选中镜头 Command。
- 系统 CJK 字体 + ImGui 1.92 动态图集，解决 Script 面板中文乱码。
- 示例剧本：`examples/scripts/cafe.md`。已打 `phase-3-script`（`a5d51b1`）。

### 2026-08-19：地面格网写入 Phase 4

- 产品负责人批准：视口增加 XZ 地面格网，便于看清地面和摆机位。
- 地图改动已提交为 `c5c0e25` 并随 `phase-2-model-import` 推送。

### 2026-08-19：Phase 2 模型导入

- 将 Scene/Asset 从接口库落地为实现库。
- 实现 LoaderRegistry、GLB/OBJ 加载器、后台 Worker、Windows/macOS 文件对话框。

### 2026-08-19：Phase 1 渲染、相机与离屏 PNG

- 引入 bgfx、glm、stb；ImGui 改为 `InitForOther` + bgfx 提交。
- 实现轨道相机、视口纹理、离屏透明 PNG。

### 2026-08-19：Phase 0 骨架实现

- 建立 include/src/backends/tests/CMake/vcpkg/CI 骨架。

### 2026-08-19：加入剧本驱动的分镜画布

- 将分镜画布加入核心用户路径和 P0 范围。

### 2026-08-19：开发地图初始化

- 创建愿景红线、架构地图、路线图、状态、交接规则、编码规范、持续开发 Prompt 和模块契约。
