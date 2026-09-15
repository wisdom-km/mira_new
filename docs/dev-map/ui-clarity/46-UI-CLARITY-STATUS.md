# 46 - UI-CLARITY 当前状态

> 本版进度的唯一真实来源。每轮 vibe coding 开始先读这里，结束必须更新这里，并同步 `../03-CURRENT-STATUS.md`。
> 只记录状态与验证，不复制契约（[`41`](41-AREA-CONTRACT-V2.md) / [`43`](43-CONTRACT-DELTA.md)）或任务定义（[`44`](44-LANDING-CHECKLIST.md)）。

## 当前快照

| 项 | 值 |
|----|----|
| 版本 | UI-CLARITY（界面重设计） |
| 阶段 | **U0 已完成**（01–17 齐） |
| 代码基线 | 产品 tag `v0.1.3`（含本版 U0 与不依赖 FND 的行） |
| 最后更新 | 2026-09-16 |
| 更新者 | Cursor AI |
| 下一个允许执行的工作 | **UIC-58 待批准**（AI「测试连接」需第 3 个 Command）。U3 其余已完成 |
| 当前波次 | U0～U3 允许行已写入；仅 UIC-58 待 Wisdom 批准 |

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
| UIC-15 | 术语表 | 已完成 | `rg "LIVE\|BEAT MAP\|VIEW %u\|已成镜\|重渲" src/UI` 空。显示已换术语。FND-17 后逻辑走 `SelectionKind` / `CardKind` / `linked` 与原因码；UIC-26 图标仍未开。空态带「打开剧本 / 示例 / 导入模型」 |
| UIC-16 | 上手三步纵排 | 已完成 | 禁用 Checkbox 作完成态；每步全宽按钮、无 `SameLine`；快捷键两列表格。`DrawOnboarding` 无 `[x]`。1280×800 无截断 |
| UIC-17 | 空工程藏左栏底栏 | 已完成 | 空工程不 Begin 层级 / 资源库 / 镜头条；dock 只留视口+检查器；空↔有内容会重建布局 |

### U1 · 快照

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-20 | 资源库 previewTexture | 已完成 | `LibraryAssetView.previewTexture`；App 主线程 decode sidecar / 官方 preview，每帧最多 2 张；离开当前查询集则 Destroy。`0xFFFF` 画格式色块。网格 96×72。无新 Command。Catch2 130 / 735 |
| UIC-21 | HUD 焦距 | 已完成 | `ShotHudView` 由 App 填 `Export::VerticalFovToFocalLength35mm`；无镜头 `nullptr`，无机位第三段省略。UI 只格式化 `%.0fmm`，不重算 FOV。Dispatch 比对与镜头包公式同值。Catch2 **191 / 1162**。未拍 1280/1920 截图 |
| UIC-22 | 剧本行高亮 | 已完成 | 行号栏 + `[scriptSelectedLineStart, End]` accent 6% 底。Dispatch 测选中镜行号区间。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-23 | 显隐 / 复制 / 删除入口 | 已完成 | 随 FND-13：层级右键 + 眼睛；检查器显隐 / 复制 / 删除。`NodeView.visible`；`BuildSceneView` 跳过隐藏节点 |
| UIC-24 | 镜头元数据分节 | 已完成 | 随 FND-20：检查器「元数据」五推荐键 → `SetShotMetaCommand`；改一键只动剧本文本一行 |
| UIC-25 | 加载进度 | 已完成 | 状态栏中段「加载模型 x/y」；`sceneLoadPending/Total`；pending=0 不显示。随 FND-11。Catch2 141 / 794 |
| UIC-26 | 枚举状态图标 | 已完成 | `StoryboardCardView` 增 `linkEnum` / `previewEnum` / `exportEnum`。面板不再用 `"就绪"` / `"未关联"` 判断。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-27 | 打开后自动选第一镜 | 已完成 | `--project cafe` 掌机顶栏「过肩 1/8」、检查器过肩机位，不再走上手三步。无新 Command。Catch2 112 / 616 |

