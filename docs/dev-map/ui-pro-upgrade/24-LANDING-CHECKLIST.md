# 24 - 落点清单（LANDING CHECKLIST）

> 每一行是一次**可独立完成、可独立回退**的改动。任务 ID 稳定，可以直接说「做 UIP-03」。
> 落点顺序不变（`../07` 第四节）：Panel → `Core::Command` → `Application.cpp` 的 visit → App 组装快照 → 更新 `../03` 与 [`26`](26-UI-PRO-STATUS.md)。
> 进度只在 [`26-UI-PRO-STATUS.md`](26-UI-PRO-STATUS.md) 记录，本文件只定义任务本身。

## 波次

| 波次 | 目标 | 是否需要新 Command / 新快照字段 |
|------|------|--------------------------------|
| **W1** | 先把「乱」修掉：搬家 + 空状态，界面立刻像专业工具 | 都不需要，纯 UI 重组 |
| **W2** | 主线成型：Shot 常驻、中央舞台随模式换宿主 | 需要 `SetWorkspaceModeCommand` 与 3 组快照字段 |
| **W3** | 打磨：省点击、可追溯、可恢复 | 需要其余 3 个新 Command |

W1 做完就应该能明显感到界面变专业；不做 W2/W3 也不破坏任何现有功能。

## W1 · 结构与密度（无新契约）

| ID | 区域 | 工作流步骤 | 改动类型 | 落点 | 读哪些 `AppViewState` | 发哪些 Command | 完成标准 |
|----|------|------------|----------|------|----------------------|----------------|----------|
| UIP-01 | `STATUS_BAR` | 全程反馈 | 搬家 | `src/UI/WorkspacePanel.cpp`，`BeginViewportSideBar(ImGuiDir_Down)` | `statusText` · `importInProgress` · `projectPath` · `viewportTextureWidth/Height` | 无 | 导演台面板底部不再有 STATUS 分节；任意 Command 的反馈无需滚动即可看到；窗口设 `NoDocking` |
| UIP-02 | `LEFT_HIERARCHY` | 03 选镜头 / 04 置景 | 搬家 + 新控件 | 新窗口 `层级###Hierarchy`，同文件 | `scriptScenes` · `nodes` · `storyboardCards` | `SelectShotCommand` · `SelectNodeCommand` · `InsertSceneCommand` · `InsertShotCommand` | 镜头表与场景对象在同一列；两者都不再是固定 118px 内滚小盒；镜头行右侧显示关联/缩略图/导出三点 |
| UIP-03 | `RIGHT_INSPECTOR` | 04 / 05 / 06 | 搬家 | 新窗口 `检查器###Inspector`，同文件 | `nodes` · `cameras` · `selectedShotLinkedCamera` · `lightPresetId` · `scriptScenes` | `LinkShotToCameraCommand` · `UnlinkShotCommand` · `ApplyCameraPresetCommand` · `SetLightPresetCommand` · `SetNodeTransformCommand` · `RenameCameraCommand` · `RemoveCameraCommand` · `AddCameraCommand` | 同一时刻只显示一种面孔；机位预设只剩这一处主入口（视口工具条上的四个 SmallButton 同时撤掉）；相机面能列出占用它的镜头 |
| UIP-04 | `CENTER_STAGE` | 01 建档 | 新控件 | 视口窗口内的空状态分支 | `projectPath` · `scriptHasSnapshot` · `nodes` · `exampleProjectPath` · `exampleScriptPath` | `NewProjectCommand` · `OpenProjectCommand` · `OpenProjectFromPathCommand` · `LoadScriptFromPathCommand` | 空工程首屏只有一个主行动区；四个面板不再各自说一句空话；「快速开始」不再默认折叠（内容并入开始板） |
| UIP-05 | `LEFT_LIBRARY` | 04 置景 | 减法 | `src/UI/LibraryPanel.cpp` | `libraryAssets` · `librarySearch` · `libraryOriginFilter` · `libraryViewMode` · `officialCategories` · `officialCatalogStatus` | 现有 Library 系列 Command 全部保留 | 进入资产网格前的 chrome 从四排降到一排；20% 宽下网格至少两列；资产详情与下载移入 `RIGHT_INSPECTOR` |

## W2 · 主线与模式（新契约见 [`23`](23-CONTRACT-DELTA.md)）

