# UI-PRO · UI 专业化升级版（入口）

> 本文件夹是 DirectorDesk 的 **UI 专业化升级（代号 UI-PRO）** 工作集：把 P0 Demo 的界面重做成专业创作软件的工作台（对标 Photoshop / Premiere / Blender 的组织方式，不抄它们的功能）。
> 权威关系：`../00`–`../08` 仍是最高约束（依赖方向、模块数锁定、AI 冻结、暂不拆 `Application.cpp`）。
> 本文件夹**只在一件事上覆盖 `../00`**：界面主次与用户操作顺序。产品路径以 [`21-WORKFLOW-V2.md`](21-WORKFLOW-V2.md) 为准，`00` 第二节的 Demo 顺序降级为「历史起点」。
> 批准者 Wisdom。设计依据：Phase 10 基线的本机代码审计。

## 一、这个版本是什么

| 项 | 值 |
|----|----|
| 版本代号 | **UI-PRO**（UI 专业化升级） |
| 基线 | Phase 10 已完成，tag `phase-10-p1` |
| 一句话目标 | 让软件从「五个功能面板拼在一起」变成「一个有主次、有工作区模式、有状态栏的工作台」 |
| 主线对象 | **Shot**（镜头）。剧本生成它、相机服务它、缩略图证明它、导出交付它 |
| 允许改 | UI 布局与信息层级、`Core::Command` 新增值对象、`AppViewState` 新增只读字段、`App` 的 visit 分发 |
| 不允许改 | 模块集合、依赖方向、领域规则所有权、AI 冻结状态、`Application.cpp` 的文件切分 |
| 零新增 | 不新增 CMake 目标，不新增 `src/<Module>/`，不新增第三方依赖 |

## 二、为什么要做（审计一句话结论）

当前界面把 `00` 的线性流程固化成了空间顺序：剧本在右栏 25%、资源库在左下、机位在左上、分镜在中下 30%，用户的眼睛必须绕屏幕一圈；而 `导演台###Workspace` 一栏塞了九种职责，最终交付物（分镜总览）只占屏幕下方三成。完整证据链见 [`20-UI-PRO-SCOPE.md`](20-UI-PRO-SCOPE.md) 第二节。

## 三、阅读顺序

每轮 vibe coding **必读前四项**：

1. 本文件（版本身份与权威关系）
2. [`26-UI-PRO-STATUS.md`](26-UI-PRO-STATUS.md) — 现在做到哪、这轮该做哪一行
3. [`22-AREA-CONTRACT.md`](22-AREA-CONTRACT.md) — 你要动的区域 ID 的契约
4. [`25-DO-NOT.md`](25-DO-NOT.md) — 停手清单与不可破坏的硬约束

按需读：

- [`20-UI-PRO-SCOPE.md`](20-UI-PRO-SCOPE.md) — 本版目标、审计证据、范围与红线
- [`21-WORKFLOW-V2.md`](21-WORKFLOW-V2.md) — 新主路径 10 步、四个工作区模式、统一选择模型
- [`23-CONTRACT-DELTA.md`](23-CONTRACT-DELTA.md) — 新增 Command / 快照字段 / 窗口 ID 的确切签名
- [`24-LANDING-CHECKLIST.md`](24-LANDING-CHECKLIST.md) — 三波任务表，每行是一次可独立回退的改动

## 四、区域 ID 索引（稳定契约）

区域 ID 是本版最重要的约定：**它让「只改 CENTER_STAGE」成为一句可执行的指令。** ID 一旦写下不再改名。

| 区域 ID | 中文名 | 宿主窗口（ImGui 稳定 ID） | 契约 |
|---------|--------|---------------------------|------|
| `MENU_BAR` | 菜单栏 | `DirectorDeskDockSpace` 的 `BeginMenuBar` | [`22`](22-AREA-CONTRACT.md) |
| `TOOL_STRIP` | 模式与进度条 | 新：主视口上沿侧边条 | [`22`](22-AREA-CONTRACT.md) |
| `LEFT_HIERARCHY` | 镜头表 + 场景对象 | 新：`层级###Hierarchy` | [`22`](22-AREA-CONTRACT.md) |
| `LEFT_LIBRARY` | 资源库 | 现有：`资源库###Library` | [`22`](22-AREA-CONTRACT.md) |
| `CENTER_STAGE` | 中央舞台（随模式换宿主） | `视口###Viewport` / `分镜###Storyboard` / `剧本###Script` | [`22`](22-AREA-CONTRACT.md) |
| `RIGHT_INSPECTOR` | 唯一检查器 | 新：`检查器###Inspector` | [`22`](22-AREA-CONTRACT.md) |
| `BOTTOM_STRIP` | 镜头条 / 导出记录 | 新：`镜头条###ShotStrip` | [`22`](22-AREA-CONTRACT.md) |
| `STATUS_BAR` | 状态栏 | 新：主视口下沿侧边条 | [`22`](22-AREA-CONTRACT.md) |

## 五、工作方式（与 `../06`/`../07` 一致，不重复）

- 落点顺序不变：Panel → `Core::Command` → `Application.cpp` 的 visit → App 组装快照。见 `../07` 第四节。
- 一轮只做 [`24-LANDING-CHECKLIST.md`](24-LANDING-CHECKLIST.md) 里的一行或一组；收工必须更新 [`26-UI-PRO-STATUS.md`](26-UI-PRO-STATUS.md) 与 `../03-CURRENT-STATUS.md`。
- 与本文件夹冲突时：控制面（依赖、模块、线程、第三方隔离）以 `../01`/`../07` 为准；界面主次与操作顺序以本文件夹为准。
- 未经 Wisdom 明确要求，不得 commit、tag、push。