### U2 · Command 与资源

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-30 | SelectAdjacentShotCommand | 已完成 | `[` `]` / `↑↓` 与顶栏 ‹ › 发 Command；`AdjacentShotId` 回绕；30 镜走一圈测试。Dispatch 等 FND-10。Catch2 115 / 664 |
| UIC-31 | RevealPathCommand | 已完成 | `://` / 空 / 相对路径被拒且不调系统；审片导出记录与状态栏「打开」「文件夹」。Dispatch 等 FND-10。Catch2 118 / 680 |
| UIC-32 | 分镜 grid 契约 | 已完成 | `storyboard.layout` 接受 `grid`（默认）与 `left-to-right`。`.ddproj` 仍为 v1。每场横幅、镜头按列换行；总览 PNG 走同一 `BuildLayout`。Catch2 **223 / 1394**。未 commit |
| UIC-33 | Lucide 子集 | 已完成 | `assets/fonts/lucide-dd.ttf` 40 字形；MergeMode 叠 CJK；审片/窄窗 48px 条 + 浮层。无新 CMake 目标。Catch2 119 / 683。1280×800 掌机/审片已拍 |
| UIC-34 | 用户设置文件 | 已完成 | `%UserData%/DirectorDesk/settings.json`；不进 `.ddproj`。无新 Command。Catch2 126 / 722 |

