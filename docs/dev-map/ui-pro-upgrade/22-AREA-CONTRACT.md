# 22 - 区域契约（AREA CONTRACT）

> 八个区域 ID 是本版最重要的约定：写下后不再改名，用于把「只改 CENTER_STAGE」变成一句可执行指令。
> 每个区域写死：宿主窗口、归属（现有类 / 搬家 / 新职责）、主要 ImGui 控件、主按钮 Command、比例。
> 新增的窗口 ID 与 Command 签名见 [`23-CONTRACT-DELTA.md`](23-CONTRACT-DELTA.md)。

## 一、当前布局（改造前基线）

```text
┌──────────────────────────────────────────────────────────────────────┐
│ MENU BAR  DirectorDeskDockSpace   文件 / 镜头 / 帮助 ····· 工程名 ●   │
├──────────────┬───────────────────────────────────┬───────────────────┤
│ 导演台###    │ 视口###Viewport                   │ 剧本###Script     │
│ Workspace    │  工具条 34px：LIVE 尺寸 四个机位   │  打开/保存/示例    │
│  九职责合体   │  AddImage(viewportTextureIndex)   │  镜头树 170px 固定 │
│  层级 118px  │  InvisibleButton 独占输入          │  编辑器            │
│  变换检查器   │  底部 24px 操作提示                │  max(180,avail-116)│
│  相机 88px   ├───────────────────────────────────┤  诊断              │
│  绑定/机位/灯 │ 分镜###Storyboard  中央下方 30%    │                   │
│  statusText  │  真正的交付物，只有三成高           │  25% 宽写正文      │
├──────────────┤                                   │                   │
│ 资源库###    │                                   │                   │
│ Library 40%  │                                   │                   │
└──────────────┴───────────────────────────────────┴───────────────────┘
   左 21%                     中 54%                      右 25%
   （没有状态栏、没有工具条、没有模式切换）
```

## 二、目标默认工作区（`shoot` 掌机模式）

```text
┌──────────────────────────────────────────────────────────────────────┐
│ MENU_BAR    文件 · 视图 · 镜头 · 帮助 ················· cafe.ddproj ●│ 26px
├──────────────────────────────────────────────────────────────────────┤
│ TOOL_STRIP  [编剧][置景]▣掌机[审片] · S02-SH03 · 12镜/7已成镜 · 导出▾ │ 34px
├────────────┬────────────────────────────────────────┬────────────────┤
│LEFT_       │ CENTER_STAGE                           │RIGHT_INSPECTOR │
│HIERARCHY   │  HUD: S02-SH03 · CAM 过肩A · 1920×1080 │ 检查器 · 镜头   │
│ ▾第一场     │                                        │ S02-SH03(第二场)│
│  SH01 ●●○  │  LIVE 画面                              │ ─ 相机 ─        │
│  SH02 ●○○  │  AddImage(viewportTextureIndex)         │ CAM 过肩A ▾     │
│ ▾场景对象   │  左键旋转 右键平移 滚轮推拉               │ 取消关联         │
│  ◆cafe_room│  （机位按钮不在这里）                    │ 机位:正侧过俯特  │
│      58%   │                                  78%   │ ─ 画面 ─        │
├────────────┼────────────────────────────────────────┤ 灯光:中性 暖 冷  │
│LEFT_LIBRARY│ BOTTOM_STRIP                           │ 缩略图:就绪 重渲 │
│ 本地|在线   │ SH01▣ SH02▣ ▶SH03◀ SH04▢ SH05▢         │ ─ 交付 ─        │
│ 搜索 ▤▦     │ 缩略图 + 镜头号 + 三点状态         22%  │ 导出 1080p / 2K │
│      42%   │                                        │                │
├────────────┴────────────────────────────────────────┴────────────────┤
│ STATUS_BAR  已应用机位 over-shoulder · 缩略图 1/3 ··· VIEW 1920×1080 │ 22px
└──────────────────────────────────────────────────────────────────────┘
   左 20%                   中 56%                        右 24%
```

## 三、每个格子写死

### `MENU_BAR`

