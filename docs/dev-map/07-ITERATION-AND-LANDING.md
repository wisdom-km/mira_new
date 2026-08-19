# 07 - 迭代政策与功能落点

> Demo 阶段的现行工作方式：部分模块化、锁定模块数、按名词落代码。批准者 Wisdom。
> 为什么要接缝、好处与弊端、vibe coding 哪些能做哪些不能：见 `08-SEAMS-AND-VIBE-CODING.md`。
> 本文件是落点，不复制技术栈、契约签名或 Phase 清单。那些仍以 `00`—`05` 与 `modules/` 为准。
> 与本文件冲突时：用户路径与依赖方向以 `00`/`01` 为准；加删功能的落点以本文件为准。

## 一、现阶段策略

DirectorDesk 已走完 P0 主路径，仍处于可加、可删功能的 Demo 阶段。

- **最小可执行**决定做什么；**模块化**决定放哪、删时从哪拆。
- 采用**部分模块化**：控制面全部保留，产品模块不再扩张，预留模块冻结。
- 不要退回单文件大泥球，也不要为小功能新建第 14 个库。
- **暂缓**：不拆分 `src/App/Application.cpp`。编排层允许厚；痛点未到之前不做文件切开。

控制面（必须遵守，见 `01`）：

- 依赖单向；UI 只发 Command、只读 `AppViewState`
- 第三方类型不进公共业务头（GLFW / bgfx / libcurl / spdlog）
- 主线程独占业务状态；后台只交回值对象
- 模块之间传快照，不传对方的对象

## 二、模块数锁定

当前 CMake 产品模块集合视为**锁定**。未经产品负责人批准，禁止新增 `src/<Module>/` 静态库。

| 类别 | 模块 | 现阶段怎么对待 |
|------|------|----------------|
| 控制面 | Core、Platform、Renderer、`backends/` | 保留，不为整洁而重写 |
| 主路径落点 | Script、Scene、Camera、Asset、Storyboard、UI、Export、App | 保留；新功能填进这些名词 |
| 薄但独立 | Link | 保留类型与测试；不要往里堆逻辑；不必为了「少一个库」而合并 |
| 预留空岛 | AI | **冻结**：不接线、不扩展、不在面板留入口。接入真实供应商前当它不存在 |

允许新建模块的条件（需同时满足至少两条，并先修订本文件与 `03`）：

1. 出现第二种实现（例如新的模型格式加载器之外，还要换图形后端）
2. 出现第二个调用方（同一职责被两条主路径使用）
3. 不启动窗口就无法验证该职责
4. 人/AI 反复把该职责写进错误文件
5. 与邻接代码的变更频率已经明显不同

一个小需求若要同时改五个模块，优先怀疑切错了，而不是再拆一层。

## 三、功能落点（按名词）

先问用户动的是哪个名词，再问像素、文件、网络、窗口归谁。

| 变化属于 | 落点 | 不要放进 |
|----------|------|----------|
| 看见、点击、控件布局 | `UI::IPanel` | Scene / Script / Renderer |
| 一种新的用户意图 | `Core::Command` + `App` 的 visit 分发 | 面板回调里直接改业务状态 |
| Markdown 场次 / 镜头 | Script | Storyboard、UI |
| 节点位置 / 旋转 / 缩放 | Scene | Camera、UI |
| 轨道机位、预设、多相机 | Camera | Scene 节点 |
| Shot 绑定到哪台相机 | Link | Script 解析器 |
| 画布卡片、自动布局、折叠 | Storyboard | 直接 include Script |
| 缩略图的像素 | App 调度 Renderer，结果交回 Storyboard | Storyboard 或 UI 直接调 bgfx |
| 导入 OBJ/GLB、官方下载 | Asset；IO 走 Platform | Library 面板里堆格式 `if` |
| 导出 PNG / 分镜总览 | Export + Renderer 离屏回读 | 截 ImGui 窗口 |
| `.ddproj` 读写、跨模块一致性 | App（`ProjectFile` / `ProjectBinding`） | 各域私自写自己的工程格式 |
| 路径、对话框、线程、HTTP | Platform | 业务模块里的 `wchar_t` / curl |
| 真正画出来 | `IRenderer` + `backends/bgfx` | Export / UI 公共头 |
| 文生图 / 视频 | AI 接口（当前冻结） | 面板里供应商 SDK |

边界与接口签名见 `01-ARCHITECTURE-MAP.md`。数据格式见 `modules/`。

## 四、加功能

固定顺序，缺一步视为落点错误：

1. 用户要看见或点到 → 改对应 Panel，只读 `AppViewState`
2. 需要新意图 → 在 `Core::Command` 增加值对象，并加入 `variant`
3. 在 `Application.cpp` 的 visit 里分发到拥有该名词的模块
4. 若跨模块（例如剧本变了要重排画布）→ 只允许 App 组装新快照
5. 领域规则加测试；不要只靠点 UI
6. 更新 `03-CURRENT-STATUS.md`；若改了契约，同步对应 `modules/*.md`

示例：

- 新相机预设 → `Camera::Presets` + 面板发已有或新增的 `ApplyCameraPresetCommand`。不要在面板里算轨道。
- 新模型格式 → 新 `IModelLoader` 实现并注册。不要改 Library 面板的业务分支。GPU 上传仍由 App / Renderer 做。
- 卡片多显示一个剧本字段 → 先改 Script 快照字段，Storyboard 只展示，UI 只画。不要在 `StoryboardPanel` 里重新解析 Markdown。

## 五、删功能

按层剥，避免 `AppViewState` 变成墓地：

1. 去掉 Panel 入口
2. 去掉对应 Command 与 visit 分支
3. 去掉快照字段、工程文件字段（若已持久化）和测试
4. 整个名词不再存在时，再删模块与 CMake 目标，并修订 `01` 与本文件

只藏按钮、留下死命令和死字段，视为未完成删除。

AI 模块现阶段不删库，只冻结：Demo 闭环未使用，删构建收益小；真要做供应商时再打开。

## 六、一票否决

出现任一项，停止实现，先改落点：

- UI 持有业务对象引用，或在面板回调里直接改 Scene / Script
- Storyboard include Script / Link / Camera / Renderer
- 公共头出现 bgfx、GLFW、curl、spdlog 类型
- 后台线程改场景图、UI 或 Renderer
- 新格式靠中心化 `if (ext == …)` 而不是注册表
- 为单个按钮或单个预设新建 CMake 库
- 在冻结解除前给 AI 接真实网络、密钥 UI 或供应商 SDK

## 七、与范围文件的关系

- `00` 的用户主路径仍然有效：加功能不得打断「剧本 → 资源 → 机位 → 画布 → 导出」闭环。
- `00` 的「P0 明确不做」是当时的范围锁。Demo 之后若要重开或删除某项，先由产品负责人改 `00`/`03`，再改代码；不要在实现里偷偷做。
- `02` 的 Phase 顺序已走完。此后工作以 `03` 的下一步和本文件为准，不再发明新的 Phase 号，除非负责人批准。
