# 46 - UI-CLARITY 当前状态

> 本版进度的唯一真实来源。每轮 vibe coding 开始先读这里，结束必须更新这里，并同步 `../03-CURRENT-STATUS.md`。
> 只记录状态与验证，不复制契约（[`41`](41-AREA-CONTRACT-V2.md) / [`43`](43-CONTRACT-DELTA.md)）或任务定义（[`44`](44-LANDING-CHECKLIST.md)）。

## 当前快照

| 项 | 值 |
|----|----|
| 版本 | UI-CLARITY（界面重设计） |
| 阶段 | **U0 已完成**（01–17 齐） |
| 代码基线 | 产品 tag `v0.1.3`（含本版 U0 与不依赖 FND 的行） |
| 最后更新 | 2026-09-14 |
| 更新者 | Cursor AI |
| 下一个允许执行的工作 | U1 其余等对应 `FND-xx`；UIC-32 等 F2。F1 已开，下一代码行是 FND-10 |
| 当前波次 | U0 已收口；UIC-20 / 27 / 30 / 31 / 33 / 34 已随 0.1.3 入库；U1 其余未开 |

## 任务进度

状态取值：`未开始` / `进行中` / `已完成` / `已放弃` / `待批准`。

### U0 · 快赢

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-01 | 视口取景框 letterbox | 已完成 | 默认锁 1080p/2K 比例；RT=frame；视图菜单可关；InvisibleButton 仍铺满 available。格网决策未改 |
| UIC-02 | 3D / 导出中性灰背景 | 已完成 | 清屏 `0x2b2b2e`；透明仍 `0x00000000`；网格主/次线 + 轴按 `43` 降饱和。`rg "0x3a4a62"` 空。已缓存蓝底缩略图等到该镜头标 stale，不特殊处理 |
| UIC-03 | HUD 去 LIVE / 像素 | 已完成 | 框内左下「镜头 · 相机」（无选中则「未选镜头」）；`rg "LIVE" src/UI` 空；不再画像素。HUD 不再占 28px 子窗口 |
| UIC-04 | 左栏两段固定 | 已完成 | 四模式都画镜头表+场景；编剧场景默认折；审片默认 72px，视图菜单「折叠左栏」。无图标条 |
| UIC-05 | 底栏 132px；编剧隐藏 | 已完成 | ShotStrip 为 Down SideBar 132px；`ApplyDockLayout` 不再切底；编剧不 Begin；审片标题「导出记录」；格 176×99 |
| UIC-06 | 状态栏 28px | 已完成 | 28px SideBar；无路径 / `VIEW`；右 `N 镜 · M 就绪` + dirty。路径改菜单工程名 tooltip |
| UIC-07 | AutoHideTabBar / 段头 | 已完成 | DockSpace + 叶节点 `AutoHideTabBar`；层级三段改为 caption + 分隔线；`src/UI` 无 `CollapsingHeader`。未拍四模式截图 |
| UIC-08 | 字号 14 + DPI | 已完成 | 本机 100% DPI 日志 `Loaded UI font … at 14.0px (DPI scale 1.00x1.00)`；只 AddFont 一次；`PushUiFont` 用于开始板标题与剧本编辑器。未拍 1280/1920 四模式截图 |
| UIC-09 | 中性色板 | 已完成 | `rg "0x31547d" src backends` 空；选中底=accent 18%；Modal `ButtonActive` 同步去掉选中蓝 |
| UIC-10 | 顶栏 segmented + 主按钮 | 已完成 | 40px；当前模式实色+accent 下划线；`1`–`4` 切模式；顶栏无「N 镜」。U0 无图标字体 |
| UIC-11 | 检查器面孔互斥；灯光搬家 | 已完成 | 只一张面孔；`SelectLibraryAsset` 写 `selectionKind="asset"`，其它 Select 清资产选中。灯光在置景无选中 / 点「场景」段头。镜头面孔无灯光与导出设置。网格开关无 Command，未做假控件 |
| UIC-12 | Combo / 3×2 / 段头 + | 已完成 | 审片分辨率 Combo；机位 3×2 含「平视」`eye-level`；镜头表 / 场景段头右侧 `+` 弹出菜单。1280×800 掌机选中镜头无截断 |
| UIC-13 | 资源库一排 chrome | 已完成 | 一排：本地/在线 segmented + 搜索占满剩余 + `⋯`（导入/刷新/列表·网格/清理缺失）；下一排 chips 横滚。网格列数现为 `max(2, avail.x/104)`（UIC-20 格 96×72） |
| UIC-14 | 分镜短期网格画法 | 已完成 | 不画 `root` 与连线；场次矮横幅（绘高 `min(h, 28*zoom)`，命中仍用契约 `h`）；镜头卡「已绑机位/无机位 · 最新/需刷新/失败/无」+ 16:9 缩略图。标题「分镜总览」+ 缩放%。未改 `x/y/w/h` |
| UIC-15 | 术语表 | 已完成 | `rg "LIVE\|BEAT MAP\|VIEW %u\|已成镜\|重渲" src/UI` 空。显示已换术语；逻辑仍 strcmp 快照串 `"已关联"` / `"就绪"` / `"过期"` / `"未关联"` / `"未关联相机"` 等（FND-17 / UIC-26）。空态带「打开剧本 / 示例 / 导入模型」 |
| UIC-16 | 上手三步纵排 | 已完成 | 禁用 Checkbox 作完成态；每步全宽按钮、无 `SameLine`；快捷键两列表格。`DrawOnboarding` 无 `[x]`。1280×800 无截断 |
| UIC-17 | 空工程藏左栏底栏 | 已完成 | 空工程不 Begin 层级 / 资源库 / 镜头条；dock 只留视口+检查器；空↔有内容会重建布局 |

