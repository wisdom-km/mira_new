# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 10 已完成
- **当前版本工作**：**UI-PRO（UI 专业化升级）** 已随 **0.1.2** 打包。见 [`ui-pro-upgrade/`](ui-pro-upgrade/README.md)，进度以 [`ui-pro-upgrade/26-UI-PRO-STATUS.md`](ui-pro-upgrade/26-UI-PRO-STATUS.md) 为准
- **最后更新**：2026-08-21
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`v0.1.2`
- **下一个允许执行的工作**：0.1.2 已发布后，后续功能只填进现有名词。不得新建 CMake 模块、不得接线真实 AI、暂不拆 `Application.cpp`

## 已完成

- [x] Phase 0–10：从骨架到 P0 发布准备
- [x] tag `phase-10-p0`、`phase-10-p1`、`v0.1.2`
- [x] Windows 安装包脚本：`packaging/windows/`

## 进行中

**UI-PRO 0.1.2**：W1–W3 已写入并打 Windows 安装包。设计与进度见 [`ui-pro-upgrade/`](ui-pro-upgrade/README.md)。

P0 路线图已完成；UI-PRO 之外的加删功能仍以 `07` 为准。

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
| 应用 logo | 暂定 `img/dog.png`；Windows 图标为 `img/directordesk.ico`；批准者 Wisdom |
| Demo 迭代策略 | 部分模块化：控制面保留，模块数锁定，AI 冻结为空岛；加删按 `07` 落点；原理见 `08`；暂不拆 `Application.cpp`；批准者 Wisdom |
| 当前画面镜头与关联 | 镜头检查器可选已有相机或新建机位；相机面孔不把当前画面镜头列入占用关联；删镜头/删相机都在左侧右键；批准者 Wisdom |
| 资源库缺失条目 | 网格不显示；可删索引记录，不删磁盘源文件；批准者 Wisdom |

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

- Windows Debug 测试：109 cases / 590 assertions
- 产品版本 **0.1.2**：UI-PRO 工作台 + 镜头可选已有相机 / 建新机位 + 左侧右键删镜头与相机 + 资源库清理缺失

## 下一步清单

1. 确认 GitHub Release `v0.1.2` 安装包可下载、安装后能打开咖啡馆示例
2. 确认 GitHub Actions Windows + macOS 全绿
3. 有 Mac 时按 `docs/RELEASE-CHECKLIST.md` 补实机回归
4. 后续功能只填进现有名词；不拆 `Application.cpp`，除非编排痛到无法安全改 visit

## 工作日志

### 2026-08-21：0.1.2 Windows 发布

- 产品版本升到 0.1.2：UI-PRO 四模式工作台、镜头/相机左侧右键删除、镜头检查器可选已有相机或建新机位、资源库隐藏并清理缺失条目、关闭弹窗文字对比度。
- 打包 `DirectorDesk-0.1.2-windows-x64.exe` / `.zip`，发布到 GitHub Releases tag `v0.1.2`。

### 2026-08-21：镜头可选已有相机、左侧删相机

- 镜头检查器增加已有相机下拉，并保留「为该镜头建机位」；当前选中镜头可以 `LinkShotToCameraCommand`。
- 左侧相机表右键删除；检查器不再删相机。相机面孔占用列表仍不把当前画面镜头列为关联目标。

### 2026-08-21：镜头关联、左侧删镜头、资源库清理缺失

- 当前画面镜头不再走 `LinkShotToCameraCommand`；检查器去掉「关联到当前相机」；相机面孔只让其他镜头关联。
- 删除镜头改到左侧镜头表右键菜单，检查器不再删镜头。
- 资源库不展示源文件缺失的本地条目；右键删除 +「清理缺失」去掉索引记录。Wisdom 批准追加 `DeleteShotCommand` / `RemoveLibraryAssetCommand`。
- Windows Debug：109 cases / 590 assertions。

### 2026-08-21：关闭确认弹窗文字对比度

- `WorkspacePanel` 三个 `BeginPopupModal` 的正文和按钮改为与标题相同的浅色 `#e9edf4`；ImGui 样式补齐 1.92 颜色并把弹窗背景设为不透明。
- 根因：自定义 ImGui 后端按命令顺序累计索引，忽略 `ImDrawCmd::IdxOffset`。模态变暗层会把 draw command 挪到列表前面，正文/按钮因此画错成暗色。

### 2026-08-21：UI-PRO W1–W3 代码落地

- 四个新 Command、统一选择快照、工作区模式 dock 重建、状态栏/工具条/层级/检查器/镜头条。
- 导演台九职责面板停用 `Begin`；空工程中央改为开始板。
- 未新建 CMake 模块、AI 仍冻结、未拆 `Application.cpp`。
- 测试当时 106 cases / 570 assertions。

### 2026-08-21：UI-PRO 设计集落地

- 新增 `docs/dev-map/ui-pro-upgrade/`（README + `20`–`26`）：UI 专业化升级版的范围、新主路径、区域契约、契约增量、任务表、停手清单与状态页。
- 依据 Phase 10 基线代码审计：`导演台###Workspace` 一栏九职责、三份互不相关的选择、`statusText` 藏在左栏底部、分镜总览只占中央下方三成。
- 定下八个稳定区域 ID、四个工作区模式（默认 `shoot`）、统一选择模型、15 个任务、4 个新 Command 与 6 组新快照字段。
- 仅文档改动：未改业务代码、未新建 CMake 模块、AI 仍冻结、未拆 `Application.cpp`。

### 2026-08-20：暂定应用 logo

- 源图 `img/dog.png`，Windows 多尺寸图标 `img/directordesk.ico`。
- `DirectorDesk.exe` 嵌入 `GLFW_ICON`；Inno Setup 使用同一图标和向导小图。

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
