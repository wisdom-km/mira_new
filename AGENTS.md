# Agent 入口

你在 DirectorDesk（仓库名 `mira_new`）里改代码或答架构问题时，以仓库内 `docs/dev-map/` 为唯一权威。聊天记录不覆盖这些文件。

按顺序阅读：

1. [00 愿景与红线](docs/dev-map/00-VISION-AND-CONSTRAINTS.md)
2. [01 架构地图](docs/dev-map/01-ARCHITECTURE-MAP.md)
3. [02 路线图](docs/dev-map/02-ROADMAP.md)
4. [03 当前状态](docs/dev-map/03-CURRENT-STATUS.md)
5. [05 编码规范](docs/dev-map/05-CODING-STANDARDS.md)
6. [07 迭代与落点](docs/dev-map/07-ITERATION-AND-LANDING.md)
7. 当前工作涉及的 [modules](docs/dev-map/modules/)

当前进行中的版本：**FOUNDATION（地基、镜头包与 Skills 融合）**。产品 tag **`v0.1.3`** 已含 F0 与 UI-CLARITY。所有代码工作先读 [foundation-upgrade/README.md](docs/dev-map/foundation-upgrade/README.md)，从 [36-FOUNDATION-STATUS.md](docs/dev-map/foundation-upgrade/36-FOUNDATION-STATUS.md) 找「下一个允许执行的工作」，按 [34-LANDING-CHECKLIST.md](docs/dev-map/foundation-upgrade/34-LANDING-CHECKLIST.md) 的任务 ID（`FND-xx`）落点，动手前扫一遍 [35-DO-NOT.md](docs/dev-map/foundation-upgrade/35-DO-NOT.md)。新 Command 与新文件格式以 [33-CONTRACT-DELTA.md](docs/dev-map/foundation-upgrade/33-CONTRACT-DELTA.md) 为准。下一代码行是 **FND-16**。

上一版本 **UI-PRO** 已收口。凡涉及界面布局、面板主次、工作区模式、检查器、状态栏、镜头条，仍按 [ui-pro-upgrade/README.md](docs/dev-map/ui-pro-upgrade/README.md) 的区域 ID 落点。界面主次以该文件夹为准，控制面仍以 01/07 为准。

界面重设计 **UI-CLARITY**：入口 [ui-clarity/README.md](docs/dev-map/ui-clarity/README.md)。进度以 [46-UI-CLARITY-STATUS.md](docs/dev-map/ui-clarity/46-UI-CLARITY-STATUS.md) 为准，按 [44-LANDING-CHECKLIST.md](docs/dev-map/ui-clarity/44-LANDING-CHECKLIST.md) 的 `UIC-xx` 落点，动手前扫 [45-DO-NOT.md](docs/dev-map/ui-clarity/45-DO-NOT.md)。区域内容以 [41-AREA-CONTRACT-V2.md](docs/dev-map/ui-clarity/41-AREA-CONTRACT-V2.md) 为准，新 Command / 样式以 [43-CONTRACT-DELTA.md](docs/dev-map/ui-clarity/43-CONTRACT-DELTA.md) 为准。调研报告仍是 [40](docs/dev-map/ui-clarity/40-UI-AUDIT-AND-REDESIGN.md)。**U0 已完成**（UIC-01～17），**UIC-20 / 27 / 30 / 31 / 33 / 34** 已随 `v0.1.3` 入库。其余 U1 等对应 `FND-xx`。

工作方式（不复制项目事实）：[06 Cursor Prompt](docs/dev-map/06-CURSOR-WORKING-PROMPT.md)。

原理（为什么要接缝、vibe coding 哪些能做哪些不能）：[08 接缝与 Vibe Coding](docs/dev-map/08-SEAMS-AND-VIBE-CODING.md)。任务若只是修局部 bug，读完 00–05 与 07 即可；改架构、加删功能或讨论模块化时必读 08。

可缩放模块图：[GitHub Pages](https://wisdom-km.github.io/mira_new/architecture.html)。

约束摘要：

- P0 完成后只把功能填进现有名词；不得为小功能新建 CMake 模块
- UI 只发 Command、只读展示快照
- 第三方类型不进公共业务头
- AI 模块冻结，不得接线真实服务、不得在软件内运行 Skill 或子进程
- `src/App/Application.cpp` 允许在 App 目标内分文件，不得新建模块（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）
- 未经用户明确要求，不得 commit、tag、push
