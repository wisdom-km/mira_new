# 44 - 落点清单（LANDING CHECKLIST）

> 每一行可独立完成、独立回退。任务 ID 稳定，可直接说「做 UIC-08」。
> 落点顺序：Panel →（若需要）`Core::Command` → App visit → 快照 → 测试 → 更新 [`46`](46-UI-CLARITY-STATUS.md) 与 `../03`。
> 进度只在 [`46`](46-UI-CLARITY-STATUS.md) 记。契约在 [`41`](41-AREA-CONTRACT-V2.md) / [`43`](43-CONTRACT-DELTA.md)。
> 证据编号见 [`40`](40-UI-AUDIT-AND-REDESIGN.md) 第三节。

## 波次

| 波次 | 进入条件 | 完成条件 | 契约 |
|------|----------|----------|------|
| **U0 · 快赢** | Wisdom 过完本设计集 | UIC-01～17 完成；**1280×800** 与 1920×1080 四模式无文字截断（1024 不作验收） | 无新 Command、无新快照字段。允许给已有 `selectionKind` 加字符串 `"asset"` |
| **U1 · 快照** | 该行依赖的 `FND-xx` 已完成（F1/F2 已打相应 tag 或 `36` 标已完成） | UIC-20～27 | 只加字段 / 接 FOUNDATION 字段 / 复用既有 Select 记录 |
| **U2 · Command 与资源** | U1 完成；F2 之后才做 UIC-32 | UIC-30～34 | 2 个新 Command、图标字体、可选用户设置、分镜 grid |

U0 **不依赖** `v0.1.3` / F1，可与 FOUNDATION 门禁并行。F1 本身仍须 Wisdom 打 `v0.1.3` 后才开。

## U0 · 快赢（纯 UI + 样式）

推荐顺序（先全局样式，避免每步重拍截图）：**UIC-08 → 09 → 07 → 06 → 10 → 04 → 05 → 17 → 01 → 02 → 03 → 11 → 12 → 16 → 13 → 14 → 15**。

