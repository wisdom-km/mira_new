# 20 - UI-PRO 目标、审计证据与范围

> 本版为什么存在、要达到什么、不做什么。审计基线：Phase 10 已完成的本机代码。
> 与 `../00` 的关系：`00` 的价值主张与 P0 红线继续有效；`00` 第二节的「剧本 → 资源 → 机位 → 画布 → 导出」降级为历史起点，界面主次改由 [`21-WORKFLOW-V2.md`](21-WORKFLOW-V2.md) 决定。

## 一、本版目标

| # | 目标 | 可验收的表现 |
|---|------|--------------|
| 1 | 有一个**常驻主线** | 任何时刻界面上都能看到「当前是哪一颗镜头、它差什么」 |
| 2 | 有**工作区模式** | 编剧 / 置景 / 掌机 / 审片四切片，切换只重排界面，不动领域状态 |
| 3 | 有**唯一检查器** | 同一时刻只显示一种面孔（镜头 / 场景对象 / 相机），不再三段并排 |
| 4 | 有**状态栏** | 所有 Command 反馈无需滚动即可看到 |
| 5 | 交付物**占主视觉** | 审片模式下分镜总览是全尺寸主舞台，不再是底部三成 |
| 6 | 空状态**有主行动** | 空工程首屏只有一个主行动区，而不是四个面板各说一句空话 |
| 7 | 密度**受控** | 除镜头条与 HUD，竖向空间按比例分配，不用写死高度的内滚小盒 |

## 二、审计证据（为什么现状不够专业）

### 1. Demo 路径是被界面逼出来的

布局写死在 `src/UI/WorkspacePanel.cpp` 的 `ApplyDefaultDockLayout`（左 0.21 → 右 0.25 → 下 0.30，左栏再上下 0.40），于是：

| Demo 步骤 | 代码证据 | 代价 |
|-----------|----------|------|
| 写剧本 | `剧本###Script` 钉在右栏 0.25，编辑器高度 `max(180, avail-116)` | 编剧在 25% 宽的竖条里写正文 |
| 找资源 | `资源库###Library` 在左栏下 40%，资产前有 TabBar + 搜索 + 来源单选 + 视图单选四排 chrome | 网格列数按 `avail.x / 108` 算，常只有 1～2 列 |
| 摆机位 | 机位预设同时出现在菜单「镜头」、视口工具条 SmallButton、`导演台###Workspace` 六个 Button | 同一个 `ApplyCameraPresetCommand` 三套同权重入口 |
| 看分镜 | `分镜###Storyboard` 固定中央下方 0.30，缩放 clamp 0.35～2.0 | 最终交付物永远只有三成高 |
| 导出 | 只在「文件」菜单第三组 + 分镜面板一个 SmallButton | 交付没有独立工作态；`exportStaleCount` 只弹一句「有 N 个缺失」，不列是哪几个 |

### 2. `导演台###Workspace` 一栏九职责

| 行号 | 职责 | 形态 |
|------|------|------|
| 353-364 | 工程头 | 文本 |
| 367-374 | 导入模型 + 透明导出 | Button / Checkbox |
| 375-392 | 快速开始 | CollapsingHeader，**默认折叠**，新用户看不到示例入口 |
| 397-414 | 场景层级 | `BeginChild` 固定 118px |
| 416-470 | 变换检查器 | `BeginTable` + 三组 `DragFloat3` |
| 472-507 | 相机增删改名 | `BeginChild` 固定 88px + `InputText` |
| 509-523 | 镜头绑定 | Button ×2 |
| 525-545 | 五个机位预设 | Button ×5，手算三列宽 |
| 547-561 | 灯光预设 | RadioButton ×3 |
| 563-566 | 状态文字 | `TextWrapped`，滚到底才看得见 |

### 3. 三个选择各自常驻，所以检查器只能分散

`script.SelectedShotId()`、`scene.SelectedId()`、`cameras.SelectedId()` 同时有值且互不相关，界面无法收敛出「当前对象」。唯一已有的联动是 `SelectShotCommand` 会把绑定相机设为当前相机（`src/App/Application.cpp:1116-1125`）——这是统一选择模型的现成地基。

### 4. 反馈只有一行字且藏在角落

`status` 是全应用唯一反馈通道（已关联镜头与相机 / 已适配分镜画布 / 请先选择镜头和相机 / 导出失败都写它），显示位置是左栏最底部。

### 5. 代码中不存在（不是「做得不好」，是「没有」）

- 状态栏、工具条 / 工具模式、工作区模式切换
- `AppViewState` 的统一选择字段（`selectionKind` / `selectionId`）
- 布局重置与布局预设（`ApplyDefaultDockLayout` 遇到已有节点直接 `return`）
- 横向镜头条
- 导出就绪清单（只有 `exportStaleCount` 一个计数）
- 导出历史记录

## 三、本版范围

### 在范围内

- 界面布局、信息层级、空状态、密度、模式切换
- `Core::Command` 新增值对象与 `Application.cpp` 的 visit 分支
- `UI::AppViewState` 新增只读字段（由 App 组装）
- 新增 ImGui 窗口（在现有 `UI` 模块的现有 `.cpp` 内 `Begin`，见 [`23`](23-CONTRACT-DELTA.md)）

### 不在范围内（本版一律不碰）

- 领域算法：剧本解析、分镜自动布局、机位数学、导出像素路径
- 渲染管线、帧序、纹理生命周期
- `.ddproj` 格式（新增字段若需持久化，先改 `../modules/project-file.md` 并由 Wisdom 批准）
- AI 模块（继续冻结）
- `00` 的 P0 红线：Undo/Redo、时间轴、自由连线画布、ImGuizmo、视频导出

## 四、本版红线（违反即停工改落点）

1. UI 只读 `AppViewState`、只发 `Core::Command`；面板里不得解析剧本、算机位、调 bgfx。
2. 不新增 CMake 目标，不新增 `src/<Module>/`；新窗口落在现有 `src/UI/*.cpp` 内。
3. 不拆 `src/App/Application.cpp`；新 visit 分支加在原处。
4. 不改稳定窗口 ID 后缀、`0xFFFF` 无效纹理约定、拖放载荷名 `DD_ASSET_ID`、帧序与视口输入独占策略（详见 [`25-DO-NOT.md`](25-DO-NOT.md) 第三节）。
5. 新增快照字段只进 `include/DirectorDesk/UI/IPanel.h`；新增 Command 只进 `include/DirectorDesk/Core/Command.h` 与其 `variant`。
6. 每个任务收工更新 [`26-UI-PRO-STATUS.md`](26-UI-PRO-STATUS.md) 与 `../03-CURRENT-STATUS.md`。
