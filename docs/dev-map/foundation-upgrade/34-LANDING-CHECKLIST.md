# 34 - 落点清单（LANDING CHECKLIST）

> 每一行是一次**可独立完成、可独立回退**的改动。任务 ID 稳定，可以直接对执行模型说「做 FND-11」。
> 落点顺序不变（`../07` 第四节）：Panel → `Core::Command` → `App` visit → App 组装快照 → 领域测试 → 更新 [`36`](36-FOUNDATION-STATUS.md) 与 `../03`。
> 契约先于代码：涉及第三至六节格式的任务，第一步都是把 [`33`](33-CONTRACT-DELTA.md) 对应节抄进 `../modules/`。
> 进度只在 [`36`](36-FOUNDATION-STATUS.md) 记录。

## 波次

| 波次 | 目标版本 | 进入条件 | 完成条件 |
|------|----------|----------|----------|
| **F0 · 止血** | 0.1.3 | 立即 | Windows CI 绿（macOS job 已暂停）；F0 全部任务完成；打 `v0.1.3` |
| **F1 · 地基** | 0.2 | F0 完成 | `Application.cpp` ≤ 400 行；App 层有 Command 测试；打开 / 保存不卡 UI；打 `v0.2.0` |
| **F2 · 镜头包** | 0.3 | F1 完成 | 一个镜头能导出 PNG + `.shot.json`，被外部脚本读取成功 |
| **F3 · Skills 融合** | 0.4 | F2 完成 | 一份 `storyboard-import.json` 导入后能在四模式里走完并导出镜头包；一个 Skill 能从资源库下载 |
| **F4 · 角色包与体验** | 0.5 | Wisdom 逐项批准 | 见各任务 |

## F0 · 止血（0.1.3，不改契约）

| ID | 名词 / 落点 | 改动 | 依据 | 完成标准 |
|----|-------------|------|------|----------|
| FND-01 | `.github/workflows/ci.yml` | Windows 矩阵改 `windows-2022`（或 Ninja + msvc-dev-cmd，二选一） | CR-01 | Windows job 绿 |
| FND-02 | `cmake/ShaderCompile.cmake` | `_backends` 按 `WIN32` / `APPLE` / 其他裁剪 | CR-02 | macOS job 绿；Windows 产物无 `metal/` |
| FND-03 | `src/UI/WorkspacePanel.cpp` | `Ctrl+E` 与菜单改为空 `resolutionId`，菜单加「按当前选择导出」 | CR-14 | 选 `2k` 后 `Ctrl+E` 导出 `2k` |
| FND-04 | `Application.cpp` visit `RemoveLibraryAssetCommand` | 去掉 `projectDirty = true` | CR-13 | 删索引后关闭不弹保存提示 |
| FND-05 | `src/UI/WorkspacePanel.cpp` | `ViewportResizeCommand` 只在阈值触发或首帧推送 | CR-16 | 静止时每秒 0 条 |
| FND-06 | `src/App/ProjectFile.cpp` | 删手写 SHA-256，改调 `Core::Sha256Hex`；`03` 决策表同步 | CR-17 | 现有哈希断言不变 |
| FND-07 | `README.md`、`docs/` | 顶部英文摘要；两张截图 + 一段 GIF（四模式各一帧、导出流程）；「30 秒上手」；CI / Release 徽章 | A2 | 陌生人读 README 能在不看其他文档时装好并打开示例 |
| FND-08 | `.github/ISSUE_TEMPLATE/*.yml`、`PULL_REQUEST_TEMPLATE.md` | Bug / Feature / Asset 三模板；PR 模板勾选项：未新建模块、AI 仍冻结、更新 `03`、测试全绿 | A3 | 新建 issue 时出现模板选择 |
| FND-09 | `.github/workflows/release.yml`、`packaging/windows/build-installer.ps1`、`docs/RELEASE-CHECKLIST.md` | tag `v*` 触发打包并上传；版本号只从 `CMakeLists.txt` 读 | CR-03 | 草稿 Release 能拿到 `.exe/.zip` |

## F1 · 地基（0.2）

