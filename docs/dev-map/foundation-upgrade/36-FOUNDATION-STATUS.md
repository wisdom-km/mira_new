# 36 - FOUNDATION 当前状态

> 本版进度的唯一真实来源。每轮 vibe coding 开始先读这里，结束必须更新这里，并同步 `../03-CURRENT-STATUS.md`。
> 只记录状态与验证，不复制契约（契约在 [`33`](33-CONTRACT-DELTA.md)，任务定义在 [`34`](34-LANDING-CHECKLIST.md)）。

## 当前快照

| 项 | 值 |
|----|----|
| 版本 | FOUNDATION（地基 + 镜头包 + Skills 融合） |
| 阶段 | **F0 止血进行中** |
| 代码基线 | 产品 tag `v0.1.2`（UI-PRO W1–W3 已在内） |
| 最后更新 | 2026-09-13 |
| 更新者 | Cursor AI |
| 下一个允许执行的工作 | **FND-07** 产品截图 / GIF。macOS CI 已暂停（Wisdom）。F0 未齐之前不开 F1 |
| 当前波次 | F0 · 止血 |

## 任务进度

状态取值：`未开始` / `进行中` / `已完成` / `已放弃` / `待批准`。

### F0 · 止血（0.1.3）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| FND-01 | Windows CI 生成器 | 已完成 | `ci.yml` `os: windows-2022`；已 push `81a25af` |
| FND-02 | shader 按平台裁剪 | 已完成 | CI metal 已过。macOS 另因 FileDialog 弃用失败；Windows 构建过、`External file change` 因 mtime 同刻失败。补丁待推 |
| FND-03 | `Ctrl+E` 用当前分辨率 | 已完成 | 空 `resolutionId`；菜单置顶「按当前选择导出」；`ExportCurrentShotCommand` 默认空 |
| FND-04 | 删索引不标脏 | 已完成 | `RemoveLibraryAssetCommand` 不再写 `projectDirty` |
| FND-05 | `ViewportResizeCommand` 只在变化时推 | 已完成 | Push 移进 2px / 首帧阈值 |
| FND-06 | 统一 SHA-256 | 已完成 | `ProjectFile::Sha256File` 调 `Core::Sha256Hex`；手写块删除 |
| FND-07 | README 英文摘要 / 截图 / 30 秒上手 | 进行中 | 英文摘要、徽章、30 秒上手已写；产品截图与 GIF 未拍 |
| FND-08 | Issue / PR 模板 | 已完成 | Bug / Feature / Asset + PR 勾选项 |
| FND-09 | tag 触发 Release | 已完成 | `release.yml` + 安装脚本从 `CMakeLists.txt` 读版本；待打 `v*` 出草稿 |

### F1 · 地基（0.2）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| FND-10 | App 内分文件 + Dispatch 测试 | 未开始 | |
| FND-11 | 异步模型加载 | 未开始 | |
| FND-12 | 保存免重算哈希 / 后台哈希 | 未开始 | |
| FND-13 | 节点删 / 复制 / 显隐 + 三个对齐按钮 | 未开始 | |
| FND-14 | 镜头插在选中之后 | 未开始 | |
| FND-15 | 删 / 插镜头改用解析器行号 | 未开始 | |
| FND-16 | 官方资产按 `official` 三元组持久化 | 未开始 | |
| FND-17 | 枚举替代字符串判断 | 未开始 | |
| FND-18 | 导出记录按 ID 回点 | 未开始 | |

### F2 · 镜头包（0.3）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| FND-20 | `script-format 1.1` 元数据 + `SetShotMetaCommand` | 未开始 | |
| FND-21 | `shot-package 1` + `ExportShotPackageCommand` | 未开始 | |
| FND-22 | 卡片 / 镜头条 metaLine | 未开始 | |
| FND-23 | 总览 `board.json` | 未开始 | |

### F3 · Skills 融合（0.4）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| FND-30 | `storyboard-import 1` + `Script::ImportStoryboard` | 未开始 | |
| FND-31 | 导入 Command / 入口 / 诊断 | 未开始 | |
| FND-32 | `asset-manifest 2`（`skill` / `kind` / `rig`） | 未开始 | |
| FND-33 | 资源库 Skills 类别 + 检查器 Skill 面孔 | 未开始 | |
| FND-34 | Skill 作者指南 + 清单生成脚本 | 未开始 | |
| FND-35 | 导入示例 | 未开始 | |

### F4 · 角色包与体验（0.5）

| ID | 任务 | 状态 | 验证记录 |
|----|------|------|----------|
| FND-40 | 角色包静态加载 | 未开始（依赖角色包仓库） | |
| FND-41 | ImGuizmo | 待批准 | |
| FND-42 | Undo / Redo | 待批准 | |
| FND-43 | 总览 PDF | 待批准 | |

## 契约落地追踪

