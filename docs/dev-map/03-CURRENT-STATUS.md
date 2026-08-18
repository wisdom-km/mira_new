# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 4（Windows 本地验收已通过）
- **当前状态**：多相机、一键预设机位、基础灯光预设、视口 XZ 地面格网；导出 PNG 不含格网。Phase 4 代码尚未提交
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`phase-3-script`（`a5d51b1`）
- **下一个允许执行的工作**：用户明确要求后提交 Phase 4。打 `phase-4-camera-presets` 前不得开始 Phase 5

## 已完成

- [x] Phase 0 骨架、日志、GLFW、ImGui docking、CI
- [x] Phase 1 bgfx、轨道相机、离屏透明 PNG
- [x] Phase 2 GLB/OBJ 导入、Scene Node、后台加载、数值 Transform
- [x] Phase 3 Markdown 剧本、镜头列表、CJK 字体
- [x] `phase-3-script` 已打 tag 并推送
- [x] CameraManager：新增/删除/重命名/切换相机，各自保留轨道参数
- [x] 正视、侧面、过肩、俯视、特写预设（无选择时落到世界原点附近的明确退化）
- [x] 中性/暖光/冷光基础灯光预设
- [x] 视口 XZ 地面格网，原点轴向可辨；离屏导出不绘制
- [x] 相机与灯光操作只走 Command

## 进行中

无。Phase 4 功能在 Windows 上已验收，等待用户授权提交。

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
| 完整性校验 | picosha2（SHA-256） |
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

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc，禁止手工产物 | Phase 1 已在 Windows 验证 |
| 透明离屏渲染/回读可能因后端差异失败 | 在正式 Export 前完成技术切片 | Phase 1 Windows 已通过；macOS 待 CI |
| Windows 中文路径与编码 | Platform 统一 UTF-8 边界并加入测试 | Phase 0/2/3 已测 |
| GLB 特性范围失控 | P0 限基础静态网格/材质，忽略骨骼和动画 | Phase 2 |
| GitHub 在部分网络环境不可用 | 缓存最后有效清单；镜像源仅列后续 | Phase 8 |
| 大型剧本的布局和缩略图刷新造成卡顿 | 确定性布局、可见区裁剪、防抖、单帧单任务和缓存上限 | Phase 7 |
| 多模型协作导致架构漂移 | 强制读地图、更新状态、执行 Phase 门禁 | 全程 |

## 本次验证

- Windows MSVC 19.44 + Ninja Debug 配置与构建成功
- `DirectorDeskTests` 全部通过（含预设确定性、无主体退化、多相机参数隔离、最后一台相机不可删）
- 导出路径 `BuildSceneView(..., showGroundGrid=false)`；视口传入 `true`
- 未实现镜头↔相机关联、工程文件、分镜画布

## 下一步清单

1. 用户明确要求后提交并推送 Phase 4 代码
2. 用户明确要求后再打 `phase-4-camera-presets`
3. 之后才能开始 Phase 5：本地资源库

## 工作日志

### 2026-08-19：Phase 4 预设机位与地面格网

- 实现 CameraManager、五种构图预设和三种灯光预设。
- 视口绘制 XZ 格网与红/蓝原点轴；离屏导出不画格网。
- UI 只发相机/灯光 Command。未实现线框、镜头关联或工程文件。

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