| ID | 名词 / 落点 | 改动 | 依据 | 完成标准 |
|----|-------------|------|------|----------|
| FND-10 | `src/App/`（`AppState.h`、`CommandDispatch*.cpp`、`ViewStateBuilder.cpp`）+ `tests/App/` | App 内分文件；不新建 CMake 目标 | CR-10 | `Application.cpp` ≤ 400 行；≥ 6 个 Dispatch 测试；行为零变化；109 旧测试全绿 |
| FND-11 | `src/App/`（模型加载）、`AppViewState.sceneLoadPending/Total`、状态栏 | 后台线程 `registry.Load` 回值对象；主线程每帧上传 ≤ N 个；占位盒先挂 | CR-11 | 打开示例工程期间视口持续刷帧；状态栏显示 `x/y` |
| FND-12 | `src/App/ProjectBinding.cpp`、`Asset::Library` 索引字段、`AppViewState.projectSaveInProgress` | 哈希优先取索引；变化才后台重算；保存期间状态栏提示 | CR-12 | 二次保存不读模型文件；测试「索引有哈希则不重算」 |
| FND-13 | `Command.h`（`DeleteNode` / `DuplicateNode` / `SetNodeVisible`）、`Scene::Document`、层级面板右键、节点检查器 | 三个新 Command + visit；检查器加「对齐地面 / 面向相机 / 复位」按钮（不需批准） | B2、CR-22 | 右键能删 / 复制 / 显隐；隐藏节点不渲染不导出；`.ddproj` round-trip 保留 `visible` |
| FND-14 | `Command.h`（`InsertShotCommand.afterShotId`）、`Script::Document::InsertShot(after)`、层级面板右键 | 镜头可在选中镜头后插入 | CR-31 | 顺序测试通过；空 ID 旧行为不变 |
| FND-15 | `Script::Parser`（行号区间）、`Script::Document::RemoveShot` | 删除 / 插入改用解析器行号区间，去掉 `RemoveShotSection` 文本扫描 | CR-30 | `####` 小标题、代码围栏、CRLF 三条测试通过 |
| FND-16 | `Asset::Library`（`origin/version`）、`ProjectBinding` | 官方资产按 `.ddproj` 1 已定义的 `official` 三元组写 / 读 | CR-15 | 文本查看 `.ddproj` 见 `"source": "official"`；round-trip 测试 |
| FND-17 | `IPanel.h`（`SelectionKind` / `CardKind`）、`src/UI/*.cpp` | 枚举替代字符串判断；F1 末删字符串版 | CR-20 | `rg '== "shot"\|"未关联"' src/UI` 只剩显示用途 |
| FND-18 | `ExportLogView.shotId`、`StoryboardPanel.cpp` | 导出记录按 ID 回点 | CR-21 | 同名镜头分别定位 |

## F2 · 镜头包（0.3）

| ID | 名词 / 落点 | 改动 | 依据 | 完成标准 |
|----|-------------|------|------|----------|
| FND-20 | `../modules/script-format.md` → 1.1；`Script::Parser`（`Shot::meta`）；`Command.h`（`SetShotMetaCommand`）；镜头检查器「元数据」分节 | 引用块元数据解析与编辑；检查器给五个推荐键的快捷输入 | CR-32、[`33`](33-CONTRACT-DELTA.md) 三 | 六条解析测试通过；改一条元数据后剧本文本只变那一行 |
| FND-21 | 新建 `../modules/shot-package.md`；`Export::WriteShotPackage`；`Command.h`（`ExportShotPackageCommand`）；审片检查器与镜头右键「导出镜头包」 | PNG + `.shot.json`；焦距换算在 Export | CR-50、[`33`](33-CONTRACT-DELTA.md) 五 | `tests/Export` 用固定输入比对 JSON；外部 Python/Node 一行读出 `camera.focalLength35mmEquivalent` |
| FND-22 | `Storyboard::CardModel.metaLine`、`BoardComposer`、`StoryboardCardView.metaLine`、镜头条 | 卡片与镜头条显示「中景 · 推 · 3s」 | CR-60 | 总览 PNG 标题下多一行小字；无 meta 时不留空行 |
| FND-23 | `Export`（`board.json`） | 分镜总览导出时附带索引 JSON | [`33`](33-CONTRACT-DELTA.md) 五末 | 每格对应 `id/title/image/metaLine` |

## F3 · Skills 融合（0.4）