| 项 | 内容 |
|----|------|
| 宿主 | `DirectorDeskDockSpace` 的 `BeginMenuBar`（现有） |
| 归属 | 现有 `WorkspacePanel`；新增「视图」菜单 |
| 控件 | `BeginMenu` 文件 / 视图 / 镜头 / 帮助；右侧工程名 + 脏点（现有实现保留） |
| 主 Command | `NewProjectCommand` · `OpenProjectCommand` · `SaveProjectCommand` · `SaveProjectAsCommand` · `ImportModelCommand` · `QuitCommand` · **新** `SetWorkspaceModeCommand` · **新** `ResetLayoutCommand` |
| 比例 | 全宽 × 26px |
| 读 | `projectName` · `projectDirty` |

### `TOOL_STRIP`

| 项 | 内容 |
|----|------|
| 宿主 | **新**：主视口上沿侧边条（`ImGui::BeginViewportSideBar(ImGuiDir_Up)`，`imgui_internal.h` 已被 `WorkspacePanel.cpp` 引入） |
| 归属 | **新职责**，落在 `src/UI/WorkspacePanel.cpp` |
| 控件 | 四个模式切片（`Selectable` 或 `SmallButton` 高亮当前）· 当前 Shot 名 · 「N 镜 / M 已成镜」· 右侧导出主按钮 |
| 主 Command | **新** `SetWorkspaceModeCommand{modeId}` · `ExportCurrentShotCommand` · `ExportStoryboardBoardCommand` |
| 比例 | 全宽 × 34px |
| 读 | `workspaceModeId`（新）· `scriptScenes` · `storyboardCards` · `selectedShotLinkedCamera` |

### `LEFT_HIERARCHY`

| 项 | 内容 |
|----|------|
| 宿主 | **新窗口** `层级###Hierarchy` |
| 归属 | **搬家**：从 `导演台###Workspace` 拆出层级；新增「镜头表」分节 |
| 控件 | 三段 `CollapsingHeader`：镜头表（右键删除镜头）+ 场景对象 + 相机（右键删除相机） |
| 主 Command | `SelectShotCommand` · `SelectNodeCommand` · `SelectCameraCommand` · `InsertSceneCommand` · `InsertShotCommand` · `DeleteShotCommand` · `AddCameraCommand` · `RemoveCameraCommand` |
| 比例 | 左栏 20% × 上 58% |
| 读 | `scriptScenes` · `nodes` · `storyboardCards` |

### `LEFT_LIBRARY`

| 项 | 内容 |
|----|------|
| 宿主 | 现有 `资源库###Library` |
| 归属 | 现有 `LibraryPanel`，**减法**：chrome 压成一行，资产详情与下载移入 `RIGHT_INSPECTOR` |
| 控件 | 一行：来源 TabBar + 搜索 + 视图切换；其余空间全给资产网格 |
| 主 Command | `SetLibraryOriginFilterCommand` · `SetLibrarySearchCommand` · `SetLibraryViewModeCommand` · `SelectLibraryAssetCommand` · `AddLibraryAssetToSceneCommand` · `RefreshLibraryCommand` · `RefreshOfficialCatalogCommand` · `DownloadOfficialAssetCommand` · `CancelOfficialDownloadCommand` |
| 比例 | 左栏 20% × 下 42%（`set` 模式升为 70%） |
| 读 | `libraryAssets` · `librarySearch` · `libraryOriginFilter` · `libraryViewMode` · `officialCategories` · `officialCatalogStatus` · `officialConfigured` |

### `CENTER_STAGE`

| 项 | 内容 |
|----|------|
| 宿主 | **随模式换**：`视口###Viewport`（`shoot`/`set`）· `分镜###Storyboard`（`review`）· `剧本###Script`（`script`） |
| 归属 | **新路径**：`ApplyDefaultDockLayout` 接受模式参数并允许显式重建 |
| 控件 | `InvisibleButton` 吃输入 + `AddImage` / 自绘；顶部 28px 极简 HUD（当前 Shot · 绑定相机 · 分辨率）；**不再放机位按钮** |
| 主 Command | `OrbitDeltaCommand` · `ViewportResizeCommand` · `AddLibraryAssetToSceneCommand`（拖放 `DD_ASSET_ID`）· `SelectShotCommand`（分镜点击）· `SetScriptTextCommand`（编剧） |
| 比例 | 中列 56% × 上 78% |
| 读 | `workspaceModeId`（新）· `viewportTextureIndex` · `viewportTextureWidth/Height` · `storyboardCards` · `scriptText` |

> 为什么不永远是 3D：视口是取景器，只有置景与掌机需要它当主角。编剧阶段主角是文字，审片阶段主角是分镜总览。中央舞台随模式换宿主，是本版与现有代码最大的结构差异。

