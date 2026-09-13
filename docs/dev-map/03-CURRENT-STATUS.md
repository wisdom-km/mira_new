# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 10 已完成
- **上一版本**：**UI-PRO（UI 专业化升级）** 已随 **0.1.2** 打包。见 [`ui-pro-upgrade/`](ui-pro-upgrade/README.md)
- **当前版本工作**：**FOUNDATION（地基、镜头包与 Skills 融合）**，F0 已随 **`v0.1.3`** 发布（含并行 UI-CLARITY）。见 [`foundation-upgrade/`](foundation-upgrade/README.md)，进度以 [`foundation-upgrade/36-FOUNDATION-STATUS.md`](foundation-upgrade/36-FOUNDATION-STATUS.md) 为准
- **最后更新**：2026-09-14
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`v0.1.3`
- **下一个允许执行的工作**：FOUNDATION F1 — **FND-10**（先写 Dispatch 测试再搬 `Application.cpp`）。UI-CLARITY U1 其余等对应 `FND-xx`；UIC-32 等 F2。不得新建 CMake 模块、不得接线真实 AI；`Application.cpp` 允许在 App 目标内分文件（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）

## 已完成

- [x] Phase 0–10：从骨架到 P0 发布准备
- [x] tag `phase-10-p0`、`phase-10-p1`、`v0.1.2`、`v0.1.3`
- [x] Windows 安装包脚本：`packaging/windows/`

## 进行中

**FOUNDATION**：F0 已随 `v0.1.3` 发布。下一波 F1 地基从 **FND-10** 开始。设计与进度见 [`foundation-upgrade/`](foundation-upgrade/README.md)。

**UI-CLARITY（界面重设计）**：U0 + UIC-20 / 27 / 30 / 31 / 33 / 34 已随 `v0.1.3` 入库。进度以 [`ui-clarity/46`](ui-clarity/46-UI-CLARITY-STATUS.md) 为准。U1 其余等对应 `FND-xx`；UIC-32 等 F2。

**UI-PRO 0.1.2**：W1–W3 已写入并打 Windows 安装包，已收口。

P0 路线图已完成；升级版之外的加删功能仍以 `07` 为准。

## 阻塞项

- **UI-CLARITY**：不依赖 FND 的行已随 `v0.1.3` 入库。U1 其余等 `FND-xx`。左栏段头 `SetCursorScreenPos` 导致的 Debug abort 已修
- **F1**：门禁已开，下一行 **FND-10**
- **macOS CI / 实机**：云上 macOS job 已关；实机回归待有 Mac 时再开（Wisdom 2026-09-13）

## 已确认决策

| 决策 | 结论 |
|------|------|
| P0 平台 | Windows + macOS；先开发、验证 Windows |
| macOS CI job | 暂停云上 macOS job，待有 Mac 再开；批准者 Wisdom 2026-09-13 |
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
| 完整性校验 | picosha2（SHA-256）；工程文件统一改用 `Core::Sha256`（FOUNDATION 版改写，见 foundation-upgrade/35 第四节；任务 FND-06） |
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
| Demo 迭代策略 | 部分模块化：控制面保留，模块数锁定，AI 冻结为空岛；加删按 `07` 落点；原理见 `08`；`Application.cpp` 允许在 App 目标内分文件、不新建模块（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）；批准者 Wisdom |
| Skills 融合 | DirectorDesk 只消费 Skill 产出（`storyboard-import`）与分发 Skill（官方资产 `format: skill`），不在软件内运行；见 `foundation-upgrade/32`；批准者 Wisdom |
| 镜头元数据与镜头包 | 元数据以剧本引用块表达（`script-format 1.1`）；对下游 AI 的接口是镜头包 PNG + `.shot.json`；`.ddproj` 保持版本 1；批准者 Wisdom |
| 当前画面镜头与关联 | 镜头检查器可选已有相机或新建机位；相机面孔不把当前画面镜头列入占用关联；删镜头/删相机都在左侧右键；批准者 Wisdom |
| 资源库缺失条目 | 网格不显示；可删索引记录，不删磁盘源文件；批准者 Wisdom |
| UI-CLARITY Q1 取景框 | 视口默认锁导出比例，视图菜单可关；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q2 字号 | 默认 14，跟随 DPI；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q3 图标 | Lucide 子集 TTF，许可证 **ISC**，文件 `assets/fonts/lucide-dd.ttf`（说明见同目录 `README.md`；部分字形源自 Feather MIT）；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q4 色板 | 中性灰，只留橙色强调；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q5 3D 背景 | 视口与非透明导出改中性灰，透明导出不变；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q6 分镜 | 先做短期 UIC-14；长期 UIC-32 等 F2 之后；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q7 编剧底栏 | 隐藏，不留空槽；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q8 灯光 | 搬到检查器「场景」面孔；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q9 Command | 本版上限 2：`SelectAdjacentShotCommand`、`RevealPathCommand`；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q10 用户设置 | 用户目录 `settings.json`（`Paths::UserSettingsFile`），**不进** `.ddproj`；批准者 Wisdom 2026-09-13 |

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