### U3 · 专业化与适配（定义在 [`47`](47-FULL-FLOW-AUDIT.md) 七 / [`44`](44-LANDING-CHECKLIST.md) U3）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| UIC-40 | 场景行名字（B-01） | 已完成 | `Selectable` 宽改为 `avail.x - 28`（负宽会变成 0）。2560 宽掌机场景行「立方体 / 桌子 / 杯子」可见、眼睛在右。Catch2 **214 / 1319**。无新 Command。未 commit |
| UIC-41 | `▾` → `Icon::ChevronDown`（B-02） | 已完成 | 掌机主按钮「导出镜头」+ Lucide chevron-down，不再画 `?`。帮助 / 上手三步 / 切镜 tooltip 的 `↑↓` 同步改图标。`rg "▾" src/UI` 空。Catch2 **214 / 1319**。截图 `build/qa-e2e/uic41-{1280,1920,2560}-{script,set,shoot,review}.png`。无新 Command。未 commit |
| UIC-42 | 元数据标签表格（B-03） | 已完成 | 两列 `BeginTable`：左标签（「负面提示词」实测宽 + 8，下限 56px）、右 `InputText("##key")`。1280 掌机可见「景别 / 运镜 / 时长 / 提示词 / 负面提示词」。Catch2 **214 / 1319**。截图 `uic42-{1280,1920,2560}-{script,set,shoot,review}.png`。无新 Command。未 commit |
| UIC-43 | `projectIsEmpty` 由 App 给（B-04） | 已完成 | 新字段 `bool projectIsEmpty`：无路径、无剧本文本、无用户节点（占位立方体算空）。无工程启动出开始板；上手「打开剧本」未勾。Catch2 **215 / 1327**。截图 `uic43-{1280,1920}-empty.png` + cafe 四模式。无新 Command。未 commit |
| UIC-44 | 文件名 / 镜头条标题 / 导出记录空槽 / 示例按钮（B-05/06/11/12） | 已完成 | 剧本工具条只显示 `cafe.md`；镜头条格底「1.1 过肩」+ 三点；审片无记录不占 132px；资源库有内容时无「安装示例」按钮。Catch2 **215 / 1327**。B-11 需同步跳过 `WorkspacePanel` 占位 Begin。无新 Command。未 commit |
| UIC-45 | 资源库默认网格（B-07） | 已完成 | `AppState.libraryViewMode` / 快照默认 `"grid"`。置景/掌机资源库首启为缩略图格。Catch2 **215 / 1327**。无新 Command。未 commit |
| UIC-46 | 面板像素宽 + 上下限（S-01） | 已完成 | `DockBuilderSetNodeSize`：左 300–420（1280 下限 280）、右 340–480。按窗口名锁定，避免误缩中央。2560 右栏 ≤480；1280 左右 ≥280。Catch2 **215 / 1327**。无新 Command。未 commit |
| UIC-47 | 界面缩放设置（S-03） | 已完成 | 视图「界面缩放」100/125/150/200；`settings.json` `uiScale`；缺键用 `max(窗口, 主显示器) ≥ 2400` 且 DPI≈1 默认 125%。`LoadUiFont` 只 AddFont 14px；`ApplyUiScale` 从基准 style `ScaleAllSizes(dpi×ui)` 并改 `FontSizeBase`。首启日志 `Applied UI scale 1.25`。Catch2 **218 / 1346**。截图 `uic47-{1280,1920,2560}-{script,set,shoot,review}.png`。无新 Command。未 commit |
| UIC-48 | 格尺寸 / 网格线 / 栏高随缩放（S-04/05/08） | 已完成 | 设计像素 × `UiScale()`：顶栏 40、状态栏 28、镜头条 132（≥1920 格 208×117）。2560 125% 格约 260×146。网格主线 `0x6a6a72`，线宽随缩放（三角带）。Catch2 **218 / 1346**。截图 `uic48-{1280,1920,2560}-{script,set,shoot,review}.png`。无新 Command。未 commit |
| UIC-49 | 设置对话框四页；AI 块移出检查器（P-01） | 已完成 | `编辑 → 设置…` / `Ctrl+,`；760×540×缩放；左列四页常规 / 视口 / 导出 / AI（无 Skills、无测试连接）。检查器只留生成图像 / 视频 + 「打开设置」。偏好走 `settings.json`，不进 `.ddproj`。Catch2 **220 / 1369**。截图 `uic49-1920-{shoot,settings-general,viewport,export,ai}.png`。无新 Command。未 commit |
| UIC-50 | AI 字段标签 + 就地报错（B-09/13） | 已完成 | 无密钥：检查器 AI 段橙字 + 「打开设置」。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-51 | 菜单重组 | 已完成 | 文件 ≤9 项（导出子菜单）；示例进帮助；编辑补复制/删除/显隐；帮助「快捷键…」对话框。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-52 | Skill 面孔重排（P-02） | 已放弃 | 并入 UIC-59：Skill 不再出现在资源库，检查器 Skill 面孔整段删除 |
| UIC-53 | 主按钮下拉补镜头包 / 设置 | 已完成 | 掌机「导出镜头 ▾」：1080p / 2K / 镜头包 / 设置…。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-54 | 左栏 tab 化 | 已完成 | `层级###Hierarchy` / `场景###SceneTree` / `资源库###Library` 同节点；该节点不 AutoHideTabBar。`41` 已补节。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-55 | 编剧编辑器最大行宽 | 已完成 | 居中最大行宽 `900×UiScale()`。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-56 | 新镜头自动选中滚动 | 已完成 | `InsertShot` / 导入分镜 `RecordShotSelection`；镜头表/镜头条 `SetScrollHere*`。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-57 | 空状态原因 + 动作 | 已完成 | 资源库在线原因+刷新；编剧无剧本打开/示例。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-58 | AI 测试连接 | 待批准 | 第 3 个 Command |
| UIC-59 | 设置 → Skills 页；资源库过滤 Skill | 已完成 | 第五页装/卸/官方获取/设默认；`defaultSkillId` 走 `settings.json`。资源库过滤 Skill。Catch2 **223 / 1394**。无新 Command。未 commit |
| UIC-60 | 运行入口迁到编剧模式「生成分镜 ▾」 | 已完成 | 剧本面孔「生成分镜」+ 下拉；无 Skill / 无密钥就地橙字 + 打开设置。Catch2 **223 / 1394**。无新 Command。未 commit |

## 契约落地追踪

| 契约 | 定义在 | 状态 |
|------|--------|------|
| `SelectAdjacentShotCommand` | [`43`](43-CONTRACT-DELTA.md) 一 | 已写入 |
| `RevealPathCommand` | [`43`](43-CONTRACT-DELTA.md) 一 | 已写入 |
| `previewTexture` | [`43`](43-CONTRACT-DELTA.md) 二 | 已写入 |
| `ShotHudView` / `scriptSelectedLine*` | [`43`](43-CONTRACT-DELTA.md) 二 | 均已写入（UIC-21 / UIC-22） |
| `SelectionKind::Asset` | [`43`](43-CONTRACT-DELTA.md) 二 | 已写入（FND-17；`IPanel.h`） |
| 样式常量表 / 字号 | [`43`](43-CONTRACT-DELTA.md) 三、四 | 字号与色板已写入 |
| Lucide 子集 TTF | [`43`](43-CONTRACT-DELTA.md) 五 | 已加入 `assets/fonts/lucide-dd.ttf`（ISC，见 `03`） |
| 用户设置文件 | [`43`](43-CONTRACT-DELTA.md) 七 | 已写入 `settings.json`（非 `.ddproj`）；UIC-47 增 `uiScale`；UIC-49 增导出/上次工程；UIC-59 增 `defaultSkillId` |

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

