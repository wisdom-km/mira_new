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

当前进行中的版本：**UI-PRO（UI 专业化升级）**。凡涉及界面布局、面板主次、工作区模式、检查器、状态栏、镜头条的工作，追加读 [ui-pro-upgrade/README.md](docs/dev-map/ui-pro-upgrade/README.md)，并按其中的区域 ID 与任务 ID 落点。界面主次与操作顺序以该文件夹为准，控制面仍以 01/07 为准。

工作方式（不复制项目事实）：[06 Cursor Prompt](docs/dev-map/06-CURSOR-WORKING-PROMPT.md)。

原理（为什么要接缝、vibe coding 哪些能做哪些不能）：[08 接缝与 Vibe Coding](docs/dev-map/08-SEAMS-AND-VIBE-CODING.md)。任务若只是修局部 bug，读完 00–05 与 07 即可；改架构、加删功能或讨论模块化时必读 08。

可缩放模块图：[GitHub Pages](https://wisdom-km.github.io/mira_new/architecture.html)。

约束摘要：

- P0 完成后只把功能填进现有名词；不得为小功能新建 CMake 模块
- UI 只发 Command、只读展示快照
- 第三方类型不进公共业务头
- AI 模块冻结，不得接线真实服务
- 暂不拆 `src/App/Application.cpp`
- 未经用户明确要求，不得 commit、tag、push