| 契约 | 定义在 | 状态 |
|------|--------|------|
| `DeleteNodeCommand` / `DuplicateNodeCommand` / `SetNodeVisibleCommand` | [`33`](33-CONTRACT-DELTA.md) 一 | 未写入 |
| `InsertShotCommand.afterShotId` | [`33`](33-CONTRACT-DELTA.md) 一 | 未写入 |
| `SetShotMetaCommand` / `ExportShotPackageCommand` | [`33`](33-CONTRACT-DELTA.md) 一 | 未写入 |
| `ImportStoryboardCommand` / `ImportStoryboardFromPathCommand` | [`33`](33-CONTRACT-DELTA.md) 一 | 未写入 |
| `SelectionKind` / `CardKind` 枚举、`sceneLoadPending/Total`、`projectSaveInProgress`、`selectedShotMeta`、`importDiagnostics`、`metaLine`、`ExportLogView.shotId`、`SceneNodeView.visible/hasSkin` | [`33`](33-CONTRACT-DELTA.md) 二 | 未写入 |
| `../modules/script-format.md` 1.1 | [`33`](33-CONTRACT-DELTA.md) 三 | 未改 |
| `../modules/asset-manifest.md` 2 | [`33`](33-CONTRACT-DELTA.md) 四 | 未改 |
| `../modules/shot-package.md` | [`33`](33-CONTRACT-DELTA.md) 五 | 未创建 |
| `../modules/storyboard-import.md` | [`33`](33-CONTRACT-DELTA.md) 六 | 未创建 |
| `Asset::Library` 索引 `origin/version/sourceMtime/sourceSize` | [`33`](33-CONTRACT-DELTA.md) 七 | 未写入 |
| `src/App/AppState.h` / `CommandDispatch*.cpp` / `ViewStateBuilder.cpp` | [`31`](31-CODE-REVIEW.md) CR-10 | 未创建 |
| `.github/workflows/release.yml` | [`34`](34-LANDING-CHECKLIST.md) FND-09 | 已创建 |

## 本版已确认决策

| 决策 | 结论 | 批准者 |
|------|------|--------|
| macOS CI job | 暂停，待有 Mac 再开 | Wisdom 2026-09-13 |
| Skill 的角色 | DirectorDesk 只**消费产出**与**分发**，不运行 | Wisdom |
| Skill 的分发方式 | 作为官方资产（`format: skill`），复用 `OfficialCatalog` | Wisdom |
| 导入格式 | DirectorDesk 自有 `storyboard-import 1`，不追随上游 schema | Wisdom |
| 镜头元数据存放 | 剧本 Markdown 引用块，不进 `.ddproj` | Wisdom |
| 对下游 AI 的接口 | 镜头包（PNG + `.shot.json`），无深度图 | Wisdom |
| `Application.cpp` | 允许 App 内分文件，不新建模块 | Wisdom |
| 角色骨骼 | 0.4 只静态显示 + manifest 字段；姿态用预烘焙 GLB | Wisdom |
| `.ddproj` 格式版本 | 本版保持 1 | Wisdom |
| Gizmo / Undo / PDF | 待批准 | — |

## 已知风险

| 风险 | 应对 |
|------|------|
| 分文件时 lambda 捕获的隐式状态漏迁，行为悄悄变 | FND-10 先写 Dispatch 测试锁定现有语义，再搬 |
| 异步加载与「打开另一个工程」竞态 | 结果队列带 `projectGeneration`，过期结果丢弃 |
| 共享 `gpuModelId` 的引用计数出错 | Scene 不知道 GPU；App 维护 `modelId → refCount` 表并加测试 |
| `windows-2022` 镜像将来也下线 | 同时记录 Ninja + msvc-dev-cmd 方案作为备份 |
| 官方清单切 `schemaVersion 2` 后 0.1.x 用户整体拒绝清单 | 0.4 Release note 写明；或官方仓库并行保留 `manifest.json`（v1）与 `manifest.v2.json` 一个版本周期 |
| 元数据引用块与用户原有正文里的引用块混淆 | 只识别紧跟标题、空行前的连续 `> k: v` 行；正文里的引用块保持不变 |

## 工作日志

### 2026-09-13：暂停云上 macOS CI

- `ci.yml` 去掉 macOS 矩阵项；Windows 仍是门禁。

### 2026-09-13：CI macOS FileDialog 弃用

- `FileDialogMac.mm` 改 `allowedContentTypes` + `UniformTypeIdentifiers`，消掉 `-Werror` 下的弃用错误。metal shader 已通过。

### 2026-09-13：F0 FND-02–09 止血代码

- shader 按平台裁剪；Ctrl+E / 删索引 / 视口 resize / SHA-256；Issue/PR 模板；tag 触发草稿 Release。
- README 英文摘要与 30 秒上手已写，截图 / GIF 未补。

### 2026-09-13：FND-01 Windows CI 生成器

- `.github/workflows/ci.yml` 的 Windows 矩阵从 `windows-latest` 改为 `windows-2022`，与 `CMakePresets.json` 的 `Visual Studio 17 2022` 对齐（CR-01 方案 a）。
- 未改 `ci-windows` 预设，未引入 Ninja / msvc-dev-cmd。

### 2026-09-13：FOUNDATION 设计集落地

- 新增 `docs/dev-map/foundation-upgrade/`（README + `30`–`36`）：同类项目调研与差距清单、逐文件代码审查、Skills 三层融合方案、契约增量（8 个新 Command、1 个改字段、快照字段、`script-format 1.1`、`asset-manifest 2`、`shot-package 1`、`storyboard-import 1`）、五波任务表、停手清单与状态页。
- 明确改写四条旧决策（App 内分文件、统一 SHA-256、剧本元数据、清单 `skill`），记录于 [`35`](35-DO-NOT.md) 第四节。
- 仅文档改动：未改业务代码、未新建 CMake 模块、AI 仍冻结。
