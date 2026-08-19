# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 10 已完成
- **当前状态**：视口滚轮闪屏已修；`07`/`08` 已生效。知识库入口：`AGENTS.md`、GitHub Pages 目录页
- **最后更新**：2026-08-19
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`phase-10-p1`
- **下一个允许执行的工作**：按 `07` 在现有名词上加删功能。不得新建 CMake 模块、不得接线真实 AI、暂不拆 `Application.cpp`

## 已完成

- [x] Phase 0–10：从骨架到 P0 发布准备
- [x] tag `phase-10-p0`、`phase-10-p1`
- [x] Windows 安装包脚本：`packaging/windows/`

## 进行中

无。P0 路线图已完成；此后加删功能以 `07` 为准。

## 阻塞项

- **macOS 实机回归**：本机无 Mac。以 GitHub Actions `macOS` job 为门禁

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
| Windows 分发 | Inno Setup 安装包，发布到 GitHub Releases；批准者 Wisdom |
| Demo 迭代策略 | 部分模块化：控制面保留，模块数锁定，AI 冻结为空岛；加删按 `07` 落点；原理见 `08`；暂不拆 `Application.cpp`；批准者 Wisdom |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc | Phase 1 Windows 已验证 |
| 透明离屏渲染/回读 | Phase 1 技术切片 | Phase 1/7 Windows 已通过；macOS 待 CI |
| Windows 中文路径 | Platform UTF-8 边界 | Phase 0–7、10 已测 |
| 大型剧本卡顿 | 防抖、可见区、缓存上限 | Phase 7 |
| raw.githubusercontent.com 在部分网络环境不可用 | 清单刷新失败时使用最后有效缓存 | Phase 8 |
| 源文件移动后 ID 变化 | 稳定路径键 | Phase 5 |

## 本次验证

- Windows Debug 测试：104 cases / 564 assertions
- Windows 安装包随本 tag 发布

## 下一步清单

1. 确认 GitHub Actions Windows + macOS 全绿
2. 有 Mac 时按 `docs/RELEASE-CHECKLIST.md` 补实机回归
3. 后续功能只填进现有名词；不拆 `Application.cpp`，除非编排痛到无法安全改 visit

## 工作日志

### 2026-08-19：Pages 目录按 00–08 正序

- `docs/index.html` 开发地图入口改为 00→08，不再 08、07 倒着排。

### 2026-08-19：知识库 `08` 与 GitHub 入口

- 新增 `docs/dev-map/08-SEAMS-AND-VIBE-CODING.md`：接缝先于功能、好处与弊端、vibe coding 能/不能。
- 根目录 `AGENTS.md` 只指向开发地图。Pages `docs/index.html` 改为目录枢纽，图与正文仍各一处。
- 未改业务代码，未拆 `Application.cpp`。

### 2026-08-19：`07` 生效

- Wisdom 确认 `07-ITERATION-AND-LANDING.md` 无改动意见；去掉草稿标记，作为 Demo 现行迭代政策。
- 未改业务代码，未拆 `Application.cpp`。

### 2026-08-19：Demo 迭代政策

- 新增 `docs/dev-map/07-ITERATION-AND-LANDING.md`：模块数锁定、按名词落点、加/删分层、AI 冻结、暂不拆 Application.cpp。
- `01`/`04`/`06`、`CONTRIBUTING.md`、`README.md` 增加指向，避免第二份架构正文。

### 2026-08-19：0.1.1 Windows 发布

- 视口滚轮闪屏修复随 `phase-10-p1` 打 Windows 安装包。

### 2026-08-19：视口滚轮缩放闪屏

- 视口窗口不再把滚轮当成滚动条；离屏缩略图不再 `bgfx::reset` 成 320×180。
- 缩略图改为异步回读，避免在主循环里额外 `bgfx::frame()` 把清空后的交换链呈上去。
- 视口用 InvisibleButton 吃输入，尺寸变化小于 2px 不重建 RT。

### 2026-08-19：Phase 10 体验打磨与发布准备

- 统一中文空状态、快捷键与对外文档。
- 示例工程 `examples/cafe.ddproj`。
- Windows Inno Setup 安装包脚本，发布到 GitHub Releases。

### 2026-08-19：Phase 9–0

- AI 接口、官方资产库、分镜画布、工程文件、资源库、机位、剧本与渲染骨架。
