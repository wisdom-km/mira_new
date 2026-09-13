# 30 - 同类项目调研与差距清单（RESEARCH AND GAPS）

> 2026-09-13 调研结论。目的不是抄功能，而是回答「一个做 AI 短剧前置分镜的开源桌面工具，缺什么就不算完整」。
> 每条差距都指向 [`34`](34-LANDING-CHECKLIST.md) 的任务 ID；没有任务 ID 的条目就是本版**明确不做**。

## 一、调研对象与可借鉴点

| 类别 | 代表项目 / 产品 | 它做对了什么 | DirectorDesk 对应态度 |
|------|-----------------|--------------|----------------------|
| 传统分镜工具 | Storyboarder（wonderunit）、Blender Storypencil/Grease Pencil、Toon Boom Storyboard Pro | 镜头即一等对象；每镜有景别 / 运镜 / 时长 / 对白等结构化字段；导出 PDF/图像序列/剪辑软件工程 | 借「镜头元数据」与「导出即交付包」；不借绘画能力（`00` 红线：不做自由绘制） |
| 3D 预演 / Blockout | FrameForge、Blockout 系列、Unreal Sequencer 的 previs 用法 | 真实焦距/传感器、机位表、多机位对比 | 借「相机以焦距表达」进入镜头包；不借时间轴与关键帧（`00` 红线） |
| AI 前置工具 | CozyClay 一类「粘土/积木搭场景 → 出参考图 → 喂视频模型」；带 MCP 的 3D 编辑器 | **核心价值是下游对接**：一键出「首帧 + 相机 + 提示词」；用 MCP 让 AI Agent 驱动软件 | 借「镜头包」格式（F2）；MCP 属于 AI 接线，**冻结**，本版只把镜头包做成 Agent 可读的 JSON |
| AI 短剧工作流 | `eternityspring/shuohao-skills`（Node 脚本 Skill，在 Claude Code / Codex 里跑，产出角色/美术/剧本/分镜 JSON 并带质量门） | 把「小说 → 剧本 → 分镜」拆成可替换的 Skill；每步产出结构化 JSON | 借「Skill 作为可选、可替换的包」；DirectorDesk 只做 **消费方与分发方**（F3），不做运行时 |
| AI 视频模型输入 | Seedance、Kling、Veo、Wan 系列 | 普遍接受首帧/尾帧图、部分接受深度或姿态控制图、需要结构化提示词 | 镜头包 v1 出彩色 PNG + 相机 + 场景描述 + 提示词字段；深度图列为 v2 可选（F2 不做） |
| 开源桌面工具的「门面」 | 任何 star 数上千的 C++ 桌面项目 | README 首屏有截图/GIF、英文摘要、一键安装、Good First Issue、CI 徽章绿 | 全部缺失或为红（F0） |

## 二、差距清单（按严重度）

### A. 会让第一个陌生贡献者当场离开的

| # | 差距 | 证据 | 任务 |
|---|------|------|------|
| A1 | GitHub Actions Windows + macOS **双红** | `windows-latest` 已无 VS 17 2022 生成器；macOS 无条件对 `dx11/metal/glsl/spirv` 四后端跑 `shaderc`，HLSL 在 mac 上失败 | FND-01、FND-02 |
| A2 | README 无英文摘要、无截图/GIF、无「30 秒上手」 | 根目录 `README.md` | FND-07 |
| A3 | 无 issue / PR 模板、无 Good First Issue 标签约定 | `.github/` 仅 workflows | FND-08 |
| A4 | Release 打包靠本机脚本，不是 tag 触发的 Action | `packaging/windows/build-installer.ps1` 手工执行 | FND-09 |

### B. 会让用户在十分钟内撞到的

| # | 差距 | 证据 | 任务 |
|---|------|------|------|
| B1 | 打开工程 / 保存工程**同步**加载全部模型与逐文件 SHA-256，大场景整段卡 UI | `Application.cpp: UploadSceneModels`；`ProjectBinding.cpp:89/103 Sha256File` | FND-11、FND-12 |
| B2 | 场景对象**没有删除 / 复制 / 显隐**命令 | `Command.h` 无 `DeleteNode*`；`.ddproj` 有 `visible` 字段但无入口 | FND-13 |
| B3 | 新建镜头永远追加到剧本末尾，无法在选中镜头后插入 | `Script::Document::InsertShot()` 无参数 | FND-14 |
| B4 | 删除镜头靠文本扫描 `###` 行，遇到四级标题、代码围栏会误伤 | `Document.cpp:58 RemoveShotSection` | FND-15 |
| B5 | `Ctrl+E` 与菜单项写死 `"1080p"/"2k"`，忽略检查器里已选分辨率 | `WorkspacePanel.cpp:675/678/868` | FND-03 |
| B6 | 删资源库索引会把工程标脏 | visit `RemoveLibraryAssetCommand` 分支 | FND-04 |
| B7 | `ViewportResizeCommand` 每帧都推，2px 阈值只挡住了 RT 重建，没挡住命令 | `WorkspacePanel.cpp:939` 在 `if` 外 | FND-05 |
| B8 | 官方资产保存进 `.ddproj` 时被写成 `user-library` + 绝对缓存路径，工程不可移植；与 `modules/project-file.md` 的 `official` 定义不符 | `ProjectBinding.cpp:99–114` | FND-16 |

### C. 让它「不算完整产品」的

| # | 差距 | 任务 |
|---|------|------|
| C1 | 镜头没有景别 / 运镜 / 时长 / 提示词等结构化字段，分镜总览只有标题 | FND-20、FND-22 |
| C2 | 导出物只有 PNG，无法直接喂 AI 视频模型（缺相机、缺提示词、缺场景描述） | FND-21 |
| C3 | 相机只有垂直 FOV，导演语言是焦距 | FND-21（镜头包内换算）；不改 `OrbitCamera` |
| C4 | 剧本只能手写，无法从上游（Skill / LLM）结构化产出导入 | FND-30、FND-31 |
| C5 | Skill 无处发现、无处安装 | FND-32、FND-33 |
| C6 | 角色资产将带骨骼，当前 manifest 与加载器没有表达位置 | FND-40（待批准） |

### D. 代码健康（不影响用户，但影响下一个改代码的人）

| # | 差距 | 任务 |
|---|------|------|
| D1 | `Application::Run()` 1200+ 行、几十个局部变量当状态、无法单测 visit | FND-10 |
| D2 | 两份 SHA-256 实现（`Core::Sha256` 与 `ProjectFile.cpp` 手写） | FND-06 |
| D3 | UI 用字符串比较判断业务状态（`"未关联"`、`"shot"`、`selectionKind` 字符串） | FND-17 |
| D4 | 导出记录回点镜头按**标题**匹配卡片，同名镜头会点错 | FND-18 |
| D5 | `AppViewState` 每帧全量重组（当前规模可接受，记录不改） | 不做，见 [`35`](35-DO-NOT.md) |

## 三、明确不做（本版）

| 不做 | 依据 |
|------|------|
| 在软件内运行 Skill（Node 子进程）、MCP 服务端、任何真实 AI 调用 | `../07` 第二节 AI 冻结；F3 只做导入与分发 |
| 自由绘制、时间轴、关键帧、相机动画 | `../00` 第五节 |
| 深度图 / 姿态图导出 | 镜头包 v2 议题，先验证 v1 是否被下游真的用起来 |
| 多人协作、云同步 | 超出桌面工具定位 |
| 把 `AppViewState` 改成脏标记增量更新 | 当前规模无性能问题，改了只增复杂度 |
