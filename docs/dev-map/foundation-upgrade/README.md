# FOUNDATION · 地基、镜头包与 Skills 融合升级版（入口）

> 本文件夹是 DirectorDesk 在 **0.1.2 之后** 的工作集，代号 **FOUNDATION**。来源：2026-09-13 Wisdom 与 Cursor AI 的一轮完整审计——GitHub 同类项目调研 + 全仓代码审查 + `shuohao-skills` 融合讨论。本文件夹把那次对话里**所有需要改代码、改契约、改文档的结论**写成可执行任务，供任意模型按任务 ID 逐条落地。
> 权威关系：`../00`–`../08` 仍是最高约束（依赖方向、模块数锁定、UI 只发 Command）。AI 已按 F5 解冻（见 [`35`](35-DO-NOT.md) 第四节）。`../ui-pro-upgrade/` 仍管界面主次与区域 ID。本文件夹只在 [`35-DO-NOT.md`](35-DO-NOT.md) 第四节列出的几处**明确改写**既有决策，其余一律服从。
> 批准者 Wisdom（口头授权编写本执行文档，2026-09-13）。凡标注「待批准」的任务，动手前仍需 Wisdom 单独点头。

## 一、这个版本是什么

| 项 | 值 |
|----|----|
| 版本代号 | **FOUNDATION**（地基 + 镜头包 + Skills 融合） |
| 基线 | 产品 tag `v0.1.2`，UI-PRO W1–W3 已落地 |
| 一句话目标 | 把「能用的 Demo」变成「别人愿意 clone、愿意提 PR、愿意拿来给 AI 视频喂参考图」的开源工具 |
| 主线对象 | 仍是 **Shot**。本版给 Shot 加三样东西：**元数据**（景别/运镜/时长/提示词）、**镜头包**（PNG + 相机 + 场景 JSON，喂 AI 视频）、**来源**（可从 Skill 产出的分镜 JSON 导入） |
| 目标版本 | 0.1.3（止血）→ 0.2（地基）→ 0.3（镜头包）→ 0.4（Skills）→ 0.5（角色包）→ **0.6（AI 解冻，F5 已齐、未打 tag）** |
| 允许改 | 所有已有模块内部；`Core::Command` 新增值对象；`AppViewState` 新增只读字段；`App` 目标内**分文件**；`modules/*.md` 契约升小版本 |
| 不允许改 | 模块集合（不新建 `src/<Module>/` 与 CMake 目标）；依赖方向；第三方类型进公共头 |

## 二、为什么要做（审计一句话结论）

代码结构是对的（十三个模块、单向依赖、Command 单入口），但**产品还停在「作者自己能用」**：CI 双红没人管、打开工程会卡 UI、场景对象删不掉、镜头只能加在末尾、导出物只有一张 PNG——而同类工具（Storyboarder、Blockout、CozyClay 一类 AI 前置工具）的核心价值恰恰是「一键喂给下游 AI」。完整证据链见 [`30-RESEARCH-AND-GAPS.md`](30-RESEARCH-AND-GAPS.md)，逐文件代码问题见 [`31-CODE-REVIEW.md`](31-CODE-REVIEW.md)。

## 三、阅读顺序

每轮 vibe coding **必读前四项**：

1. 本文件（版本身份与权威关系）
2. [`36-FOUNDATION-STATUS.md`](36-FOUNDATION-STATUS.md) — 现在做到哪、这轮该做哪一行
3. [`34-LANDING-CHECKLIST.md`](34-LANDING-CHECKLIST.md) — 你要做的任务 ID 的落点、DoD
4. [`35-DO-NOT.md`](35-DO-NOT.md) — 停手清单、硬约束、本版改写了哪几条旧决策

按需读：

- [`30-RESEARCH-AND-GAPS.md`](30-RESEARCH-AND-GAPS.md) — 同类项目调研、差距清单、每条差距对应哪个任务
- [`31-CODE-REVIEW.md`](31-CODE-REVIEW.md) — 逐文件代码审查，每条带「现状 / 问题 / 改法 / 验收」
- [`32-SKILLS-INTEGRATION.md`](32-SKILLS-INTEGRATION.md) — `shuohao-skills` 是什么、三层融合方案、角色骨骼包的预留方式
- [`33-CONTRACT-DELTA.md`](33-CONTRACT-DELTA.md) — 新 Command / 快照字段 / 三个新 JSON 格式 / 两个契约小版本升级的确切签名

## 四、波次索引

| 波次 | 目标版本 | 一句话 | 是否改契约 |
|------|----------|--------|------------|
| **F0 · 止血** | 0.1.3 | CI 全绿、六个明确 bug、开源门面（README 英文 / 截图 / 模板） | 不改 |
| **F1 · 地基** | 0.2 | `Application.cpp` 在 App 内分文件；打开/保存不再卡 UI；场景对象可删/复制/显隐；镜头可插在中间 | 新 Command 4 个，快照字段 1 组 |
| **F2 · 镜头包** | 0.3 | 剧本镜头元数据；导出「镜头包」（PNG + camera + scene JSON）；分镜卡片显示元数据 | `script-format` 1.0 → 1.1；新增 `shot-package.md` |
| **F3 · Skills 融合** | 0.4 | 导入 Skill 产出的分镜 JSON 成剧本；Skill 作为官方资产分发与安装；**不在软件内运行 Skill** | `asset-manifest` 1 → 2；新增 `storyboard-import.md` |
| **F4 · 角色包与体验** | 0.5 | 角色资产包（含骨骼字段的 manifest）静态加载；Gizmo；Undo；总览 PDF | Wisdom 2026-09-15 已批准 FND-41/42/43 |
| **F5 · AI 解冻** | 0.6 | OpenAI 兼容图像/视频；设置密钥；运行已安装 Skill | Wisdom 2026-09-15 批准 |

任务 ID 形如 `FND-xx`，定义在 [`34`](34-LANDING-CHECKLIST.md)，进度只在 [`36`](36-FOUNDATION-STATUS.md) 记。

## 五、工作方式（与 `../06`/`../07` 一致，不重复）

- 落点顺序不变：Panel → `Core::Command` → `App` 的 visit → App 组装快照 → 领域规则加测试 → 更新 [`36`](36-FOUNDATION-STATUS.md) 与 `../03-CURRENT-STATUS.md`。
- 一轮只做 [`34`](34-LANDING-CHECKLIST.md) 里的一行或一组；F0 全部完成并 CI 全绿之前不开 F1。
- 契约先于代码：F2/F3 任何一行代码落地前，先把 [`33`](33-CONTRACT-DELTA.md) 对应格式抄进 `../modules/`，再写解析器与测试。
- 与本文件夹冲突时：控制面以 `../01`/`../07` 为准；界面主次以 `../ui-pro-upgrade/` 为准；本版的新格式与新 Command 以 [`33`](33-CONTRACT-DELTA.md) 为准。
- 并行的界面重设计 **UI-CLARITY**（`../ui-clarity/`）U0 不依赖本版；U1/U2 依赖 F1/F2 若干字段（见 `../ui-clarity/44`）。本版任务不因它改动落点。
- 未经 Wisdom 明确要求，不得 commit、tag、push。