| ID | 区域 | 工作流步骤 | 改动类型 | 落点 | 读哪些 `AppViewState` | 发哪些 Command | 完成标准 |
|----|------|------------|----------|------|----------------------|----------------|----------|
| UIP-06 | `RIGHT_INSPECTOR` | 05 统一 focus | 新快照字段 | `IPanel.h` 加 `selectionKind`/`selectionId`/`selectionLabel`；`Application.cpp` 在三个 `Select*Command` 分支记录 | 新增三字段 | 无新 Command | 点镜头表的镜头 / 场景对象 / 相机，检查器立即换面孔且不闪；`selectionKind` 为 `"none"` 时显示上手三步 |
| UIP-07 | `TOOL_STRIP` | 01 / 05 / 10 | 新控件 + 新 Command | `BeginViewportSideBar(ImGuiDir_Up)`；`Command.h` 加 `SetWorkspaceModeCommand`；`Application.cpp` visit | `workspaceModeId` · `scriptScenes` · `storyboardCards` · `selectedShotLinkedCamera` | `SetWorkspaceModeCommand` · `ExportCurrentShotCommand` · `ExportStoryboardBoardCommand` | 四个模式切片可点、当前高亮；条上能看到当前 Shot 与「N 镜 / M 已成镜」；未知 `modeId` 不改变状态 |
| UIP-08 | `CENTER_STAGE` | 02 / 05 / 08 | 新路径 | `ApplyDefaultDockLayout` → `ApplyDockLayout(id, size, modeId, force)` | `workspaceModeId` · `viewportTextureIndex` · `viewportTextureWidth/Height` | `OrbitDeltaCommand` · `ViewportResizeCommand` · `AddLibraryAssetToSceneCommand` | 切模式后中央宿主窗口随之更换；视口输入仍由 `InvisibleButton` 独占并清零 `io.MouseWheel`；2px 阈值仍在；来回切模式十次不出现窗口丢失或纹理闪 |
| UIP-09 | `BOTTOM_STRIP` | 03 / 07 | 新控件 | 新窗口 `镜头条###ShotStrip`，在 `src/UI/StoryboardPanel.cpp` 内 | `storyboardCards`（`kind=="shot"`、`thumbTexture`、`link`、`preview`、`exported`）· `scriptScenes` | `SelectShotCommand` · `RefreshStoryboardThumbnailCommand` · `FocusStoryboardSelectionCommand` | 掌机模式下不打开分镜画布也能横向切镜头；当前格自动滚进可视区；缩略图无效（`0xFFFF`）时画占位而不是空白 |
| UIP-10 | `RIGHT_INSPECTOR` | 09 就绪清单 | 新快照字段 | `Application.cpp` 遍历已组装的 `storyboardCards` 生成 `exportIssues`（不改 Storyboard 接口） | `exportIssues` · `exportTransparent` · `exportStaleCount` | `SetExportTransparentCommand` · `RefreshStoryboardThumbnailCommand` · `LinkShotToCameraCommand` | 导出前能逐条看到「哪个镜头、为什么没就绪」，每条自带修复按钮；清单清空后原「缩略图未就绪」modal 不再出现 |
| UIP-11 | `CENTER_STAGE` | 02 编剧 | 搬家 | `src/UI/ScriptPanel.cpp`：结构树交给 `LEFT_HIERARCHY`，诊断交给 `RIGHT_INSPECTOR`，本窗口只留编辑器 | `scriptText` · `scriptExternalRevision` · `scriptDirty` · `scriptPath` | `SetScriptTextCommand` · `LoadScriptCommand` · `SaveScriptCommand` | `script` 模式下正文可用全宽书写；`scriptExternalRevision` 回填逻辑不变；编辑器不再是 `max(180, avail-116)` 的夹缝 |

## W3 · 打磨

| ID | 区域 | 工作流步骤 | 改动类型 | 落点 | 读哪些 `AppViewState` | 发哪些 Command | 完成标准 |
|----|------|------------|----------|------|----------------------|----------------|----------|
| UIP-12 | `RIGHT_INSPECTOR` | 05 一键成镜 | 新 Command | `Command.h` 加 `BindShotToNewCameraCommand`；`Application.cpp` visit 串 `cameras.Add()` + `links.Set()` | `selectedShotLinkedCamera` · `cameras` | `BindShotToNewCameraCommand` | 未关联的镜头点一次即得到「新相机 + 绑定 + 状态栏反馈」；无当前镜头时给明确提示而不是静默 |
| UIP-13 | `BOTTOM_STRIP` | 10 交付留痕 | 新快照字段 | `Application.cpp` 维护环形缓冲，经 `exportLog` 送出 | `exportLog` · `exportPendingPath` | `SelectShotCommand`（点失败条目定位） | 审片模式底部能看到最近若干次导出的路径与结果；失败条目可点回对应镜头 |
| UIP-14 | `MENU_BAR` | 全程 | 新控件 + 新 Command | `src/UI/WorkspacePanel.cpp` 新增「视图」菜单；`Command.h` 加 `ResetLayoutCommand` | `workspaceModeId` | `SetWorkspaceModeCommand` · `ResetLayoutCommand` | 布局被拖乱后一键恢复，不需要手删 `imgui.ini`；菜单与工具条的模式状态一致 |
| UIP-15 | `RIGHT_INSPECTOR` | 09 分辨率 | 新 Command | `Command.h` 加 `SelectExportResolutionCommand`；`Application.cpp` visit | `exportResolutionId` | `SelectExportResolutionCommand` · `ExportCurrentShotCommand` | 分辨率是一个持续选择而不是两个菜单项；`ExportCurrentShotCommand` 的既有 `resolutionId` 行为保持兼容 |

## 每个任务的 Definition of Done

除 `../05` 第十一节的通用 DoD，本版追加：

1. 该任务的「完成标准」列逐条可复现（人工点一遍，写进 [`26`](26-UI-PRO-STATUS.md) 的验证记录）。
2. 没有触碰 [`25-DO-NOT.md`](25-DO-NOT.md) 第三节的硬约束。
3. 面板里没有新增业务判断（三点状态、就绪原因、模式合法性都由 App 决定）。
4. 现有测试仍全绿；本版不要求为 UI 布局写单元测试，但**新增的 Command 语义**要在 App 层有测试或在 [`26`](26-UI-PRO-STATUS.md) 记录人工复现步骤。
5. 收工同步 [`26-UI-PRO-STATUS.md`](26-UI-PRO-STATUS.md) 与 `../03-CURRENT-STATUS.md`。