- UIC-20：Windows Debug 构建绿；Catch2 **130 cases / 735 assertions**；`previewTexture` 默认 `0xFFFF`；缺失路径 / 空缓冲解码失败；中文路径 PNG 往返。无新 Command、无新 CMake 模块
- UIC-34：Windows Debug 构建绿；Catch2 **126 cases / 722 assertions**；`settings.json` 中文路径往返；咖啡馆 `.ddproj` 不含偏好键
- UIC-33：Windows Debug 构建绿；Catch2 **119 cases / 683 assertions**；`fonts/lucide-dd.ttf` 已拷到 exe 旁。1280×800 掌机顶栏四模式有图标；审片左侧 48px 条，点击镜头表展开浮层
- 热修 BeginSection：`abort()` 断言为 `SetCursorScreenPos` 撑窗；已改为 `SameLine` + `Dummy`。Catch2 **111 / 598**；编剧冷启动 4s 无 Assertion
- UIC-13 / 14 / 15：Windows Debug 构建绿；Catch2 **111 cases / 598 assertions**；`rg "LIVE\|BEAT MAP\|VIEW %u\|已成镜\|重渲" src/UI` 空。1280×800 掌机见资源库一排 chrome + chips；审片标题「分镜总览」、场次横幅、16:9 卡、无连线。1920×1080 掌机/审片无文字截断
- UIC-16：Windows Debug 构建绿；Catch2 **111 cases / 598 assertions**；1280×800 检查器上手三步纵排无截断；快捷键两列表格
- UIC-12：Windows Debug 构建绿；Catch2 **111 cases / 598 assertions**；审片分辨率 Combo；掌机选中镜头见 3×2 机位（含平视）；段头右侧 `+`；1280×800 无截断
- UIC-11：Windows Debug 构建绿；Catch2 **110 cases / 591 assertions**；置景无选中见灯光面孔；掌机无选中见上手三步（无灯光）；审片只有导出设置，无「导出当前镜头」
- UIC-02 / UIC-03：Windows Debug 构建绿；Catch2 **110 cases / 591 assertions**；`0x3a4a62` 已清；`src/UI` 无 `LIVE`；1280×800 掌机/置景与 1920×1080 掌机截图为中性灰底，HUD 在取景框左下
- UIC-17 / UIC-01：Windows Debug 构建绿；Catch2 全绿；空工程无左栏/底栏；视口默认 letterbox
- UIC-04 / UIC-05：Windows Debug 构建绿；Catch2 全绿；左栏两段固定；镜头条 SideBar 132px
- UIC-06 / UIC-10：Windows Debug 构建绿；Catch2 全绿；状态栏无路径/`VIEW`；顶栏无「N 镜 / 已成镜」
- UIC-09 / UIC-07：Windows Debug 构建绿；Catch2 全绿；`src`/`backends` 无 `0x31547d`，无 `CollapsingHeader`
- UIC-08：Windows Debug 构建绿；Catch2 全绿；本机 100% DPI 日志 `Loaded UI font … at 14.0px (DPI scale 1.00x1.00)`
- FND-01 已 push（`81a25af`）
- FND-02–06 / 08 / 09 已写入；Windows Debug **110 cases / 591 assertions**；产物只有 `dx11/` `spirv/`
- FND-07 文案与产品截图 / GIF 已写入 README