### `RIGHT_INSPECTOR`

| 项 | 内容 |
|----|------|
| 宿主 | **新窗口** `检查器###Inspector` |
| 归属 | **新职责**：唯一检查器，按 `selectionKind` 换面孔 |
| 控件 | `SeparatorText` 分节 + `BeginTable` 两列属性。镜头面：已有相机下拉 / 为该镜头建机位 / 取消关联、缩略图状态、导出状态、机位预设、灯光；节点面：三组 `DragFloat3`；相机面：改名、占用镜头列表（删相机在左侧右键） |
| 主 Command | `LinkShotToCameraCommand` · `UnlinkShotCommand` · `ApplyCameraPresetCommand` · `SetLightPresetCommand` · `SetNodeTransformCommand` · `RenameCameraCommand` · `AddCameraCommand` · `RefreshStoryboardThumbnailCommand` · **新** `BindShotToNewCameraCommand` |
| 比例 | 右列 24% 满高（`review` 模式上 26% 为小监视器） |
| 读 | `selectionKind`/`selectionId`/`selectionLabel`（新）· `nodes` · `cameras` · `selectedShotLinkedCamera` · `lightPresetId` · `scriptScenes` · `exportIssues`（新） |

### `BOTTOM_STRIP`

| 项 | 内容 |
|----|------|
| 宿主 | **新窗口** `镜头条###ShotStrip`（与分镜画布共用 `storyboardCards`，在 `src/UI/StoryboardPanel.cpp` 内第二个 `Begin`，不新增文件） |
| 归属 | **新职责** |
| 控件 | 水平滚动等宽格：缩略图（`thumbTexture`）+ 镜头号 + 三点状态；当前格顶边 accent；右键菜单重渲 |
| 主 Command | `SelectShotCommand` · `RefreshStoryboardThumbnailCommand` · `FocusStoryboardSelectionCommand` |
| 比例 | 中列 56% × 下 22% |
| 读 | `storyboardCards`（`kind == "shot"`、`thumbTexture`、`link`、`preview`、`exported`）· `scriptScenes` · `exportLog`（新，仅 `review` 模式） |

### `STATUS_BAR`

| 项 | 内容 |
|----|------|
| 宿主 | **新**：主视口下沿侧边条（`ImGui::BeginViewportSideBar(ImGuiDir_Down)`） |
| 归属 | **搬家**：`statusText` 从 `导演台###Workspace` 底部迁出 |
| 控件 | 左：`statusText`；中：后台指示（导入中 / 缩略图渲染中 / 下载中）；右：工程路径 + 视口尺寸 |
| 主 Command | 无，只读 |
| 比例 | 全宽 × 22px |
| 读 | `statusText` · `importInProgress` · `projectPath` · `viewportTextureWidth/Height` · `officialCatalogStatus` |

## 四、模式切换对照

| 区域 ID | `script` 编剧 | `set` 置景 | `shoot` 掌机（默认） | `review` 审片 |
|---------|---------------|------------|----------------------|---------------|
| `MENU_BAR` | 不变 | 不变 | 不变 | 不变 |
| `TOOL_STRIP` | 右侧动作 = 保存剧本 | 右侧 = 导入模型 | 右侧 = 导出当前镜头 | 右侧 = 导出分镜总览 |
| `LEFT_HIERARCHY` | 只留场次/镜头树 | 只留场景对象树 | 镜头表 + 场景对象 | 隐藏 |
| `LEFT_LIBRARY` | 隐藏 | **升主**：半屏资产网格 | 折叠抽屉 | 隐藏 |
| `CENTER_STAGE` | 剧本编辑器 | 视口（含格网） | 视口 LIVE + HUD | **分镜总览全尺寸** |
| `RIGHT_INSPECTOR` | 诊断 + 场次统计 | 节点变换检查器 | 镜头检查器（三面孔） | 小监视器 + 交付检查器 |
| `BOTTOM_STRIP` | 解析摘要：N 场 M 镜 K 诊断 | 压成一行缩略条 | **镜头条（主要导航）** | 导出记录 |
| `STATUS_BAR` | 不变 | 不变 | 不变 | 不变 |

## 五、三个状态，同一套 chrome

### 状态 1：`EMPTY`（`projectPath` 为空且 `scriptHasSnapshot` 为 false 且 `nodes` 为空）