| ID | 区域 | 改动 | 落点 | 依赖 | 完成标准 |
|----|------|------|------|------|----------|
| UIC-08 | 全局 | 默认 body=14 + DPI；动态字号 `PushFont`（Q2） | `ImGuiGlfwBackend.cpp` `LoadUiFont`、`src/UI/UiFonts.h`、各面板 | — | 100% DPI 下默认 14；`ScaleAllSizes`；不预载多尺寸 atlas |
| UIC-09 | 全局 | 中性色板、单强调橙、选中底=accent 淡色（Q4） | `ApplyDirectorDeskStyle` | — | 无 `0x31547d` 选中蓝 |
| UIC-07 | 全局 | `AutoHideTabBar`；段头透明 + caption | dock flags、样式、层级段头 | — | 单窗口节点无 tab 条；无蓝色大横幅 |
| UIC-06 | `STATUS_BAR` | 高 28px；删路径与 `VIEW`；右侧 `N 镜 · M 就绪` | `WorkspacePanel.cpp` 810–842 | — | 1280×800 底边文字不被裁 |
| UIC-10 | `TOOL_STRIP` | segmented 模式 + 中央镜头名 + 右侧唯一主按钮；`1`–`4` 切模式 | 761–808 | — | 当前模式一眼可辨；「N 镜」不在顶栏 |
| UIC-04 | `LEFT_HIERARCHY` | 两段固定；编剧折叠场景；审片默认最小宽度，视图菜单「折叠左栏」 | `ApplyDockLayout`、988–1059 | — | 四模式左栏位置不漂移；U0 **不做** 48px 图标条 / 浮层（U2 + UIC-33） |
| UIC-05 | `BOTTOM_STRIP` | 镜头条改为 Down SideBar 132px；编剧不 `Begin`（Q7）；审片标题「导出记录」 | `StoryboardPanel.cpp`、`WorkspacePanel.cpp`；`ApplyDockLayout` 不再切 `dockBottom` | — | 拉伸窗口不裁半张缩略图；编剧无空槽 |
| UIC-17 | 空工程 | 左栏 / 底栏隐藏，只留开始板 | 层级 / 镜头条 SideBar / dock | — | 空工程旁不再挂三段「空」 |
| UIC-01 | `CENTER_STAGE` | 视口按导出比例 letterbox；RT = 取景框；视图菜单可关（Q1） | `WorkspacePanel.cpp` 视口分支（约 931–944） | — | 取景框内比例与 1080p/2K 导出一致；关锁后回到铺满；`InvisibleButton` + 清零滚轮 + 2px 仍在。`03`「格网视口有、导出无」不变，只加开关 |
| UIC-02 | 视口 / 导出 | 3D 与非透明导出背景 `0x2b2b2e`；网格/轴降饱和（Q5） | `BgfxRenderer.cpp` 清屏与网格 | — | 透明导出 alpha 仍 0；已缓存蓝底缩略图等到标 stale（记 `46`） |
| UIC-03 | `CENTER_STAGE` | HUD 改为「镜头 · 相机」；删 `LIVE` / 像素 | `WorkspacePanel.cpp` 910–929 | — | `rg "LIVE" src/UI` 空 |
| UIC-11 | `RIGHT_INSPECTOR` | 面孔互斥；`selectionKind="asset"`；灯光搬到场景面孔（Q8）；导出设置只在导出面孔 | `WorkspacePanel.cpp` 1061–1092、`DrawShotInspector`；`Application.cpp` 的 `SelectLibraryAsset` / `SelectShot` / `SelectNode` / `SelectCamera` | — | 资产块不再追加在其他面孔下；镜头面孔无灯光 |
| UIC-12 | `RIGHT_INSPECTOR` / 层级 | 分辨率 Combo；机位 3×2；段头统一 `+` | 检查器 + 层级段头 | — | 1280×800 无截断 |
| UIC-16 | `RIGHT_INSPECTOR` | 上手三步纵排；快捷键两列表格 | `DrawOnboarding` | — | 1280×800 无截断 |
| UIC-13 | `LEFT_LIBRARY` | 一排 chrome + 溢出菜单 + chips | `LibraryPanel.cpp` 84–174 | — | 搜索框可用；20% 宽至少两列色块 |
| UIC-14 | `CENTER_STAGE` | 分镜短期：藏根卡与连线、场次横幅、缩略图 16:9（Q6 短期） | `StoryboardPanel.cpp` 310–366 | — | 仍用现有 `x/y/w/h`，不改 Storyboard 契约 |
| UIC-15 | 全部 UI | [`40`](40-UI-AUDIT-AND-REDESIGN.md) 第七节术语表全量替换 | `src/UI/*.cpp` | — | `rg "LIVE\|BEAT MAP\|VIEW %u\|已成镜\|重渲" src/UI` 为空（显示用途例外须在 `46` 注明） |

## U1 · 需要快照字段（无新 Command）

| ID | 区域 | 改动 | 新字段 / 引用 | 依赖 | 完成标准 |
|----|------|------|---------------|------|----------|
| UIC-20 | `LEFT_LIBRARY` | 缩略图网格 | `LibraryAssetView.previewTexture` | App 主线程加载 sidecar / 官方 preview | `0xFFFF` 画格式色块；可见项有图 |
| UIC-21 | `CENTER_STAGE` | HUD 第三段焦距 | `ShotHudView` | **FND-21**（公式已在 FOUNDATION `33` 五） | 与镜头包 JSON `focalLength35mmEquivalent` 同值 |
| UIC-22 | `CENTER_STAGE` | 剧本当前镜头行高亮 + 行号栏 | `scriptSelectedLineStart/End` | **FND-15** | 选镜头后对应 `###` 区间底色 6% accent |
| UIC-23 | `LEFT_HIERARCHY` / 检查器 | 眼睛 / 复制 / 删除 | `SceneNodeView.visible` | **FND-13** | 隐藏节点不进视口与导出 |
| UIC-24 | `RIGHT_INSPECTOR` | 镜头面孔元数据分节 | `selectedShotMeta` | **FND-20** | 改一键只动剧本文本一行 |
| UIC-25 | `STATUS_BAR` | `加载模型 x/y` | `sceneLoadPending/Total` | **FND-11** | 打开示例期间视口持续刷帧 |
| UIC-26 | 多处 | 三点状态用枚举画图标 | `SelectionKind`（含 `Asset`）/ `CardKind` | **FND-17** | 面板不再用 `"未关联"` 判断逻辑 |
| UIC-27 | App | 打开工程 / 剧本后若无选中镜头，自动选第一镜 | `Application.cpp` 的 `OpenProjectFromPath` / `LoadScriptFromPath` 末尾，复用 `SelectShotCommand` 同一段记录逻辑 | — | `40` 第十节「打开示例 → 看到第一镜」可 2 步 |

