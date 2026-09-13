# 41 - 区域契约 v2（AREA CONTRACT V2）

> 八个区域 ID **不改名**（见 `../ui-pro-upgrade/22`）。本文件只改每个区域**里面**的内容、比例与面孔。
> 窗口稳定 ID 仍是：`###Viewport` `###Workspace` `###Script` `###Library` `###Storyboard` `###Hierarchy` `###Inspector` `###ShotStrip`。
> 新增 Command / 快照字段见 [`43`](43-CONTRACT-DELTA.md)。布局线框来自 [`40`](40-UI-AUDIT-AND-REDESIGN.md) 第五节；Q1–Q10 已由 Wisdom 拍板（2026-09-13）。

## 一、目标默认工作区（`shoot` 掌机，1280×800）

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ 文件 编辑 视图 镜头 帮助                                       ● 咖啡馆短片 │ MENU_BAR 26
├──────────────────────────────────────────────────────────────────────────────┤
│ [编剧][置景][掌机][审片]     ‹ 过肩 · 咖啡馆-日-内 (2/3) ›   ○○●  [导出镜头 ▾] │ TOOL_STRIP 40
├────────────┬───────────────────────────────────────────────┬─────────────────┤
│ 镜头表   + │           16:9 取景框（letterbox）              │ 镜头 · 过肩      │
│ 场景     + │           HUD: 过肩 · 主镜头 · 29mm             │ 相机 / 机位预设   │
│            │                                               │ （无灯光、无导出） │
├────────────┴───────────────────────────────────────────────┴─────────────────┤
│ [▣过肩] [▣特写] [▣俯拍]                                                      │ BOTTOM_STRIP 132
├──────────────────────────────────────────────────────────────────────────────┤
│ 已导出 过肩 → shot-cafe-001.png                    3 镜 · 2 就绪              │ STATUS_BAR 28
└──────────────────────────────────────────────────────────────────────────────┘
```

比例：左 18%（最小 220px）· 右 22%（最小 280px）· 底栏**固定 132px**（SideBar，不是 dock 比例）。设计与验收基准 **1280×800**（以及 1920×1080）；1024 宽不作验收。U0 不做窄窗图标条 / 浮层（U2 + UIC-33）。

## 二、每个格子写死

### `MENU_BAR`

| 项 | 内容 |
|----|------|
| 宿主 | `DirectorDeskDockSpace` 的 `BeginMenuBar`（不变） |
| 归属 | `WorkspacePanel` |
| 控件 | 文件 / **编辑** / 视图 / 镜头 / 帮助。右侧工程名 + dirty 点；hover 显示全路径 |
| 主 Command | 既有文件与镜头命令 · `SetWorkspaceModeCommand` · `ResetLayoutCommand` · U2：`RevealPathCommand`（帮助里「打开示例」仍走既有打开命令） |
| 视图菜单新增 | **锁定导出比例**（默认开，Q1）· 网格 / 三分线 / 安全框 / 坐标轴 · 折叠左栏 |
| 编辑菜单 | 复制 / 删除 / 显隐（U1 起发 FND-13 Command；U0 可先画禁用项或只列已有删除） |
| 帮助 | 「快捷键…」表格弹层 + 「打开示例工程」+ GitHub 链接（打开链接走系统，不新建模块） |
| 比例 | 全宽 × 26px |
| 读 | `projectName` · `projectDirty` · `projectPath`（只进 tooltip） |

### `TOOL_STRIP`

| 项 | 内容 |
|----|------|
| 宿主 | 主视口上沿 `BeginViewportSideBar(ImGuiDir_Up)` |
| 归属 | `WorkspacePanel` |
| 控件 | 左：四模式 **segmented**（图标 + 文字，当前实色 + accent 下划线），高 28，快捷键 `1`–`4`。中：`‹ 镜头名 · 场次 (序号/总数) ›`，`[` `]` / `↑↓` 切镜（U2 发 `SelectAdjacentShotCommand`）。中右：三点状态（机位 · 预览 · 导出），hover 图例。右：**一个**主按钮 |
| 主按钮 | 编剧「保存剧本」· 置景「导入模型 ▾」· 掌机「导出镜头 ▾」（1080p / 2K / 镜头包（FND-21 后）/ 设置…）· 审片「导出总览 ▾」 |
| 主 Command | `SetWorkspaceModeCommand` · `ExportCurrentShotCommand` · `ExportStoryboardBoardCommand` · `SaveScriptCommand` · `ImportModelCommand` · U2：`SelectAdjacentShotCommand` |
| 比例 | 全宽 × 40px |
| 读 | `workspaceModeId` · `scriptScenes` · `storyboardCards` · `selectedShotLinkedCamera` · `selectionId` |
| 去掉 | 「N 镜 / M 已成镜」文字（改到 `STATUS_BAR` 右侧） |

### `LEFT_HIERARCHY`

| 项 | 内容 |
|----|------|
| 宿主 | `层级###Hierarchy`（ID 不改） |
| 归属 | `WorkspacePanel` |
| 控件 | **两段固定**：镜头表 + 场景（对象 + 相机子段）。段头：caption 大写灰字 + 右侧 `+`，**不用**蓝色 `CollapsingHeader` 填色 |
| 镜头行 | 左侧 3px accent 竖条 = 当前；右侧三点状态等宽对齐 |
| 主 Command | 既有 `Select*` / `Insert*` / `DeleteShot` / `AddCamera` / `RemoveCamera` · 拖镜头到相机行 = `LinkShotToCameraCommand` · U1：`SetNodeVisible` / `DuplicateNode` / `DeleteNode` · U2：双击镜头 = `SetWorkspaceModeCommand{"shoot"}` + `SelectShotCommand` |
| 比例 | 左栏 18% × 上段（置景时库升高则镜头表仍在） |
| 读 | `scriptScenes` · `nodes` · `cameras` · `storyboardCards` · U1：`SceneNodeView.visible` |
| 四模式 | **都显示两段**。编剧：「场景」默认折叠。审片：左栏默认收成 dock 最小宽度，视图菜单「折叠左栏」可切换。48px 图标条与窄窗浮层挪到 U2（跟 UIC-33）。不再按模式删掉一整段 |

