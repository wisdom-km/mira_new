# 25 - 停手清单（DO NOT）

> 本版每轮动手前扫一遍。第一节是范围外的功能，第二节是会毁专业度的 Demo 习惯，第三节是改了就会安静弄坏东西的技术约束。
> 与 `../07` 第六节「一票否决」不冲突：那是全项目的，本文件是本版的。

## 一、现在不要做（范围外）

| 不要做 | 依据 |
|--------|------|
| 接线真实 AI（连「AI 生成」按钮都不要画） | `../07` 第二节：AI 模块冻结，不接线、不扩展、不在面板留入口 |
| 新建 CMake 目标或 `src/<Module>/` | `../07` 第二节：模块集合锁定。本版所有改动落在 UI / Core / App 三个已有目标 |
| 拆 `src/App/Application.cpp` | `../03`、`../07`：暂缓。新 visit 分支加在原处，编排层允许厚 |
| Undo / Redo | `../00` 第五节 P0 明确不做 |
| 完整时间轴、相机动画、关键帧 | `../00` 第五节 |
| 自由绘制 / 任意连线画布 | `../00` 第五节：分镜画布只由剧本自动生成结构 |
| ImGuizmo 或任何第三方 UI 组件 | `../03` 已确认决策：变换入口用 `DragFloat3` |
| 改 `.ddproj` 格式 | 要先改 `../modules/project-file.md` 并由 Wisdom 批准；本版 `workspaceModeId` 不持久化 |
| 第二套配色 / 换字体方案 | 调色板已在 `backends/imgui/ImGuiGlfwBackend.cpp` 落地，本版沿用 |
| 为假想需求预留空面板、空模式、空开关 | `../08` 第十节：为变化预留接缝，不为幻想预留房间 |
| 第 5 个新 Command | 本版上限是 [`23`](23-CONTRACT-DELTA.md) 的 4 个；逼出第 5 个先怀疑是搬家问题 |

## 二、不要保留的 Demo 习惯

| 习惯 | 为什么毁专业度 | 改成 |
|------|----------------|------|
| 同一意图三个同权重入口（机位预设现在在菜单、视口工具条、导演台各一套） | 用户学不到正规路径 | `RIGHT_INSPECTOR` 一处主入口 + 菜单一处快捷路径；视口工具条上撤掉 |
| 一个面板承担九种职责 | 每种职责都只能分到一小条，全都做不好 | 按区域 ID 拆开，见 [`22`](22-AREA-CONTRACT.md) |
| 写死高度的 `BeginChild(118px / 88px / 170px)` | 内容多了在小盒子里内部滚动，是拥挤感的直接技术来源 | 除镜头条与 HUD，竖向按比例分配 |
| 用 modal 报告普通结果 | 打断操作流 | 进 `STATUS_BAR`；阻塞性问题升级成就绪清单 |
| 空状态各面板各说一句 | 新用户不知道先点哪 | 一个主行动区 + 上手三步清单 |
| 「快速开始」默认折叠 | 首次用户根本看不到示例入口 | 并入开始板，默认可见 |
| 只给一个计数（「有 N 个缩略图缺失」） | 用户不知道要修哪几个 | 逐条清单 + 单条修复按钮 |
| 帮助菜单是纯文本 | 快捷键无法被发现 | 空状态与状态栏承担发现职责 |
| 把最终交付物（分镜总览）钉在屏幕三成高 | 成果被当边角料 | 审片模式升为全尺寸主舞台 |
| 在面板里推断业务状态 | `../07` 一票否决 | 三点状态、就绪原因、模式合法性都由 App 组装进快照 |

## 三、改了就会安静弄坏东西（硬约束）

| 约束 | 出处 | 弄坏什么 |
|------|------|----------|
| 窗口稳定 ID：`###Viewport` `###Workspace` `###Script` `###Library` `###Storyboard` | `src/UI/*.cpp` 的 `Begin` | 改后缀等于丢用户 `imgui.ini` 布局 |
| 帧序：`RenderScene` → 可选缩略图 `RequestReadback` → `imgui.BeginFrame` → 各面板 `Draw` → `imgui.Submit` → `EndFrame` | `src/App/Application.cpp` 主循环末尾 | 面板里插 bgfx 调用会撕裂帧；缩略图必须跨帧 `TakeReadback` |
| 纹理以 `std::uint16_t` 索引进出，`0xFFFF` 表示无效 | `viewportTextureIndex`、`StoryboardCardView::thumbTexture` | 公共头不许出现 bgfx 类型；忘判 `0xFFFF` 会画出垃圾 |
| 拖放载荷名 `DD_ASSET_ID` | `LibraryPanel` 设置 / 视口接收 | 改名即断掉「拖资产进场景」 |
| 视口输入必须由 `InvisibleButton` 吃掉，并清零 `io.MouseWheel` / `io.MouseWheelH`；窗口带 `NoScrollWithMouse` | 0.1.1 修的滚轮闪屏 | 滚轮会被当窗口滚动，视口重新开始闪 |
| 视口尺寸变化小于 2px 不上报 `ViewportResizeCommand` | `src/UI/WorkspacePanel.cpp` | 每帧重建 render target |
| 缩略图走异步回读（`RequestReadback` / `HasPendingReadback` / `TakeReadback`） | `backends/bgfx/BgfxRenderer.cpp` | 同步回读会在主循环里额外提交清空后的交换链，重新闪屏 |
| UI 只读 `AppViewState`、只发 `Core::Command` | `../01` 第四节 1、`../07` 第六节 | 架构漂移，直接停工改落点 |
| 后台线程不碰业务状态、UI、Renderer | `../01` 第五节 | 数据竞争 |
| `Storyboard` 不得 include `Script` / `Link` / `Camera` / `Renderer` | `../01` 第四节 4 | 依赖环 |

## 四、遇到冲突怎么办

1. 本文件夹与 `../00`–`../08` 冲突：**控制面**（依赖、模块、线程、第三方隔离、AI 冻结）以 `../01`/`../07` 为准；**界面主次与操作顺序**以 [`21`](21-WORKFLOW-V2.md) 为准。
2. 代码与文档冲突：停工，由 Wisdom 决定改代码还是改文档（`../08` 第八节）。
3. 某个任务做到一半发现要动第五个模块：说明落点切错了，回 [`24`](24-LANDING-CHECKLIST.md) 重新拆，不要继续堆。