## 下一步清单

1. FOUNDATION F1：从 **FND-10** 开始（先写 Dispatch 测试再搬文件）
2. UI-CLARITY U1 其余等对应 `FND-xx`；UIC-32 等 F2 之后
3. 有 Mac 时按 `docs/RELEASE-CHECKLIST.md` 补实机回归

## 工作日志

### 2026-09-14：v0.1.3

- `CMakeLists.txt` 版本 0.1.3。F0 止血 + UI-CLARITY（U0 与不依赖 FND 的 U1/U2 行）一并入库。Windows CI 在 F0 基线已绿；macOS job 仍暂停。

### 2026-09-14：UIC-20 资源库预览纹理

- `LibraryAssetView.previewTexture`；App 主线程加载 sidecar / 官方 preview；网格 96×72，`0xFFFF` 画格式色块。无新 Command。Catch2 **130 / 735**。已随 `v0.1.3` 入库。

### 2026-09-14：UIC-34 用户设置

- `settings.json` 在 Platform 用户数据目录；取景框锁 / 网格 / 轴 / 三分线 / 安全框 / 视口背景 / 左栏折叠。无新 Command、不进 `.ddproj`。未 commit。

### 2026-09-13：UIC-33 Lucide 子集

- `assets/fonts/lucide-dd.ttf` 40 字形；ImGui MergeMode；审片/窄窗 48px 图标条与浮层。无新 CMake 目标、无第 3 个 Command。未 commit。

### 2026-09-13：UIC-31 打开导出文件

- `RevealPathCommand`；`://` / 空 / 相对路径被拒。审片导出记录与状态栏「打开」「文件夹」。本版 Command 上限 2 已用完。Catch2 **118 cases / 680 assertions**。未 commit。

### 2026-09-13：UIC-30 键盘切镜

- `SelectAdjacentShotCommand`；顶栏 ‹ › 与 `[` `]` / `↑↓`。30 镜回绕测试绿。Dispatch 等 FND-10。Catch2 **115 cases / 664 assertions**。未 commit。

### 2026-09-13：UIC-27 打开后自动选第一镜

- `OpenProject` / `LoadScript` / `--project` 后若无选中镜头，复用 `SelectShotCommand` 记录选第一镜并切到绑定机位。咖啡馆打开即过肩 1/8。Catch2 **112 cases / 616 assertions**。未 commit。

### 2026-09-13：咖啡馆示例本机全流程点过

- `--project examples/cafe.ddproj` 四模式可开。镜头条点过肩/特写/柜台正视，机位与画面跟着切。`1`/`2`/`4` 能进编剧/置景/审片。stderr 无 Assertion。导出另存为对话框未自动点。未 commit。

### 2026-09-13：咖啡馆测试剧本扩成 3 场 8 镜

- `examples/scripts/cafe.md` 与 `cafe.ddproj` 补全机位、三物体、暖光。开始板「打开示例」仍指向这两份文件。未 commit。

### 2026-09-13：热修 BeginSection abort

- 左栏段头不再用 `SetCursorScreenPos` 撑窗。未 commit。

### 2026-09-13：UIC-13 / UIC-14 / UIC-15

- 资源库一排 chrome；分镜短期画法；术语表全量替换。U0 01–17 齐。未 commit。

### 2026-09-13：UIC-16

- 上手三步纵排 + 快捷键表格。未 commit。

### 2026-09-13：UIC-12

- 分辨率 Combo；机位 3×2 + `eye-level`；段头统一 `+`。未 commit。

### 2026-09-13：UIC-11

- 检查器面孔互斥；`selectionKind="asset"`；灯光搬到场景面孔。未 commit。

### 2026-09-13：UIC-02 / UIC-03

- 3D / 非透明导出背景改中性灰；网格与轴降饱和。HUD 去掉 `LIVE` / 像素。未 commit。