### `LEFT_LIBRARY`

| 项 | 内容 |
|----|------|
| 宿主 | `资源库###Library` |
| 归属 | `LibraryPanel` |
| 控件 | **一排**：`[本地 \| 在线]` segmented + 搜索占满 + `⋯` 溢出（导入 / 刷新 / 列表·网格 / 清理缺失）。下一排：分类 chips（可横滚） |
| 网格 | U0 仍可用色块占位；U1 用 `previewTexture` 96×72 |
| 主 Command | 既有 Library 系列全部保留 |
| 比例 | 左栏下段；置景升高，编剧 / 审片隐藏 |
| 读 | 既有 library 字段 · U1：`LibraryAssetView.previewTexture` |
| 选择 | 单击选中；拖到视口或双击发 `AddLibraryAssetToSceneCommand`。U0 起 App 把 `selectionKind` 写成 `"asset"`（字符串取值，不是新字段） |

### `CENTER_STAGE`

| 项 | 内容 |
|----|------|
| 宿主 | 随模式：`视口###Viewport` / `剧本###Script` / `分镜###Storyboard` |
| 视口 | **取景框**（Q1）：按 `exportResolutionId` 比例居中 letterbox；框外 `0x141414` 约 90% 暗幕；**RT 尺寸 = 取景框**，不是整个可用区。`ViewportResizeCommand` 语义不变，只改推上去的宽高 |
| 视口 HUD | 框内左下：`镜头 · 相机 · 焦距`（U0 无焦距则省略第三段；U1 读 `ShotHudView`）。右下：适配 / 重置轨道。**禁止** `LIVE` 与像素尺寸 |
| 视口辅助 | 网格 / 轴 / 安全框 / 三分线，UI 本地开关；`G` / `Shift+G`。不进 `.ddproj`（Q10：U2 末才持久化） |
| 视口空选 | 框内提示「从左侧镜头表选择一个镜头，或按 `]`」 |
| 剧本 | `InputTextMultiline` 保留。U0：工具条只显示文件名。U1：行号栏 + 当前镜头行高亮（`scriptSelectedLineStart/End`） |
| 分镜短期 | UIC-14：隐藏根卡与连线；场次卡拉成横幅；镜头卡缩略图锁 16:9；标题改「分镜总览」 |
| 分镜长期 | UIC-32（F2 之后）：`grid` 布局契约 |
| 主 Command | 既有 `OrbitDelta` · `ViewportResize` · 拖放 `DD_ASSET_ID` · `SetScriptText` · `SelectShot` |
| 比例 | 中列剩余；底栏不再按百分比切中央 |
| 读 | `workspaceModeId` · `viewportTexture*` · `exportResolutionId` · `storyboardCards` · `scriptText` · U1 HUD / 行号 |

> 取景框内画面必须与导出 PNG 同比例（[`40`](40-UI-AUDIT-AND-REDESIGN.md) 第十节验收 3）。`InvisibleButton` 仍覆盖**整个**可用区（含 letterbox 暗幕），滚轮清零规则不改。

### `RIGHT_INSPECTOR`

| 项 | 内容 |
|----|------|
| 宿主 | `检查器###Inspector` |
| 归属 | `WorkspacePanel` |
| 规则 | **同一时刻只一张面孔**。U0 起禁止把资产块追加在其他面孔下面（修 1089–1091） |
| 面孔 | 见下表 |
| 比例 | 右栏 22%，最小 280px |
| 读 | `selectionKind` / `selectionId` / `selectionLabel` · 各域快照 · U1 元数据 / 显隐 |

