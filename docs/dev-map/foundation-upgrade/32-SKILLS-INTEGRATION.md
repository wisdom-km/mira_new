# 32 - Skills 融合方案（SKILLS INTEGRATION）

> 回答 Wisdom 的三个问题：`shuohao-skills` 这类东西怎么进 DirectorDesk；「剧本 + 分镜 + 导演台」怎么融成一条线；将来带骨骼的角色包怎么预留。
> 硬前提（FOUNDATION F5 改写，Wisdom 2026-09-15）：L1 / L2 不变。**L3 已解冻**：运行 Skill 落在 AI 模块（`skill.json` + 子进程或内置拷贝），产出仍是 L1 的 `storyboard-import.json`。图像/视频生成走 OpenAI 兼容 HTTPS，不嵌供应商 SDK。

## 一、`shuohao-skills` 是什么（对本项目有意义的部分）

- 一组在 Claude Code / Codex 这类 Agent 里运行的 Node 脚本 Skill，流水线大致为：小说/大纲 → 人物设定 → 美术设定 → 剧本 → **分镜 JSON**，每步带质量门校验。
- 关键特征：**产物是结构化 JSON**，而不是软件插件。它假设旁边有一个 Agent 在跑脚本、读结果、决定下一步。
- 与 DirectorDesk 的关系：它负责「想」，DirectorDesk 负责「摆机位 + 出参考图」。两者的交接点只有一个文件：**分镜 JSON**。

结论：不要把 Skill 做成 C++ 插件系统。把交接点做扎实，Skill 生态就自然能换、能选、能加。

## 二、三层方案

```text
┌──────────────────────────────────────────────────────────────────┐
│ L3  运行 Skill（子进程 / 内置拷贝产出 JSON）   ← F5 · FND-55          │
├──────────────────────────────────────────────────────────────────┤
│ L2  分发 Skill（官方清单 format=skill → 下载到本机 → 资源库可见） │  F3 · FND-32/33
├──────────────────────────────────────────────────────────────────┤
│ L1  消费 Skill 产出（storyboard-import.json → Markdown 剧本）     │  F3 · FND-30/31
└──────────────────────────────────────────────────────────────────┘
                 ↑ 上游 Skill 在 Agent 里跑，把 JSON 放到工程目录
                 ↓ DirectorDesk 出镜头包（F2 · FND-21），Agent 拿去喂视频模型
```

### L1 · 消费：分镜 JSON → 剧本（Script 名词）

- DirectorDesk 定义**自己的**导入格式 `storyboard-import.json`（[`33`](33-CONTRACT-DELTA.md) 六）。理由：上游 Skill 的 schema 会变，不能让 Script 模块跟着别人的字段名走。任何 Skill 想接 DirectorDesk，只要多写一步「输出成 DirectorDesk 导入格式」——这一步本身也可以是一个 Skill。
- 落点：`Script::ImportStoryboard(jsonText) -> Result<ImportResult{markdown, diagnostics}>`。它只生成符合 `script-format 1.1` 的 Markdown 文本（含稳定 ID 与 `> key: value` 元数据行），然后走既有 `SetScriptTextCommand` 路径进 Document。**Storyboard 模块完全不参与**（它只认剧本快照）。
- 两种合并策略（导入对话框二选一）：
  - **替换**：清空当前剧本，整份生成。适合新工程。
  - **追加**：把 JSON 里的场次追加到末尾；ID 冲突时给新 ID 并诊断。
  - 不做「智能合并」——那是 Agent 的活。
- 入口：编剧模式检查器「从分镜 JSON 导入…」；`ImportStoryboardCommand`（弹对话框）与 `ImportStoryboardFromPathCommand`（拖放 / 命令行）。

### L2 · 分发：Skill 是一种官方资产（Asset 名词）