| 区域 | 显示 |
|------|------|
| `CENTER_STAGE` | 开始板：打开示例工程（主按钮）/ 新建工程 / 打开工程…；下方一行流程提示。**不显示空的 3D 视口** |
| `RIGHT_INSPECTOR` | 标题「无选择」，不画任何空属性表。上手三步清单（勾选状态由快照推断）：① 打开剧本（读 `scriptHasSnapshot`）② 放一个场景对象（读 `nodes`）③ 给第一颗镜头绑机位（读 `selectedShotLinkedCamera`）；每步右侧就是那一步的按钮。底部列出六个快捷键 |
| `BOTTOM_STRIP` | 一句「镜头表为空 · 打开剧本后生成」+ 两个按钮（打开剧本… / 打开示例剧本）。不画占位卡片 |
| `TOOL_STRIP` | 四个模式切片可见但除掌机外置灰，让用户先看到流程全貌 |
| `STATUS_BAR` | 「就绪」+ 「尚未保存」 |

### 状态 2：核心工作态 `shoot`（有 Shot 被选中）

| 区域 | 显示 |
|------|------|
| `CENTER_STAGE` | LIVE 视口 + HUD（当前 Shot · 绑定相机 · 分辨率） |
| `RIGHT_INSPECTOR` | 顶部：Shot 标题 + 所属场次。相机分节：当前绑定名或「未关联」+ 已有相机下拉 + 「为该镜头建机位」+ 取消关联 + 五个机位预设。画面分节：灯光三选 + 缩略图状态 + 重渲。交付分节：导出状态 + 导出 1080p / 2K。若 `selectionKind` 是 `node` 或 `camera`，整块换成对应面孔 |
| `BOTTOM_STRIP` | 横向镜头条：每格缩略图 + 镜头号 + 三点状态；当前格顶边高亮；右键单格可重渲 / 跳到该镜头相机；自动把当前格滚进可视区 |
| `STATUS_BAR` | 最近一次 Command 反馈 + 后台进度 |

### 状态 3：审片 / 导出态 `review`

| 区域 | 显示 |
|------|------|
| `CENTER_STAGE` | 分镜总览全尺寸；未就绪卡片描边高亮；点卡片即选中该 Shot |
| `RIGHT_INSPECTOR` | 上半小监视器（复用 `viewportTextureIndex`，不新建纹理）。下半交付检查器：透明导出开关、分辨率、就绪清单逐条 + 单条修复按钮、两个导出按钮。**清单为空时这一节不画**，只留导出按钮 |
| `BOTTOM_STRIP` | 导出记录：分辨率 / 镜头 / 目标路径 / 结果；失败条目可点，选中对应 Shot |
| `STATUS_BAR` | 最近导出结果 + 「N 项未就绪」 |

## 六、视觉语言（已落地，继续沿用）

深色电影感调色板已在 `backends/imgui/ImGuiGlfwBackend.cpp` 生效，不是默认 ImGui 灰。**本版不引入第二套颜色。**

| 常量 | 值 | 用途 |
|------|----|------|
| `kCanvas` | `#0b0d12` | 菜单栏、标题栏、画布底、Modal 遮罩 |
| `kWindow` | `#101319` | 窗口底色 |
| `kSurface` | `#151922` | 子区域、弹窗、未选中 Tab |
| `kRaised` | `#1c222d` | FrameBg、Button、表头、选中 Tab |
| `kBorder` / `kBorderHot` | `#2b3340` / `#465265` | 结构描边 / 强调描边 |
| `kText` / `kMuted` | `#e9edf4` / `#8f9baa` | 正文 / 次要文字 |
| `kAccent` / `kAccentHot` | `#d89a4a` / `#ebb25f` | 分节标签、Tab 上沿、勾选、拖放靶。**琥珀只给「当前」和「可动」** |
| `kSelection` | `#31547d` | Header、文本选中、按钮激活 |

几何同样沿用：`WindowRounding 4` · `FrameRounding 3` · `FramePadding 8×5` · `ItemSpacing 7×6` · `WindowBorderSize 1` · `FrameBorderSize 0`。

**密度纪律**：写死高度的 `BeginChild(118px / 88px / 170px)` 是当前拥挤感的技术来源。本版规则——只有镜头条与 HUD 用固定高度，其余竖向空间一律按比例分配，让内容随窗口长高，而不是在小盒子里内部滚动。