### U1 · 快照

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-20 | 资源库 previewTexture | 已完成 | `LibraryAssetView.previewTexture`；App 主线程 decode sidecar / 官方 preview，每帧最多 2 张；离开当前查询集则 Destroy。`0xFFFF` 画格式色块。网格 96×72。无新 Command。Catch2 130 / 735 |
| UIC-21 | HUD 焦距 | 未开始 | 依赖 FND-21 |
| UIC-22 | 剧本行高亮 | 未开始 | 依赖 FND-15 |
| UIC-23 | 显隐 / 复制 / 删除入口 | 未开始 | 依赖 FND-13 |
| UIC-24 | 镜头元数据分节 | 未开始 | 依赖 FND-20 |
| UIC-25 | 加载进度 | 未开始 | 依赖 FND-11 |
| UIC-26 | 枚举状态图标 | 未开始 | 依赖 FND-17 |
| UIC-27 | 打开后自动选第一镜 | 已完成 | `--project cafe` 掌机顶栏「过肩 1/8」、检查器过肩机位，不再走上手三步。无新 Command。Catch2 112 / 616 |

### U2 · Command 与资源

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-30 | SelectAdjacentShotCommand | 已完成 | `[` `]` / `↑↓` 与顶栏 ‹ › 发 Command；`AdjacentShotId` 回绕；30 镜走一圈测试。Dispatch 等 FND-10。Catch2 115 / 664 |
| UIC-31 | RevealPathCommand | 已完成 | `://` / 空 / 相对路径被拒且不调系统；审片导出记录与状态栏「打开」「文件夹」。Dispatch 等 FND-10。Catch2 118 / 680 |
| UIC-32 | 分镜 grid 契约 | 未开始 | F2 之后 |
| UIC-33 | Lucide 子集 | 已完成 | `assets/fonts/lucide-dd.ttf` 40 字形；MergeMode 叠 CJK；审片/窄窗 48px 条 + 浮层。无新 CMake 目标。Catch2 119 / 683。1280×800 掌机/审片已拍 |
| UIC-34 | 用户设置文件 | 已完成 | `%UserData%/DirectorDesk/settings.json`；不进 `.ddproj`。无新 Command。Catch2 126 / 722 |

## 契约落地追踪

