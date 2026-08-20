# 26 - UI-PRO 当前状态

> 本版进度的唯一真实来源。每轮 vibe coding 开始先读这里，结束必须更新这里，并同步 `../03-CURRENT-STATUS.md`。
> 只记录状态与验证，不复制契约（契约在 [`22`](22-AREA-CONTRACT.md) / [`23`](23-CONTRACT-DELTA.md)）。

## 当前快照

| 项 | 值 |
|----|----|
| 版本 | UI-PRO（UI 专业化升级） |
| 阶段 | **0.1.2 已打包发布**（W1–W3 代码在安装包内） |
| 代码基线 | Phase 10 已完成，产品 tag `v0.1.2` |
| 最后更新 | 2026-08-21 |
| 更新者 | Cursor AI |
| 下一个允许执行的工作 | 0.1.2 发布后，后续功能只填进现有名词。不得新建 CMake 模块、不得接线真实 AI、暂不拆 `Application.cpp` |
| 已写入代码的改动 | 4 个 UI-PRO Command + Wisdom 追加的 `DeleteShotCommand` / `RemoveLibraryAssetCommand`；`AppViewState` 增量；面板按区域 ID 重组 |

## 任务进度

状态取值：`未开始` / `进行中` / `已完成` / `已放弃`。

### W1 · 结构与密度（无新契约）

| ID | 区域 | 任务 | 状态 | 验证记录 |
|----|------|------|------|----------|
| UIP-01 | `STATUS_BAR` | 状态栏搬家 | 已完成 | `BeginViewportSideBar(Down)`，`NoDocking`；导演台窗口已停用，STATUS 分节不再出现 |
| UIP-02 | `LEFT_HIERARCHY` | 镜头表 + 场景对象拆出 | 已完成 | 新窗口 `层级###Hierarchy`；镜头行右侧三点状态；无 118px 内滚小盒 |
| UIP-03 | `RIGHT_INSPECTOR` | 唯一检查器拆出 | 已完成 | 新窗口 `检查器###Inspector`；视口工具条机位按钮已撤；机位主入口在检查器 |
| UIP-04 | `CENTER_STAGE` | EMPTY 开始板 | 已完成 | 空工程视口显示三个主行动（示例 / 新建 / 打开），不再画空 3D |
| UIP-05 | `LEFT_LIBRARY` | 资源库减法 | 已完成 | chrome 压成一行；资产详情/下载改到检查器；网格按 84px 列宽 |

### W2 · 主线与模式

| ID | 区域 | 任务 | 状态 | 验证记录 |
|----|------|------|------|----------|
| UIP-06 | `RIGHT_INSPECTOR` | 统一 focus 快照字段 | 已完成 | `SelectShot/Node/Camera` 记录 `selectionKind`；检查器按面孔切换；`none` 显示上手三步 |
| UIP-07 | `TOOL_STRIP` | 模式条 + `SetWorkspaceModeCommand` | 已完成 | 四个切片 + 当前镜头 + N镜/M已成镜；未知 `modeId` 不改状态（visit 已写） |
| UIP-08 | `CENTER_STAGE` | 中央舞台随模式换宿主 | 已完成 | `ApplyDockLayout(id, size, modeId, force)`；视口仍 `InvisibleButton` + 清零滚轮 + 2px 阈值 |
| UIP-09 | `BOTTOM_STRIP` | 镜头条 | 已完成 | `镜头条###ShotStrip`；无效缩略图画占位；当前格 `SetScrollHereX` |
| UIP-10 | `RIGHT_INSPECTOR` | 导出就绪清单 | 已完成 | App 遍历 `storyboardCards` 组装 `exportIssues`；审片检查器逐条修复按钮 |
| UIP-11 | `CENTER_STAGE` | 剧本编辑器全宽 | 已完成 | `script` 模式才 `Begin` 剧本窗口；编辑器高度为剩余空间 |

### W3 · 打磨

| ID | 区域 | 任务 | 状态 | 验证记录 |
|----|------|------|------|----------|
| UIP-12 | `RIGHT_INSPECTOR` | 一键为镜头建机位 | 已完成 | `BindShotToNewCameraCommand`：`cameras.Add()` + `links.Set()`；无镜头时「请先选择镜头」 |
| UIP-13 | `BOTTOM_STRIP` | 导出记录 | 已完成 | App 环形缓冲 20 条；审片模式镜头条列出路径/结果；失败条目按标题回点镜头 |
| UIP-14 | `MENU_BAR` | 视图菜单 + 重置布局 | 已完成 | 菜单栏「视图」含四模式 + 重置布局；与工具条共用 `workspaceModeId` |
| UIP-15 | `RIGHT_INSPECTOR` | 导出分辨率选择 | 已完成 | `SelectExportResolutionCommand`；`ExportCurrentShotCommand` 空 `resolutionId` 回落到当前选择 |

## 契约落地追踪