| 面孔 | 何时 | 内容 | 去掉 |
|------|------|------|------|
| 镜头 | 选中镜头（掌机 / 置景） | 标题（名 + 场次灰字）→ 相机下拉 + 「新建机位」主按钮 + 解绑 → 机位预设 3×2 → U1 元数据 → 底条：预览状态 + 「刷新预览」 | 灯光、分辨率、透明、导出按钮 |
| 场景对象 | 选中节点 | 名称 → `DragFloat3` + 复位 → 对齐（落地 / 面向相机 / 居中，FND-13 已含按钮）→ U1 显隐 / 复制 / 删除 | 长说明改 tooltip |
| 相机 | 选中相机 | 名称 → 设为取景 → 占用列表 → 可关联列表 | — |
| 场景 | 置景且无选中，或点「场景」段头（U0：面板本地 bool，见 `43` 一） | **灯光**（Q8 从镜头面孔搬来）→ 背景 → 网格开关 | — |
| 资产 | U0：`selectionKind == "asset"` | 大预览 → 名称 / 来源 / 格式 / 许可 → 「加入场景」主按钮 | 不再追加 |
| 导出 | 审片 | 16:9 监视器 → 分辨率下拉 + 透明背景 + 背景 → 未就绪清单 → 「导出总览」主按钮 | 「导出镜头」（在顶栏） |
| 剧本 | 编剧 | 场 / 镜统计 → 诊断（行号可点）→ FND-31 后「从分镜 JSON 导入」 | — |
| 空 | 无选中 | 上手三步纵排（真图标，不 `SameLine`）+ 快捷键两列表格 | `[x]` 字符 |

### `BOTTOM_STRIP`

| 项 | 内容 |
|----|------|
| 宿主 | `BeginViewportSideBar(viewport, ImGuiDir_Down, 132.0f)`，窗口名仍为 `镜头条###ShotStrip`（**ID 不改**）。不是 dock 窗口。`ApplyDockLayout` 不再切 `dockBottom`。状态栏同为 Down SideBar，贴在镜头条之下 |
| 掌机 / 置景 | 固定 132px；格 176×99（16:9）+ 标题 + 三图标；当前格 accent 边；横滚 |
| 编剧 | **不 `Begin`**（Q7） |
| 审片 | 「导出记录」：结果图标 + 镜头名 + 文件名；U2 起 `[打开][文件夹]` 发 `RevealPathCommand` |
| 空工程 | 与左栏一起隐藏（UIC-17） |
| 主 Command | `SelectShotCommand` · `RefreshStoryboardThumbnailCommand` · U2：`RevealPathCommand` |
| 读 | `storyboardCards` · `exportLog` |

### `STATUS_BAR`

| 项 | 内容 |
|----|------|
| 宿主 | `BeginViewportSideBar(ImGuiDir_Down, 28.0f)`，视觉贴底、在镜头条之下。Down 从底边占位，实现时先建本条再建镜头条 |
| 控件 | 高 **28px**。左：状态点（成功绿 / 警告橙 / 信息灰）+ 文字；导出成功时 U2 附打开链接。中：导入 / `加载模型 x/y`（FND-11）。右：`N 镜 · M 就绪` + dirty 点 |
| 主 Command | U0 无；U2：`RevealPathCommand` |
| 读 | `statusText` · `importInProgress` · `scriptScenes` · `storyboardCards` · U1：`sceneLoadPending/Total` |
| 去掉 | 工程全路径、`VIEW W×H`、「尚未保存」 |

## 三、模式对照（相对 UI-PRO `22` 第四节的差异）

| 区域 | `script` | `set` | `shoot` | `review` |
|------|----------|-------|---------|----------|
| `LEFT_HIERARCHY` | 两段都在；场景折叠 | 两段都在 | 两段都在 | 默认最小宽度，视图菜单可展开 |
| `LEFT_LIBRARY` | 隐藏 | 升主 | 下段 | 隐藏 |
| `BOTTOM_STRIP` | **隐藏** | 132px 镜头条 | 132px 镜头条 | 导出记录 |
| `CENTER_STAGE` | 剧本（文件名工具条） | 取景框视口 | 取景框 LIVE | 分镜总览（短期画法） |
| `RIGHT_INSPECTOR` | 剧本面孔 | 节点 / 场景 | 镜头 | 导出 + 监视器 |

## 四、三个状态

### `EMPTY`

中央开始板。打开工程 / 剧本后若无选中镜头，App 自动选第一镜（**UIC-27**，U1）。左栏与底栏 **隐藏**。检查器上手三步纵排。

### 核心 `shoot`

取景框 + 镜头面孔 + 132px 镜头条。顶栏中央永远是当前镜头。

### 审片 `review`

分镜总览全尺寸（短期无连线）。检查器导出面孔。底栏导出记录。