| Q11 功能按钮 vs 设置项分界 | 作用于当前选中对象 = 功能（检查器 / 顶栏）；跨启动且与工程无关 = 设置（`编辑 → 设置…`）；示例进帮助 / 空状态。AI 供应商与 Skill 管理进设置，「运行 Skill」进编剧模式 | Wisdom 2026-09-15 |

## 工作日志

### 2026-09-16：U3 收口（UIC-50～60 / 22 / 26 / 32）

- UIC-50/59/60：设置第五页 Skills；资源库过滤 Skill；编剧检查器「生成分镜」+ 就地橙字。无新 Command。`defaultSkillId` 进 `settings.json`。
- UIC-51/53：文件菜单 ≤9（导出子菜单）；示例进帮助；快捷键对话框；掌机主按钮补镜头包 / 设置。
- UIC-54：左栏三 tab 同节点，`41` 已补节。UIC-55 编剧最大行宽 900×缩放。UIC-56 新镜选中滚动。UIC-57 空状态原因+动作。
- UIC-22 行号栏+高亮；UIC-26 三点状态走枚举。UIC-32 `storyboard.layout: grid`（v1 枚举扩展，兼容 `left-to-right`）。
- Catch2 **223 / 1394**。未拍本轮 1280/1920/2560 四模式截图。未 commit。下一允许行 **UIC-58 待批准**。

### 2026-09-16：UIC-49 设置对话框四页

- `编辑 → 设置…`（`Ctrl+,`）模态：常规 / 视口 / 导出 / AI。检查器 AI 配置块已移除，只留两个生成按钮 + 「打开设置」（跳到 AI 页）。
- 缩放 / 上次工程 / 视口开关走 `UiPreferences`；导出分辨率与透明走既有 Command；AI 走 `SetAiSettingsCommand`（关窗或离页保存；空密钥保留原值）。无第五页、无测试连接。
- Catch2 **220 / 1369**。截图 `build/qa-e2e/uic49-1920-*.png`。未 commit。下一行 UIC-50。

### 2026-09-16：UIC-48 格尺寸 / 网格 / 栏高随缩放

- 镜头条格、资源库格、顶栏 / 状态栏 / 菜单栏、左右栏像素上下限均 × `UiScale()`。≥1920 宽镜头条格 208×117。
- 视口网格主线 `0x6a6a72`，线宽随 `uiScale` 重建三角带；轴线 2×缩放。
- Catch2 **218 / 1346**。2560 125% 底栏约 8 格铺满。未 commit。下一行 UIC-49。

### 2026-09-15：UIC-47 界面缩放

- `uiScale` 走 UIC-34：`UserSettings` / `UiPreferences` / 视图菜单 100/125/150/200。缺键未设置；首启按 `max(窗口宽, 主显示器宽) ≥ 2400` 且 contentScale≈1.0 默认 125% 并落盘。
- `ImGuiGlfwBackend` 保存基准 style；切换时还原再 `ScaleAllSizes`，避免叠乘。ImGui 1.92 动态字号，不重启。
- Catch2 **218 / 1346**。本机日志 `Applied UI scale 1.25 (DPI 1.00, combined 1.25)`。未 commit。下一行 UIC-48。

### 2026-09-15：UIC-46 面板像素宽

- 左右栏改像素上下限，每帧按「检查器 / 层级」窗口名 `SetNodeSize`。Catch2 **215 / 1327**。未 commit。下一行 UIC-47。

### 2026-09-15：UIC-45 资源库默认网格

- `libraryViewMode` 默认 `grid`。Catch2 **215 / 1327**。未 commit。下一行 UIC-46。

### 2026-09-15：UIC-44 四条小改

- B-05 文件名；B-06 镜头条编号/名称/状态（叠在缩略图底条，否则 132px 内被裁）；B-11 审片无记录不 Begin（含 WorkspacePanel 占位）；B-12 示例按钮只在空状态。Catch2 **215 / 1327**。未 commit。下一行 UIC-45。