| 契约 | 定义在 | 状态 |
|------|--------|------|
| `SetWorkspaceModeCommand` | [`23`](23-CONTRACT-DELTA.md) 一 | 已写入 `Command.h` + visit |
| `ResetLayoutCommand` | [`23`](23-CONTRACT-DELTA.md) 一 | 已写入 `Command.h` + visit |
| `BindShotToNewCameraCommand` | [`23`](23-CONTRACT-DELTA.md) 一 | 已写入 `Command.h` + visit |
| `SelectExportResolutionCommand` | [`23`](23-CONTRACT-DELTA.md) 一 | 已写入 `Command.h` + visit |
| `workspaceModeId` | [`23`](23-CONTRACT-DELTA.md) 二 | 已写入 `IPanel.h`，默认 `"shoot"` |
| `selectionKind` / `selectionId` / `selectionLabel` | [`23`](23-CONTRACT-DELTA.md) 二 | 已写入 `IPanel.h` + App 分发时记录 |
| `exportIssues` / `exportLog` / `exportResolutionId` | [`23`](23-CONTRACT-DELTA.md) 二 | 已写入 `IPanel.h` + App 组装 |
| `layoutRebuildRequested` | 实现附加（一帧脉冲） | 已写入 `IPanel.h`；Draw 后 App 清零 |
| `层级###Hierarchy` | [`23`](23-CONTRACT-DELTA.md) 三 | 已创建 |
| `检查器###Inspector` | [`23`](23-CONTRACT-DELTA.md) 三 | 已创建 |
| `镜头条###ShotStrip` | [`23`](23-CONTRACT-DELTA.md) 三 | 已创建 |
| `ApplyDockLayout(id, size, modeId, force)` | [`23`](23-CONTRACT-DELTA.md) 四 | 已替换；dockspace ID 改为 `DirectorDeskMainDockSpace.uipro`，避免旧 `imgui.ini` 钉死 Demo 布局 |
| `导演台###Workspace` | [`25`](25-DO-NOT.md) | 本轮停用 `Begin`，不留空窗口；稳定 ID 未改名 |

## 本版已确认决策

| 决策 | 结论 | 批准者 |
|------|------|--------|
| 主线对象 | Shot（镜头）常驻，不再是线性流程 | Wisdom |
| 打开软件第一件事 | 建档（开始板），不是空 3D 视口 | Wisdom |
| 工作区模式 | 四个写死：`script` / `set` / `shoot` / `review`，默认 `shoot` | Wisdom |
| `CENTER_STAGE` | 随模式换宿主窗口，3D 不永远居中 | Wisdom |
| 检查器 | 唯一一块，按 `selectionKind` 换面孔 | Wisdom |
| 区域 ID | 八个，写下不再改名 | Wisdom |
| `workspaceModeId` 持久化 | 本版不进 `.ddproj` | Wisdom |
| 新增 Command 数量 | 上限 4 个 | Wisdom |
| 配色与几何 | 沿用 `ImGuiGlfwBackend.cpp` 现有深色方案，不引入第二套 | Wisdom |

## 已知风险

| 风险 | 应对 |
|------|------|
| 模式切换重建 dock 时窗口丢失或纹理闪 | 切模式置 `layoutRebuildRequested`；视口输入路径未改。仍需人工来回切十次确认 |
| `导演台###Workspace` 拆空后残留死窗口 | 本轮不再 `Begin` 该窗口 |
| 检查器换面孔造成控件 ID 冲突 | 镜头 / 节点 / 相机面孔均 `PushID(selectionId)` |
| 新增快照字段的悬垂指针 | `const char*` 指向 App 当帧 `std::string`；容器传指针 |
| 侧边条与 DockSpace 抢空间导致布局跳动 | 菜单 / 工具条 / 状态栏在 `DockSpace` 之前 `BeginViewportSideBar`，均 `NoDocking` |
| 旧 `imgui.ini` 钉死 Demo 五窗布局 | dockspace ID 改为 `DirectorDeskMainDockSpace.uipro`；「视图 → 重置布局」可再强制重建 |

## 工作日志

### 2026-08-21：0.1.2 Windows 发布

- 随 UI-PRO W1–W3 打 `v0.1.2` 安装包。

### 2026-08-21：镜头可选已有相机、左侧删相机

- 镜头检查器：已有相机下拉 + 「为该镜头建机位」；当前选中镜头可以 `LinkShotToCameraCommand`。
- 左侧相机表右键删除；检查器不再删相机。

### 2026-08-21：镜头关联 / 删镜头 / 资源库缺失

- 当前选中（画面）镜头不参与 `LinkShotToCameraCommand`；左侧镜头表右键删除；资源库隐藏并清理缺失索引。
- 追加 Command：`DeleteShotCommand`、`RemoveLibraryAssetCommand`（Wisdom）。

### 2026-08-21：关闭确认弹窗文字对比度

- 三个模态（未保存的工程 / 覆盖导出 / 缩略图未就绪）正文与按钮改为与标题同一浅色 `#e9edf4`。
- `ApplyDirectorDeskStyle` 补齐 ImGui 1.92 颜色，弹窗背景不透明。
- 根因在 `ImGuiGlfwBackend::Submit`：必须用 `IdxOffset`/`VtxOffset`，不能按命令顺序累计索引。模态 dim 会把 command 插到 draw list 头部。

### 2026-08-21：W1–W3 代码落地

- 四个新 Command 进入 `Command.h` 的 `variant`；App visit 按 [`23`](23-CONTRACT-DELTA.md) 分发。
- `AppViewState` 增加工作区模式、统一选择、导出清单/记录/分辨率，以及一帧脉冲 `layoutRebuildRequested`。
- UI 按八个区域 ID 重组：状态栏、工具条、层级、检查器、开始板、镜头条；剧本编辑器只在编剧模式全宽；资源库 chrome 减到一行。
- 未新建 CMake 模块；AI 仍冻结；未拆 `Application.cpp`。
- Windows Debug：`DirectorDeskTests` 109 cases / 590 assertions 全绿（本轮追加 `DeleteShot` / `RemoveLibraryAsset` 与 Script/Asset 测试）。
- 界面四点（空工程开始板、四模式切换、镜头条、导出清单）待 Wisdom 本机点开 `DirectorDesk.exe` 确认。
