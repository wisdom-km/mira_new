# 45 - 停手清单（DO NOT）

> 每轮动手前扫一遍。与 `../07` 第六节、`../ui-pro-upgrade/25`、`../foundation-upgrade/35` **叠加生效**。
> 第三节完整沿用 UI-PRO `25` 第三节，一条不删。

## 一、现在不要做（范围外）

| 不要做 | 依据 |
|--------|------|
| 接线供应商 SDK、把密钥写入日志、MCP 入站 | `../07`；F5 只允许 `IHttpClient` 出站 |
| 新建 `src/<Module>/` 或 CMake 目标 | `../07` 模块数锁定 |
| 第 3 个本版新 Command | Wisdom Q9：上限 `SelectAdjacentShot` + `RevealPath` |
| 未打 `v0.1.3` 就开 FOUNDATION F1 | `../foundation-upgrade/34` / `36` |
| 依赖的 `FND-xx` 未完成就做对应 U1/U2 行 | [`44`](44-LANDING-CHECKLIST.md) |
| UIC-32 在 F2 之前 | Q6 |
| 时间轴 / 自由连线 | `../00`。ImGuizmo / 置景 Undo 由 FOUNDATION FND-41/42 落地（Wisdom 2026-09-15 批准） |
| 升 `.ddproj` 格式版本 | FOUNDATION 保持 1；UIC-32 若改 `storyboard.layout` 枚举须先改 `modules/` 并由 Wisdom 再看一眼 |
| 为图标引入 vcpkg 或运行时下载字体 | Q3：仓库内子集 TTF |
| 把取景框 / 网格偏好写进工程文件 | Q10：用户目录，且排在 U2 末 |

## 二、不要保留的 Demo 习惯

| 习惯 | 改成 |
|------|------|
| 同一意图三个同权按钮 | 顶栏一个主按钮 + 菜单快捷 |
| 检查器两张面孔叠在一起 | 互斥（UIC-11） |
| `LIVE` / 像素 / 全路径当主文案 | 术语表（[`40`](40-UI-AUDIT-AND-REDESIGN.md) 七） |
| 底栏用百分比挤出半张缩略图 | 固定 132px 或隐藏 |
| 用 `"未关联"` 字符串判断业务 | FND-17 / UIC-26 枚举 |
| 面板里算 FOV / 行号 / 导出比例 | App 送快照；取景框只做几何 letterbox |

## 三、改了就会安静弄坏东西（硬约束）

完整沿用 [`../ui-pro-upgrade/25-DO-NOT.md`](../ui-pro-upgrade/25-DO-NOT.md) 第三节，并叠加 `../foundation-upgrade/35` 第三节。摘录如下，**落地时不得改这些行为**：

| 约束 | 弄坏什么 |
|------|----------|
| 窗口稳定 ID：`###Viewport` `###Workspace` `###Script` `###Library` `###Storyboard` `###Hierarchy` `###Inspector` `###ShotStrip` | 丢 `imgui.ini` |
| 帧序：`RenderScene` → 缩略图 `RequestReadback` → `imgui.BeginFrame` → `Draw` → `Submit` → `EndFrame` | 撕帧 |
| 纹理 `std::uint16_t`，`0xFFFF` 无效 | 画出垃圾 |
| 拖放载荷名 `DD_ASSET_ID` | 拖资源进场景断开 |
| 视口 `InvisibleButton` 独占并清零滚轮；`NoScrollWithMouse` | 滚轮闪屏回归 |
| 视口尺寸变化 < 2px 不上报 `ViewportResizeCommand` | 每帧重建 RT |
| 缩略图异步回读 | 交换链闪屏 |
| UI 只读 `AppViewState`、只发 `Core::Command` | 架构漂移 |
| 后台线程不碰业务 / UI / Renderer | 数据竞争 |
| `Storyboard` 不 include `Script` / `Link` / `Camera` / `Renderer` | 依赖环 |

本版额外：

| 约束 | 弄坏什么 |
|------|----------|
| 取景框开启时 `ViewportResize` 的宽高是 **frame** 不是 available | 导出比例再次漂移 |
| `InvisibleButton` 仍覆盖整个 available（含暗幕） | letterbox 外无法轨道或误把滚轮给 dock |
| `previewTexture` 与 `thumbTexture` 同一套 `0xFFFF` 约定 | 资源库画出野纹理 |
| `RevealPathCommand` 拒绝 `://` | 打开远程地址 |
| 镜头条与状态栏都是 `BeginViewportSideBar(Down)`，不是 dock 比例切分 | 窗口一拉伸又裁半张缩略图 |
| Down SideBar 从底边往上占位：先建状态栏（28px）再建镜头条（132px），视觉上镜头条在上、状态栏贴底 | 两条栏叠反或状态栏被顶走 |

## 四、本版明确改写的旧决策

仅以下条目可以和 UI-PRO `25` / `22` 第六节旧文字不一致（Wisdom 2026-09-13，Q1–Q10）：

| 旧文字 | 位置 | 本版改为 |
|--------|------|----------|
| 「本版不引入第二套颜色 / 不换字体方案」 | `../ui-pro-upgrade/25` 一、`22` 六 | 中性色板 + body 14 + DPI + Lucide 子集（Q2/Q3/Q4） |
| 左栏随模式增删整段、审片隐藏左栏 | `22` 第四节 | 两段固定；审片默认最小宽度 + 视图菜单（图标条 / 浮层等 U2） |
| 编剧底栏留解析摘要槽 | `22` 第四节 | 编剧隐藏底栏（Q7） |
| 灯光在镜头面孔「画面」分节 | `22` `RIGHT_INSPECTOR` | 搬到场景面孔（Q8） |
| 3D / 导出背景 `0x3a4a62` | `BgfxRenderer` / 既有视觉 | 中性灰 `0x2b2b2e`（Q5） |
| UI-PRO Command 上限 4 | `25` 一 | 本版**另外**允许 2 个（Q9），不回溯改 UI-PRO 已落地的 4 个 |

`../03` 决策表「地面格网：视口绘制、导出不含」**不是**本版改写。UIC-01 只给视口加可关的辅助线开关，导出仍不含格网。

改旧文档句子时，在该行后加 `（UI-CLARITY 版改写，见 ui-clarity/45 第四节）`，不要静默替换。

## 五、遇到冲突怎么办

1. 控制面（依赖、模块、线程、AI、第三方隔离）以 `../01` / `../07` 为准。
2. 区域 ID 与窗口 ID 以 `../ui-pro-upgrade/22` 为准。
3. 本版区域内内容与视觉以 [`41`](41-AREA-CONTRACT-V2.md) / [`43`](43-CONTRACT-DELTA.md) 为准。
4. FOUNDATION 的 Command / 格式以 `../foundation-upgrade/33` 为准；本文件不重新定义它们。
5. 代码与文档冲突：停工，Wisdom 决定改哪边。