### 2026-09-15：UIC-43 projectIsEmpty

- 快照新字段由 App 填；UI 删除 `ProjectIsEmpty`。空脚本 snapshot / 占位立方体算空。Catch2 **215 / 1327**。未 commit。下一行 UIC-44。

### 2026-09-15：UIC-42 元数据两列表格

- B-03：标签改左列，输入 `##key`。47 写 56px，「负面提示词」五字加内边距用实测宽。Catch2 **214 / 1319**。未 commit。下一行 UIC-43。

### 2026-09-15：UIC-41 主按钮箭头

- B-02：字面 `▾` 改为 `Icon::ChevronDown`；切镜帮助与 tooltip 的 `↑↓` 改为 `ChevronUp` / `ChevronDown`。`rg "▾" src/UI` 空。1280 / 1920 / 2560 四模式无 `?` 字形。Catch2 **214 / 1319**。未 commit。下一行 UIC-42。
- 顺手看见：审片场次横幅折叠指示仍是 ASCII `v` / `>`（`StoryboardPanel.cpp`），不是 `▾`，本行未改。

### 2026-09-15（晚）：功能 / 设置分界，Skills 进设置

- Wisdom 指出 API 等应在设置、Skills 也应在设置。`47` 增 5.1「功能按钮与设置项的分界」三问 + 归位表；6.5 设置对话框改**五页**（+Skills）；6.3 编剧模式加「分镜」段 `[生成分镜 ▾]`。
- 新任务 UIC-59（Skills 设置页 + 资源库过滤）/ UIC-60（运行入口迁编剧模式）；UIC-52 放弃并入 59。无新 Command。`../foundation-upgrade/32` 「资源库多一个类别 Skills」由 `47` 5.1 改写。
- 仅文档改动。下一行仍是 UIC-41。

### 2026-09-15：UIC-40 场景行名字

- B-01：`Selectable` 不再传负宽。咖啡馆示例层级「立方体 / 桌子 / 杯子」可见，眼睛在行右。Catch2 **214 / 1319**。未 commit。下一行 UIC-41。

### 2026-09-15：全流程实机审查（`47`）

- Wisdom 提供 22 张 2576×1408 实机截图；写 [`47`](47-FULL-FLOW-AUDIT.md)：适配与可读性 8 条（S-01～08）、逐步审查、缺陷 13 条（B-01～13）、位置对照表、目标线框、任务 U3 `UIC-40～58`。
- 关键根因：`Selectable` 负宽（B-01）、字面 `▾` 不在字体（B-02）、`InputText` 右侧标签（B-03）、`ProjectIsEmpty` 把默认立方体算作内容（B-04）、百分比面板 + 固定 14px 在 2560 宽失效（S-01/03）。
- 仅文档改动；`44` 增 U3 波次；本文件增 U3 表；下一步改为 UIC-40。

### 2026-09-15：FND-41/42/43（FOUNDATION）

- 视口 Gizmo / 置景 Undo / 总览 PDF 已写入。下一 UI 行仍是 UIC-22。Catch2 **199 / 1206**。

### 2026-09-15：UIC-21 + FND-40

- HUD 第三段焦距读 `ShotHudView.focalLength35mm`（App 用 Export 公式填充，UI 不重算）。无选中镜头 `nullptr`；无机位省略第三段。Catch2 **191 / 1162**。未拍 1280/1920 四模式截图。下一行 UIC-22。

### 2026-09-15：FND-17 / FND-18（UIC-26 前置）

- 枚举已写入快照；`src/UI` 不再用 `"shot"` / `"未关联"` 做逻辑。UIC-26 图标未开。导出记录按 `shotId` 回点。FOUNDATION 下一行 FND-20。

### 2026-09-14：FND-15（UIC-22 前置）

- Parser 已给镜头行号区间。UIC-22 行高亮未开。

### 2026-09-14：UIC-23

- 层级右键复制 / 显隐 / 删除；眼睛按钮；检查器同样入口。随 FND-13。

### 2026-09-14：UIC-25

- 状态栏中段「加载模型 x/y」；随 FND-11。pending=0 不显示。

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