- `asset-manifest` 升到 2：`format` 允许 `skill`，`entrypoint` 为 `SKILL.md`，`files[]` 列出脚本与模板。**下载、SHA-256 校验、原子缓存、取消、状态机全部复用 `OfficialCatalog`**，一行不改。
- 缓存位置沿用 `<用户数据目录>/DirectorDesk/assets/official/<skill-id>/<version>/`。~~资源库多一个类别「Skills」；检查器显示 `SKILL.md` 的前 N 行~~ → **UI 宿主已改**（UI-CLARITY [`47`](../ui-clarity/47-FULL-FLOW-AUDIT.md) 5.1 / 6.5，2026-09-15）：Skill **不在资源库网格出现**，管理（装 / 卸 / 从官方获取 / 设默认 / README / 打开文件夹）在 `编辑 → 设置… → Skills` 页（UIC-59）；运行在编剧模式「分镜」段（UIC-60）。`Asset::Library` 仍索引 Skill，只是 `LibraryPanel` 按 `kind` 过滤。分发管线不变。
- 用户把这个路径交给自己的 Agent（Claude Code / Codex 的 skills 目录）。**DirectorDesk 不替用户装进 Agent**——各 Agent 的目录约定不同，装错比不装更糟。
- Wisdom 的 Skill 仓库（或 fork 的 `shuohao-skills`）只要在官方清单里登记一条 `format: skill` 的资产，就能被发现与下载。「更好的 Skill」加进来 = 清单加一行。

### L3 · 运行：AI 模块（F5）

- 运行 Skill 的落点是 **AI 模块**：`copy-output`、`openai-compat-json`（文本 LLM）或 `argv` 子进程。输出仍然是 L1 的 `storyboard-import.json`。
- 「运行 Skill」（UIC-60 后为编剧模式「生成分镜 ▾」）只在安装目录存在合法 `skill.json` 时启用。没有描述文件的 Skill 仍只打开文件夹，交给外部 Agent。
- 不做 MCP 服务端、不嵌 Node 运行时。

## 三、「剧本 + 分镜 + 导演台」融成一条线

现有四模式已经是这条线（编剧 → 置景 → 掌机 → 审片）。本版补的是**两端**：

| 端 | 现状 | 本版 | 任务 |
|----|------|------|------|
| 入口 | 只能手写 Markdown | 可从分镜 JSON 导入；镜头带景别/运镜/时长/提示词 | FND-20、FND-30 |
| 出口 | 一张 PNG | 镜头包：PNG + 相机（含焦距）+ 场景节点 + 剧本正文 + 元数据 + 提示词 | FND-21 |
| 中间 | 卡片只有标题 | 卡片显示元数据行；镜头条同样 | FND-22 |

一个完整回合：安装或下载 Skill → 可在软件内运行（有 `skill.json`）或在 Agent 里跑 → 导入 JSON → 摆机位 → 导出镜头包 / 在软件内生成图像或视频。

## 四、角色资产包（含骨骼）的预留方式

Wisdom 将在 GitHub 上做一系列角色包，带骨骼。本版**只做能立刻验证的部分**：

| 层 | 本版做 | 本版不做 |
|----|--------|----------|
| 清单 | `asset-manifest 2`：`kind: character`、可选 `rig: { "type": "humanoid" \| "custom" \| "none", "source": "mixamo" \| ... }`；`format` 仍是 `glb` | 骨骼绑定的校验（只做字段合法性） |
| 加载 | `cgltf` 已能读带 skin 的 GLB；`Asset::ModelData` 加 `bool hasSkin`，加载时忽略 skin 只取静态网格（当前 bind pose） | 蒙皮渲染、骨骼动画、姿态库 |
| UI | 资源库「角色」类别显示 `rig` 徽标；检查器显示「含骨骼（静态显示）」 | 姿态编辑器 |
| 后续 | 姿态库 = 每个姿态一个预烘焙 GLB（同一 `assetId` 不同 `files[]`），DirectorDesk 只需切换 entrypoint 就能换姿态——**不需要运行时蒙皮**就能覆盖分镜 80% 的需求 | 运行时蒙皮属于 Renderer 大改，另开版本 |

这条路的好处：Wisdom 做角色包时只要按 manifest 2 填字段，DirectorDesk 0.4 就能下载、显示；将来做姿态包也不需要改软件。

## 五、Skill 作者接入指南（写进仓库 `docs/skills/README.md`，FND-34）

给想让自己 Skill 出现在 DirectorDesk 资源库里的作者三步：

1. 让你的 Skill 最后一步输出 `storyboard-import.json`（格式见 `modules/storyboard-import.md`）。
2. 在你的仓库放 `SKILL.md` + 脚本；用 `tools/make-skill-manifest`（FND-34 提供的脚本）算出 `files[]` 的 SHA-256 与大小。
3. 向官方清单仓库提 PR 加一条 `format: skill` 的资产。

DirectorDesk 侧不需要任何改动。