## U2 · 新 Command / 资源 / 长期契约

| ID | 区域 | 改动 | 契约 | 依赖 | 完成标准 |
|----|------|------|------|------|----------|
| UIC-30 | `TOOL_STRIP` | `[` `]` / `↑↓` 切镜 | `SelectAdjacentShotCommand` | — | 30 镜可键盘走完；回绕；Dispatch 测试 |
| UIC-31 | `STATUS_BAR` / 导出记录 | 「打开」「文件夹」 | `RevealPathCommand` | Platform 实现 | 导出后 1 次打开目录；`://` 被拒；Dispatch 测试 |
| UIC-32 | `CENTER_STAGE` / Export | 分镜 `grid` 布局 | 改 `modules/storyboard-canvas.md` 与 `project-file.md` 的 `storyboard.layout` | **F2 之后**（Q6） | 每场横幅、镜头按列换行；默认 `grid`；总览 PNG 同步 |
| UIC-33 | 全局 | Lucide 子集 TTF + MergeMode；审片 48px 图标条与窄窗浮层 | 资源文件 + `03` ISC 记录（Q3）；接 UIC-04 | — | 约 40 字形；无新 CMake 目标 |
| UIC-34 | 偏好 | 背景 / 辅助线 / 左栏折叠写入用户目录 | 用户设置文件，**不进** `.ddproj`（Q10） | UIC-01/02/04 | 重启后开关还在 |

## U3 · 专业化与适配（2026-09-15 实机审查，定义在 [`47`](47-FULL-FLOW-AUDIT.md) 第七节）

