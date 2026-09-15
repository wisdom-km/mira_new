# UI-CLARITY · 界面重设计（入口）

> 本文件夹是 DirectorDesk 的第二次界面升级，代号 **UI-CLARITY**。起因：Wisdom 2026-09-13 反馈「现在的 UI 布局很乱、不够直观」。UI-PRO 把职责拆到八个区域；本版改每个区域**里面**的信息层级、控件、文案、比例与视觉系统。
> 当前阶段：**U0～U3 允许行已完成**（仅 UIC-58 待批准）。进度以 [`46`](46-UI-CLARITY-STATUS.md) 为准。
> 权威关系：`../00`–`../08` 与 `../foundation-upgrade/` 的控制面不变。`../ui-pro-upgrade/` 的八个区域 ID **继续有效**，不再改名、不增删区域。
> 批准者 Wisdom。Q1–Q10 已拍板（2026-09-13），记在 [`46`](46-UI-CLARITY-STATUS.md)。

## 一、这个版本是什么

| 项 | 值 |
|----|----|
| 版本代号 | **UI-CLARITY** |
| 基线 | `v0.1.2` + FOUNDATION F0 |
| 一句话目标 | 视口所见即所得；镜头为轴；稳定骨架；说导演的话 |
| 允许改 | 区域内布局与文案、样式 / 字体 / 图标资源、2 个新 Command、若干只读快照字段 |
| 不允许改 | 模块集合、区域 ID、窗口 `###` 后缀、`.ddproj` 版本 |

## 二、阅读顺序

每轮 vibe coding **必读前四项**：

1. 本文件
2. [`46-UI-CLARITY-STATUS.md`](46-UI-CLARITY-STATUS.md) — 现在做到哪、这轮该做哪一行
3. [`41-AREA-CONTRACT-V2.md`](41-AREA-CONTRACT-V2.md) — 要动的区域 ID 的新内容
4. [`45-DO-NOT.md`](45-DO-NOT.md) — 停手清单与硬约束

按需读：

- [`40-UI-AUDIT-AND-REDESIGN.md`](40-UI-AUDIT-AND-REDESIGN.md) — 第一次审计证据、原则、Q1–Q10 原文
- [`47-FULL-FLOW-AUDIT.md`](47-FULL-FLOW-AUDIT.md) — **第二次审计**（2026-09-15，2560×1440 实机全流程）：适配 / 可读性、13 条缺陷、「专业软件里它该在哪」对照、**5.1 功能按钮 vs 设置项分界**、目标线框（含五页设置对话框）、U3 任务 `UIC-40～60`
- [`43-CONTRACT-DELTA.md`](43-CONTRACT-DELTA.md) — 2 个 Command、快照、色板、字号、图标
- [`44-LANDING-CHECKLIST.md`](44-LANDING-CHECKLIST.md) — U0 / U1 / U2 / **U3** 任务表

## 三、区域 ID（不改名）

| 区域 ID | 契约 |
|---------|------|
| `MENU_BAR` `TOOL_STRIP` `LEFT_HIERARCHY` `LEFT_LIBRARY` `CENTER_STAGE` `RIGHT_INSPECTOR` `BOTTOM_STRIP` `STATUS_BAR` | [`41`](41-AREA-CONTRACT-V2.md) |

宿主窗口 ID 仍以 `../ui-pro-upgrade/22` 与 `23` 为准。

## 四、与其他版本的关系

| 版本 | 管什么 | 本版怎么对待 |
|------|--------|--------------|
| UI-PRO（已收口） | 八个区域 ID、四模式、统一选择 | 全部沿用 |
| FOUNDATION（进行中） | F1/F2 快照与 Command | U1/U2 标注 `FND-xx`；不重复定义 |
| UI-CLARITY（本版） | 区域内内容与视觉 | 本文件夹 |

U0 不依赖 `v0.1.3`。F1 仍须打 tag 后才开。

## 五、工作方式

- 一轮只做 [`44`](44-LANDING-CHECKLIST.md) 一行或一组；收工更新 [`46`](46-UI-CLARITY-STATUS.md) 与 `../03`。
- 落点：Panel → Command → visit → 快照。纯视觉落在 `ImGuiGlfwBackend.cpp` / `BgfxRenderer.cpp` / `src/UI/*.cpp`。
- **未经 Wisdom 明确要求，不得 commit、tag、push。**