### 2026-09-13：UIC-17 / UIC-01

- 空工程藏左栏底栏；视口锁定导出比例。未 commit。

### 2026-09-13：UIC-04 / UIC-05

- 左栏两段固定；镜头条 Down SideBar 132px。未 commit。

### 2026-09-13：UIC-06 / UIC-10

- 状态栏 28px；顶栏 segmented + 主按钮。未 commit。

### 2026-09-13：UIC-09 / UIC-07

- 中性色板；选中底改为 accent 18%。`AutoHideTabBar` + 层级透明段头。未 commit。

### 2026-09-13：UIC-08

- 默认 body=14 + DPI；`src/UI/UiFonts.h` 动态字号。未 commit。

### 2026-09-13：UI-CLARITY 开 U0

- 设计集六条修订已写入 `41`/`43`–`46`。U0 从 UIC-08 起。

### 2026-09-13：UI-CLARITY 设计集

- Wisdom 对 `40` 第九节 Q1–Q10 全部表态。新增 `41` `43` `44` `45` `46`。过目前不改 UI 代码。
- Lucide ISC 已记入决策表；子集字体在 `assets/fonts/lucide-dd.ttf`（UIC-33）。

### 2026-09-13：UI-CLARITY 调研报告

- 新增 `docs/dev-map/ui-clarity/`（README + `40`）：基于 `src/UI/*.cpp`、样式与四张实机截图的界面审计（所见非所得、布局不稳定、信息层级缺失、控件选择、文案、反馈六类共 40 条证据）、五条设计原则、八个区域的新内容方案、视觉系统（色板 / 字号 / 图标 / DPI）、术语表、改动清单草案 `UIC-xx`、待 Wisdom 决策 Q1–Q10。
- 区域 ID 沿用 UI-PRO；控制面约束不变。仅文档改动，未改 UI 代码。

### 2026-09-13：FND-07 产品截图

- `README.md` 补掌机 / 审片两张静图，以及编剧→置景→掌机→审片→文件导出菜单的 GIF。
- 启动参数 `--workspace-mode` 仅用于复拍；抓图脚本 `tools/capture-readme-shots.ps1`。

### 2026-09-13：暂停云上 macOS CI

- Wisdom：先只跑 Windows job；Mac 实机再编。`ci.yml` 矩阵去掉 `macos-latest`。

### 2026-09-13：CI macOS FileDialog 弃用

- macOS job 编过 metal 后死在 `FileDialogMac.mm` 的 `allowedFileTypes`（`-Werror`）。改为 `allowedContentTypes`。

### 2026-09-13：F0 FND-02–09

- shader 按平台裁剪；Ctrl+E 走当前分辨率；删资源库索引不标脏；视口 resize 只在变化时推；工程哈希改 `Core::Sha256Hex`。
- Issue/PR 模板；`release.yml` 打 `v*` 出草稿安装包；版本号只读 `CMakeLists.txt`。
- README 英文摘要、CI/Release 徽章、30 秒上手。截图 / GIF 未补。

### 2026-09-13：FND-01 Windows CI 生成器

- CI Windows runner 从 `windows-latest` 改为 `windows-2022`，对齐 `Visual Studio 17 2022` 预设。未改 Ninja。

### 2026-09-13：FOUNDATION 设计集落地

- 新增 `docs/dev-map/foundation-upgrade/`（README + `30`–`36`）：同类项目调研与差距、逐文件代码审查、Skills 三层融合、契约增量、五波任务表、停手清单、状态页。
- 依据 `v0.1.2` 全仓审查：CI 双红、打开 / 保存同步 IO、节点无删除、镜头只能追加、`RemoveShotSection` 文本扫描、官方资产持久化不符契约、两份 SHA-256、字符串判业务状态。
- 改写四条旧决策（App 内分文件、统一 SHA-256、`script-format 1.1`、`asset-manifest 2`），记录在 `foundation-upgrade/35` 第四节；Gizmo / Undo / PDF 待批准。
- 仅文档改动：未改业务代码、未新建 CMake 模块、AI 仍冻结。

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