| ID | 类型 | 改动 | 落点 | 依赖 | 完成标准 |
|----|------|------|------|------|----------|
| UIC-40 | Bug | 场景行名字（`Selectable` 负宽） | `WorkspacePanel.cpp` 场景段 | — | 对象名可见，眼睛在右 |
| UIC-41 | Bug | 字面 `▾` → `Icon::ChevronDown` | 主按钮与其他字面 `▾` | — | `rg "▾" src/UI` 空 |
| UIC-42 | Bug | 元数据两列表格标签 | 元数据段 | — | 五个标签完整可见 |
| UIC-43 | Bug | `projectIsEmpty` 由 App 给；上手三步勾选正确 | `ViewStateBuilder.cpp` + 删 UI 侧 `ProjectIsEmpty` | 新字段 `bool projectIsEmpty` | 无工程启动出开始板 |
| UIC-44 | Bug | 剧本只显示文件名；镜头条格加编号 / 名称 / 状态；无记录不画导出记录；示例按钮只在空状态 | `ScriptPanel` / `StoryboardPanel` / `LibraryPanel` | — | 四条各自可见 |
| UIC-45 | Bug | 资源库默认网格 | 默认 `libraryViewMode` | — | 首启即网格 |
| UIC-46 | 适配 | 面板像素宽 + 上下限（左 300–420，右 340–480，× 缩放） | `ApplyDockLayout` | — | 2560 宽右栏 ≤ 480px；1280 宽 ≥ 280px |
| UIC-47 | 适配 | 界面缩放设置 100/125/150/200；首启 ≥2400 宽且 contentScale 1.0 默认 125% | `UiFonts.h` / `settings.json` `uiScale` | UIC-34 | 切换不重启生效 |
| UIC-48 | 适配 | 格尺寸 / 网格线宽 / 栏高随缩放 | 各面板常量 × `UiScale()` | UIC-47 | 1440p 125% 镜头条格 ≥ 208×117 |
| UIC-49 | 位置 | 设置对话框前四页（常规 / 视口 / 导出 / AI）；AI 块移出检查器 | `DrawSettingsModal`；编辑菜单 `Ctrl+,` | UIC-47；FND-53 | 检查器只留两个生成按钮 + 状态 + 「打开设置」 |
| UIC-50 | 位置 | AI 字段标签；无密钥就地报错 | 设置 AI 页；检查器 AI 段 | UIC-49 | 按钮下橙字 + 「打开设置」 |
| UIC-51 | 位置 | 菜单重组（导出子菜单 / 示例进帮助 / 编辑补齐 / 视图补齐 / 快捷键对话框） | 菜单段 | — | 文件菜单 ≤ 9 项；帮助无纯文本行 |
| ~~UIC-52~~ | 位置 | 已放弃，并入 UIC-59 | — | — | — |
| UIC-59 | 位置 | 设置 → **Skills 页**（装 / 卸 / 从官方获取 / 设默认 / 详情 / 空状态）；资源库过滤掉 `kind == "skill"`；删检查器 Skill 面孔 | `DrawSettingsModal` 第五页；`LibraryPanel` 过滤；`UiPreferences.defaultSkillId` | UIC-49 | 资源库无 Skill 卡；无新 Command（见 `47` 5.1 / 6.5） |
| UIC-60 | 位置 | 运行入口迁到编剧模式：剧本面孔「分镜」段 `[生成分镜 ▾]` + 「从分镜 JSON 导入…」+ 就地状态 / 「打开设置」；菜单「导入模型或 Skill…」→「导入模型…」 | `WorkspacePanel` 剧本面孔 | UIC-59；UIC-56 | 不选资产也能一键生成分镜（见 `47` 6.3） |
| UIC-53 | 位置 | 主按钮下拉补「镜头包」「设置…」 | 顶栏 | FND-21 | 与 `41` 四项一致 |
| UIC-54 | 布局 | 左栏 tab 化：`镜头表` / `场景###SceneTree` / `资源库` | `ApplyDockLayout`、层级拆两窗口 | UIC-46 | 置景不裁场景段；`41` 补节 |
| UIC-55 | 布局 | 编剧编辑器居中最大行宽 900 × 缩放 | `ScriptPanel.cpp` | UIC-47 | 1440p 一行 ≤ 65 汉字 |
| UIC-56 | 反馈 | 新导入 / 新建镜头自动选中并滚到可视区 | Dispatch 末尾 + 面板 `SetScrollHere*` | — | 运行 Skill 后新镜头高亮 |
| UIC-57 | 空状态 | 资源库在线 / 编剧无剧本 空状态一句原因 + 一个动作 | `LibraryPanel` / `ScriptPanel` | — | — |
| UIC-58 | 待批准 | AI「测试连接」 | 需第 3 个 Command | Wisdom | — |

功能按钮 / 设置项 / 示例的归位规则见 [`47`](47-FULL-FLOW-AUDIT.md) 5.1：作用于当前选中对象的是功能（检查器 / 顶栏 / 右键），跨启动且与工程无关的是设置（`编辑 → 设置…`），一次性示例进帮助 / 空状态。UIC-32（审片 grid）门禁已开（F2 完成），排在 UIC-55 之后。顺序建议见 [`47`](47-FULL-FLOW-AUDIT.md) 第七节末。

## 每个任务的 Definition of Done

除 `../05` 第十一节：

1. 完成标准列可复现，写入 [`46`](46-UI-CLARITY-STATUS.md)。
2. 未触碰 [`45`](45-DO-NOT.md) 与 `../ui-pro-upgrade/25` 第三节、`../foundation-upgrade/35` 第三节。
3. 现有 Catch2 全绿。U2 新 Command 在 FND-10 之后必须有 Dispatch 测试。
4. **U0 起**每次收工：本机 1280×800 与 1920×1080 各看四模式，无文字截断（Wisdom 或执行模型截图）。**U3 起**加 2560×1440 · 界面缩放 125% 一档：右栏空白 ≤ 40%、取景框暗幕单边 ≤ 80px。
5. 收工同步 [`46`](46-UI-CLARITY-STATUS.md) 与 `../03`。