| 契约 | 定义在 | 状态 |
|------|--------|------|
| `SelectAdjacentShotCommand` | [`43`](43-CONTRACT-DELTA.md) 一 | 已写入 |
| `RevealPathCommand` | [`43`](43-CONTRACT-DELTA.md) 一 | 已写入 |
| `previewTexture` | [`43`](43-CONTRACT-DELTA.md) 二 | 已写入 |
| `ShotHudView` / `scriptSelectedLine*` | [`43`](43-CONTRACT-DELTA.md) 二 | 未写入 |
| `SelectionKind::Asset` | [`43`](43-CONTRACT-DELTA.md) 二 | 未写入（待 FND-17） |
| 样式常量表 / 字号 | [`43`](43-CONTRACT-DELTA.md) 三、四 | 字号与色板已写入 |
| Lucide 子集 TTF | [`43`](43-CONTRACT-DELTA.md) 五 | 已加入 `assets/fonts/lucide-dd.ttf`（ISC，见 `03`） |
| 用户设置文件 | [`43`](43-CONTRACT-DELTA.md) 七 | 已写入 `settings.json`（非 `.ddproj`） |

## 本版已确认决策

| 决策 | 结论 | 批准者 |
|------|------|--------|
| Q1 取景框默认锁导出比例 | 是；视图菜单可关 | Wisdom 2026-09-13 |
| Q2 默认字号 14 + DPI | 是 | 同上 |
| Q3 图标字体 | Lucide 子集，ISC，记入 `03` | 同上 |
| Q4 色板 | 中性灰，只留橙强调 | 同上 |
| Q5 3D / 非透明导出背景 | 一起改中性灰 | 同上 |
| Q6 分镜网格纸 | 先 UIC-14；UIC-32 等 F2 之后 | 同上 |
| Q7 编剧隐藏底栏 | 是 | 同上 |
| Q8 灯光 | 搬到场景面孔 | 同上 |
| Q9 新 Command | 仅 `SelectAdjacentShot` + `RevealPath`，上限 2 | 同上 |
| Q10 用户设置文件 | 推到 U2 末，不进 `.ddproj` | 同上 |

## 工作日志

### 2026-09-14：随 v0.1.3 入库

- 不依赖 FND 的 UI 行随产品 tag 发布。U1 其余仍等对应 `FND-xx`。

### 2026-09-14：UIC-20

- 资源库网格读 `previewTexture`；无纹理画格式色块。App 对当前查询结果主线程上传 sidecar / 官方 preview PNG，不可见条目保持 `0xFFFF`。解码走 Asset 公共 `ImageDecode`。未 commit。

### 2026-09-14：UIC-34

- 取景框锁、网格 / 轴 / 三分线 / 安全框、视口背景、左栏折叠写入用户目录 `settings.json`。面板持有偏好，App 读盘/落盘。导出仍不含格网。未 commit。

### 2026-09-13：UIC-33

- Lucide 40 字形子集入仓；`LoadUiFont` MergeMode；顶栏模式 / 切镜 / 三点状态 / 段头 `+` / 资源库 `⋯` 用图标。
- 审片默认与窗口 <1180：左栏改 48px 图标条，点击展开 `###Hierarchy` / `###Library` 浮层，不改 `###` 后缀。未 commit。

### 2026-09-13：UIC-31

- `RevealPathCommand` + `Platform::RevealPath`（Win32 `SHOpenFolderAndSelectItems` / ShellExecute；macOS `NSWorkspace`）。UI 不调系统 API。本版 2 个新 Command 已齐。未 commit。

### 2026-09-13：UIC-30

- `SelectAdjacentShotCommand`；顶栏 ‹ › + `[` `]` / `↑↓`。无镜头不改状态；无当前则 `]` 第一镜、`[` 最后一镜。Dispatch 等 FND-10。未 commit。

### 2026-09-13：UIC-27

- 打开工程 / 剧本 / `--project` 后若无选中镜头，自动选第一镜并切绑定机位。无新 Command。未 commit。

### 2026-09-13：咖啡馆示例本机全流程

- 实机点过编剧/置景/掌机/审片。打开后仍是「未选镜头」（UIC-27 未做）。镜头条选镜后检查器出 3×2 机位。未 commit。

### 2026-09-13：咖啡馆测试剧本

- 示例扩成 3 场 8 镜、7 机位、3 物体。路径未改：`examples/cafe.ddproj`、`examples/scripts/cafe.md`。未 commit。

### 2026-09-13：热修 BeginSection abort

- Debug `abort()` 不是正常流程。断言：`SetCursorScreenPos()` / 同行 `Dummy` 撑开窗口边界（imgui.cpp）。`BeginSection` 只留 `InvisibleButton` + `SameLine(+)`；镜头条「无预览」改 `AddText`。
- 点左栏 `+` / 进编剧都会走到这段头。掌机/编剧/置景/审片冷启动各 4s 无 Assertion。未 commit。