| ID | 名词 / 落点 | 改动 | 依据 | 完成标准 |
|----|-------------|------|------|----------|
| FND-30 | 新建 `../modules/storyboard-import.md`；`Script::ImportStoryboard`；`dd_script` 私有链接 nlohmann | JSON → `script-format 1.1` Markdown；`replace` / `append` | [`32`](32-SKILLS-INTEGRATION.md) L1、[`33`](33-CONTRACT-DELTA.md) 六 | 七条测试通过；生成文本再解析与 JSON 结构一致 |
| FND-31 | `Command.h`（`ImportStoryboard*`）、visit、编剧模式检查器、`AppViewState.importDiagnostics` | 导入入口 + 诊断显示；dirty 时先提示 | 同上 | 导入示例 JSON 后四模式可走完并导出镜头包 |
| FND-32 | `../modules/asset-manifest.md` → 2；`Asset::OfficialCatalog` 解析 `format: skill` / `kind` / `rig`；App 拒绝把 skill 拖进场景 | Skill 作为官方资产 | [`32`](32-SKILLS-INTEGRATION.md) L2、[`33`](33-CONTRACT-DELTA.md) 四 | `schemaVersion 1/2` 双测试；下载流程零改动 |
| FND-33 | 资源库「Skills」类别、检查器 Skill 面孔（`SKILL.md` 前 40 行、安装路径、打开文件夹） | UI | 同上 | 下载一个 Skill 后能看到路径并打开文件夹 |
| FND-34 | `docs/skills/README.md`、`tools/make-skill-manifest.(ps1\|py)` | Skill 作者三步接入指南 + 清单生成脚本 | [`32`](32-SKILLS-INTEGRATION.md) 五 | 用脚本给 `shuohao-skills` 的一个 Skill 生成合法清单条目 |
| FND-35 | `examples/` | 加一份 `storyboard-import.example.json` 与对应导入后的 `.ddproj` | — | 开始板「示例」可选它 |

## F4 · 角色包与体验（0.5，逐项待批准）

| ID | 名词 / 落点 | 改动 | 需批准的原因 | 完成标准 |
|----|-------------|------|--------------|----------|
| FND-40 | `asset-manifest 2`（`kind: character` / `rig`）、`Asset::ModelData.hasSkin`、cgltf 加载忽略 skin、资源库徽标 | 角色包静态加载 | 不需批准（manifest 2 已含字段）；列在此处只因依赖 Wisdom 的角色包仓库先存在 | 一个带骨骼 GLB 能下载、显示 bind pose、标「含骨骼」 |
| FND-41 | ImGuizmo（vcpkg）只在 `src/UI/*.cpp`；输出走 `SetNodeTransformCommand` | 视口 Gizmo | 改 `03` 决策「不引入 ImGuizmo」+ 新第三方依赖 | 拖 Gizmo 与 `DragFloat3` 数值一致；公共头无 ImGuizmo 类型 |
| FND-42 | `App`（命令快照栈，仅 Scene / Camera / Link 三个名词）、`Command.h`（`UndoCommand` / `RedoCommand`） | Undo / Redo | 改 `00` 第五节「P0 不做 Undo」 | 置景 20 步可逐步撤销；剧本文本不进栈（编辑器自带） |
| FND-43 | `Storyboard`、`Export` | 分镜总览导出 PDF（单文件、每页 N 格） | 需新第三方（或自写最小 PDF writer） | 导出 A4 横向 PDF |

## 每个任务的 Definition of Done

除 `../05` 第十一节通用 DoD，本版追加：

1. 「完成标准」列逐条可复现，写进 [`36`](36-FOUNDATION-STATUS.md) 验证记录。
2. 没有触碰 [`35-DO-NOT.md`](35-DO-NOT.md) 第三节硬约束；F3 任务额外确认「软件内没有运行任何外部程序」。
3. 涉及格式的任务，`../modules/*.md` 先于代码提交；解析器测试覆盖该文档「最小测试集」全部条目。
4. 新 Command 的语义在 `tests/App/` 有 Dispatch 测试（FND-10 之后强制）。
5. 现有测试全绿；Windows + macOS CI 全绿（FND-01/02 之后强制）。
6. 收工同步 [`36`](36-FOUNDATION-STATUS.md) 与 `../03-CURRENT-STATUS.md`；改了契约同步 `../modules/`。