### 2026-09-13：UIC-13 / UIC-14 / UIC-15（U0 收口）

- 资源库 chrome 收到一排 + chips；分镜短期画法；术语表与空态带动作。无新 Command、无新快照字段。未 commit。
- 验证：Windows Debug 构建绿；Catch2 **111 cases / 598 assertions**。1280×800 掌机/置景/审片与 1920×1080 掌机/审片已拍（`build/uic-preview/`）。审片无 `BEAT MAP`，检查器为「透明背景 / 刷新预览 / 导出总览」。
- 已知限制：编剧冷启动 Debug `abort()` 已修（见下方 2026-09-13 热修）。

### 2026-09-13：UIC-16

- 上手三步改为复选框 + 纵排全宽按钮；快捷键改为两列表格。未 commit。

### 2026-09-13：UIC-12

- 导出分辨率改为 Combo；机位预设 3×2，第六格「平视」走已有 `ApplyCameraPresetCommand`（`eye-level`）。
- 层级段头右侧统一 `+`：镜头表→场次/镜头，场景→添加相机。未 commit。

### 2026-09-13：UIC-11

- 检查器面孔互斥；点资源库写 `selectionKind="asset"`，不再把资产块追加在其它面孔下。
- 灯光搬到场景面孔（置景无选中或点左栏「场景」段头）；镜头面孔只留相机 / 预设 / 刷新预览。导出设置只在审片。未 commit。

### 2026-09-13：UIC-02 / UIC-03

- 视口与非透明导出清屏改为 `0x2b2b2e`；网格主线 `0x5a5a60` / 次线 `0x3c3c42`；X/Z 轴降饱和。
- HUD 改为取景框左下「镜头 · 相机」，去掉 `LIVE` 与像素。未 commit。

### 2026-09-13：UIC-17 / UIC-01

- 空工程只留开始板 + 检查器。
- 视口默认锁定导出比例 letterbox；关锁后铺满。未 commit。

### 2026-09-13：UIC-04 / UIC-05

- 左栏两段固定；审片默认收窄 + 视图菜单折叠。无 48px 图标条。
- 镜头条改为 Down SideBar 132px，编剧不画。未 commit。

### 2026-09-13：UIC-06 / UIC-10

- 状态栏 28px；去掉全路径与 `VIEW`；右侧 `N 镜 · M 就绪`。路径在菜单栏工程名 hover。
- 顶栏 40px segmented + 中央镜头名 + 右侧唯一主按钮；`1`–`4` 切模式。掌机主按钮下拉 1080p / 2K。

### 2026-09-13：UIC-07

- `DockSpace` 与 dock 叶节点设 `AutoHideTabBar`。
- 层级段头改为 `BeginSection`（caption 灰字 + 底部分隔线）。`+` 仍在段内，UIC-12 再收到段头右侧。
- 剧本 / 资源库 / 分镜 / 层级 / 检查器顶部补 caption，避免藏 tab 后无名。

### 2026-09-13：UIC-09

- `ApplyDirectorDeskStyle` 换成 `43` 第三节中性色板；选中底 `kAccent` × 18%。
- Modal 与面板本地 `kMuted`/`kText` 对齐。未改 3D 清屏（UIC-02）。

### 2026-09-13：UIC-08

- `LoadUiFont` 按 `glfwGetWindowContentScale` 加载一次 body=14，并 `ScaleAllSizes`。
- 新增 `src/UI/UiFonts.h`：`PushFont(nullptr, designPx * FontSizeBase/14)`。开始板主标题 `display`，剧本编辑器 `editor`。
- Windows Debug 构建绿；Catch2 全绿。本机 100% DPI 日志确认 14.0px。

### 2026-09-13：设计集按 Wisdom 六条修订

- 底栏改 SideBar；U0 `selectionKind="asset"`；审片 U0 只做最小宽度；验收基准 1280×800；字号改为单字体 + `PushFont`；补 UIC-27。

### 2026-09-13：设计集落地

- 新增 `41` `43` `44` `45` `46`；更新本文件夹 README、`../03`、根目录 `AGENTS.md`。
- Wisdom 对 `40` 第九节 Q1–Q10 表态已写入上表。
- 未改业务代码、未新建模块、AI 仍冻结。
