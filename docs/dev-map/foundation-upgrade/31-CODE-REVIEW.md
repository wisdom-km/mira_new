# 31 - 代码审查（CODE REVIEW）

> 2026-09-13 对 `v0.1.2` 全仓审查的逐条结论。行号以该 tag 为准，后续漂移以符号名为锚。
> 每条格式固定：**现状 / 问题 / 改法 / 验收**，并给出对应任务 ID。改法只说「落在哪个名词、契约是什么」，不贴实现代码——实现由执行模型按 `../05` 编码规范完成。
> 审查覆盖：`src/App`、`src/UI`、`src/Script`、`src/Asset`、`src/Export`、`src/Storyboard`、`include/DirectorDesk/**`、`cmake/`、`.github/workflows/`、`packaging/`。

## 一、构建与 CI

### CR-01 · Windows CI 生成器失效 → FND-01

- **现状**：`.github/workflows/ci.yml` 用 `windows-latest`；`CMakePresets.json` 的 `ci-windows` 写死 `"generator": "Visual Studio 17 2022"`。
- **问题**：`windows-latest` 已滚到不含 VS 2022 的镜像，configure 直接失败。
- **改法**：二选一，推荐第一种——（a）`ci.yml` 矩阵改 `os: windows-2022`；（b）`ci-windows` 预设改 `Ninja` + 在 job 里 `ilammy/msvc-dev-cmd@v1` 进入 MSVC 环境。不要同时做两种。
- **验收**：Actions Windows job 全绿；`ctest --preset ci-windows` 通过。

### CR-02 · macOS CI 对所有后端编 shader → FND-02

- **现状**：`cmake/ShaderCompile.cmake` 的 `_backends` 列表固定四项 `dx11 / metal / glsl / spirv`，对每个 shader 都调 `shaderc`。
- **问题**：mac 上的 `shaderc` 编 `s_5_0` HLSL 失败，整条 build 红。Windows 上则白白编 metal/glsl。
- **改法**：按平台裁剪：`if(WIN32)` 保留 `dx11`（可选加 `spirv` 备份）；`if(APPLE)` 只保留 `metal`；其他平台 `glsl + spirv`。`BgfxRenderer` 运行时选后端的逻辑不变，只是产物少了。
- **验收**：macOS job 全绿；Windows 产物目录不再出现 `metal/`。

### CR-03 · Release 打包不可复现 → FND-09

- **现状**：`packaging/windows/build-installer.ps1` 在作者本机跑，产物手工上传。
- **问题**：贡献者无法验证安装包如何生成；版本号写在多处（`CMakeLists.txt`、Inno 脚本、`03`）。
- **改法**：新增 `.github/workflows/release.yml`，`on: push: tags: ['v*']`，Windows job 调用现有 `build-installer.ps1` 并 `softprops/action-gh-release` 上传 `.exe/.zip`；版本号只从 `CMakeLists.txt` 的 `project(... VERSION x.y.z)` 读取，脚本用 `cmake -P` 或正则取出。不改 Inno 脚本结构。
- **验收**：推一个测试 tag `v0.1.3-rc1` 到 fork 或草稿 Release 能拿到产物；`03` 的发布步骤从「本机执行」改为「打 tag」。

## 二、`src/App/Application.cpp`

### CR-10 · `Run()` 1200+ 行、状态全是局部变量 → FND-10

- **现状**：`Application::Run(int, char**)` 从 643 行到文件末尾（文件共 1905 行）。工程脏标记、选择、导出分辩率、导出日志环形缓冲、缩略图调度、官方目录状态……全部是 `Run()` 的局部变量，通过 lambda 捕获给 visit 用。
- **问题**：（1）任何 Command 的语义都无法不开窗口单测；（2）新增一条 visit 分支要在 1000 行的 lambda 里找位置；（3）`AppViewState` 组装与命令分发耦在同一作用域，改一处必须通读全文。
- **改法（App 内分文件，不新建模块，`../08` 第 110 行已允许「痛了再在 App 内分文件」）**：
  1. `include/DirectorDesk/App/AppState.h`（App 私有头，放 `src/App/` 亦可）：把 `Run()` 里所有跨帧存活的局部变量收进一个 `struct AppState`。领域对象（`Script::Document`、`Scene::Document`、`Camera::CameraManager`、`Link::ShotLink`、`Storyboard::Document`、`Asset::Library`、`Asset::OfficialCatalog`）作为成员；派生状态（`projectDirty`、`selection*`、`exportResolutionId`、`exportLog`、`workspaceModeId`、`status`）作为字段。
  2. `src/App/CommandDispatch.cpp` + `CommandDispatch.h`：`void Dispatch(AppState&, const Core::Command&, DispatchServices&)`。`DispatchServices` 只装 Dispatch 需要的**接口引用**：`Renderer::IRenderer&`、文件对话框回调、`NowMs` 回调、后台任务提交回调。visit 链原样搬过去，按 Command 家族切成 `DispatchProject.cpp` / `DispatchScript.cpp` / `DispatchScene.cpp` / `DispatchCamera.cpp` / `DispatchLibrary.cpp` / `DispatchExport.cpp` / `DispatchWorkspace.cpp`，每个文件一个 `bool TryDispatchXxx(...)`，总入口按顺序试。
  3. `src/App/ViewStateBuilder.cpp`：`void BuildViewState(const AppState&, AppViewState&, FrameStrings&)`。`FrameStrings` 持有当帧 `std::string`，保证 `const char*` 生命周期规则不变（`../ui-pro-upgrade/23` 第二节）。
  4. `Application.cpp` 只剩：初始化、主循环、帧序（`RenderScene → RequestReadback → BeginFrame → Draw → Submit → EndFrame`）、退出。
  5. 全部文件加进 `src/App/CMakeLists.txt` 的 `dd_app` 目标。**不新增 CMake 目标。**
- **验收**：`Application.cpp` ≤ 400 行；`tests/App/` 新增至少 6 个用 `AppState + Dispatch` 直接跑 Command 的 Catch2 用例（`DeleteShot`、`BindShotToNewCamera`、`SelectExportResolution` 的回落、`RemoveLibraryAsset` 不标脏、`SetWorkspaceMode` 未知 ID、`InsertShot` 位置）；现有 109 个测试仍全绿；软件行为零变化。

### CR-11 · 打开工程同步加载模型 → FND-11

- **现状**：`UploadSceneModels`（`Application.cpp:336`）在 `OpenProjectFromPath` 分支里对每个节点同步 `registry.Load` + `renderer.CreateModel`。
- **问题**：十几个 GLB 就是数秒白屏；用户以为软件死了。
- **改法**：拆成两段。后台线程只做 `registry.Load(sourcePath) → Asset::ModelData`（纯值对象，允许跨线程，`../01` 第五节）；主线程每帧从结果队列取出 ≤ N 个做 `renderer.CreateModel` 并写回 `node->gpuModelId`。未就绪的节点先挂 `MakePlaceholderBox()`。在 `AppState` 加 `pendingModelLoads` 计数，经新快照字段 `sceneLoadPending` 送到状态栏（[`33`](33-CONTRACT-DELTA.md) 二）。
- **验收**：打开 `examples/cafe.ddproj` 时视口至少每 33ms 刷一帧；状态栏显示「加载模型 3/7」；全部完成后计数归零。加载失败的节点仍标 `assetMissing`。

### CR-12 · 保存工程逐文件同步 SHA-256 → FND-12

- **现状**：`ProjectBinding.cpp:89/103` 在 `CaptureProject` 时对每个本地模型调用 `ProjectFile::Sha256File`。
- **问题**：数百 MB 模型时 `Ctrl+S` 卡数秒；且 SHA 每次保存都重算，即使文件没动。
- **改法**：（1）`Asset::Library` 的索引记录已经含 `sha256`（下载与导入时算过）；`CaptureProject` 优先取索引里的哈希，只在索引缺失或 `mtime/size` 变化时才重算；（2）重算放到后台任务，返回 `{path, sha256}` 值对象，主线程写回索引后再真正落盘 `.ddproj`。保存期间状态栏显示「正在校验资产」；再次 `Ctrl+S` 排队不叠加。
- **验收**：第二次保存同一工程时不读模型文件（用日志或计数断言）；`tests/App/ProjectBinding` 加「索引有哈希则不重算」用例。

### CR-13 · `RemoveLibraryAssetCommand` 误标脏 → FND-04

- **现状**：visit 分支里 `projectDirty = true`（约 1285 行）。
- **问题**：资源库索引是用户级数据，不是工程数据（`modules/project-file.md` 第六节列的脏条件里没有它）。删一条索引后关闭软件会被问「未保存的工程」。
- **改法**：删掉那一行。若该资产正被场景节点引用，节点本来就有 `libraryAssetId`，不受影响（下次打开按缺失处理）。
- **验收**：新建工程 → 删一条资源库记录 → 关闭软件不弹保存提示。

### CR-14 · 导出快捷键忽略已选分辨率 → FND-03

- **现状**：`WorkspacePanel.cpp:868` `Ctrl+E` 推 `ExportCurrentShotCommand{"1080p"}`；菜单 675/678 行分别写死 `"1080p"/"2k"`。visit 已支持 `resolutionId` 为空时回落到 `exportResolutionId`（`Application.cpp:1445`）。
- **问题**：UIP-15 做了「分辩率是持续选择」，快捷键没跟上。
- **改法**：`Ctrl+E` 推 `ExportCurrentShotCommand{}`（空 ID）。菜单保留两项作为快捷路径可以，但要加第三项「按当前选择导出」并置顶。
- **验收**：检查器选 `2k` → `Ctrl+E` → 导出记录显示 `2k`。

### CR-15 · 官方资产写进工程成了 `user-library` → FND-16

- **现状**：`ProjectBinding.cpp:99–114` 默认 `source = UserLibrary`，仅当「索引里找不到且 sourcePath 为空」才改 `Official`；官方资产下载后有缓存路径，所以永远走 `UserLibrary` 分支，`path` 写成绝对缓存路径。
- **问题**：与 `modules/project-file.md` 第三节「official 保存 assetId + version + entrypoint」不符；换机器打开工程，官方资产全部缺失，即使那台机器也下载过同一版本。
- **改法**：`Scene::Node` 已有 `libraryAssetId`；`Asset::Library` 记录需带 `origin`（官方 / 本地）与官方 `version`（下载时已知）。`CaptureProject` 看 `origin == Official` 就写 `official` 三元组；`HydrateProject` 对 `official` 走 `OfficialCatalog` 的缓存解析（`<用户数据目录>/DirectorDesk/assets/official/<id>/<version>/<entrypoint>`），未下载则标缺失并在状态栏提示「可在资源库下载」。**不改 `.ddproj` 格式版本**——格式本来就定义了这条，是代码没实现。
- **验收**：`tests/App/ProjectFile` 加官方资产 round-trip；手工：下载一个官方资产 → 保存 → 用文本编辑器看 `.ddproj` 里是 `"source": "official"`。

### CR-16 · `ViewportResizeCommand` 每帧推送 → FND-05

- **现状**：`WorkspacePanel.cpp:939` 的 `commands.Push(ViewportResizeCommand{...})` 在 2px 阈值 `if` 块**之外**。
- **问题**：每帧一条无效命令进队列；visit 里靠比较旧尺寸才没重建 RT，属于两头兜底。
- **改法**：把 `Push` 移进 `if`，并在首帧（`m_lastViewportW == 0`）保证推一次。visit 侧保留尺寸相等即返回的保护。
- **验收**：加一个只在 Debug 生效的命令计数断言或日志：静止时每秒 `ViewportResizeCommand` 数为 0。

### CR-17 · 两份 SHA-256 → FND-06

- **现状**：`Core::Sha256Hex`（`src/Core/Sha256.cpp`，基于 picosha2）与 `ProjectFile.cpp:142–200` 手写实现并存。
- **改法**：`ProjectFile::Sha256File` 改为读文件后调用 `Core::Sha256Hex`；删掉手写块。`03` 决策表「工程文件仍用既有内置哈希」一行改为「统一 `Core::Sha256`」。
- **验收**：`tests/App/ProjectFile` 现有哈希断言不变仍通过（两实现输出必须一致，否则说明其中一个有 bug，先查再删）。

## 三、`src/UI`

### CR-20 · 用字符串判业务状态 → FND-17

- **现状**：`WorkspacePanel.cpp:42` 读 `selectionKind` 字符串；`:55` 与 `StoryboardPanel.cpp:95/286/312/350` 比较 `card.kind == "shot"`；`:350/919` 用 `"未关联"` 作为占位文案同时也是判断依据。
- **问题**：改一处中文文案就悄悄改了逻辑；`../07` 第六节「面板里推断业务状态」的边缘案例。
- **改法**：`IPanel.h` 加 `enum class SelectionKind { None, Shot, Node, Camera }` 与 `enum class CardKind { Scene, Shot }`，快照同时保留原字符串字段一版以便过渡（F1 末删除字符串版）。`linkedCameraName` 为空的判断改用已有 `selectedShotLinkedCamera`/`link` 布尔字段，不比较文案。
- **验收**：`rg '== "shot"|"未关联"' src/UI` 只剩显示用途；编译期枚举 switch 覆盖全部值（`-Wswitch`）。

### CR-21 · 导出记录按标题回点镜头 → FND-18

- **现状**：`StoryboardPanel.cpp:95` 用 `card.title == entry.shotTitle` 找卡片。
- **改法**：`ExportLogView` 加 `shotId`（[`33`](33-CONTRACT-DELTA.md) 二），面板按 ID 匹配。
- **验收**：两个同名镜头分别导出后，点击导出记录能分别定位。

### CR-22 · 检查器变换只有 `DragFloat3` → FND-41（待批准）

- **现状**：`DrawNodeInspector` 用三组 `DragFloat3`；`03` 决策「不引入 ImGuizmo」。
- **问题**：置景效率低是用户反馈里最常见的一条；但这是产品决策不是 bug。
- **改法**：F4 议题。若批准，ImGuizmo 作为 vcpkg 依赖只出现在 `src/UI/*.cpp`，公共头不出现其类型；Gizmo 输出仍走 `SetNodeTransformCommand`。若不批准，改良方案是给 `DragFloat3` 加「对齐地面 / 面向相机 / 复位」三个按钮（这三条不需要批准，放进 FND-13）。

## 四、`src/Script`

### CR-30 · `RemoveShotSection` 文本扫描 → FND-15

- **现状**：`Document.cpp:58` 找 `### [shot:id]` 标记，向下扫到下一个以 `##` 开头的行为止。
- **问题**：（1）`IsMarkdownHeading` 只看前两个字符，`####` 四级标题会被当作终止符，导致镜头正文被截断；（2）代码围栏里的 `## ` 会提前终止；（3）与 `Parser` 已经算好的行号范围重复实现了一套规则。
- **改法**：`Parser` 的快照给每个 Scene/Shot 记 `lineStart/lineEnd`（若尚无则加）。`RemoveShot`/`InsertShot` 都通过快照的行号区间操作文本，Document 不再自己识别标题。
- **验收**：`tests/Script` 加三条：含 `####` 小标题的镜头整段删除；代码围栏内 `## ` 不终止；CRLF 文本删除后行尾风格不变。

### CR-31 · `InsertShot()` 只能追加 → FND-14

- **现状**：`Document::InsertShot()` 无参数，把新镜头写到文档末尾（或当前场次末尾）。
- **改法**：`InsertShotCommand` 加字段 `afterShotId`（默认空 = 旧行为，兼容）。`Document::InsertShot(const std::string& afterShotId)`：为空时沿用；否则用 CR-30 的行号区间在该镜头 `lineEnd` 之后插入。层级面板右键「在此后插入镜头」。
- **验收**：三镜头剧本，在第一镜后插入 → 解析顺序为 1、新、2、3；`.ddproj` 关联不受影响。

### CR-32 · 剧本无结构化字段 → FND-20

- 见 [`33`](33-CONTRACT-DELTA.md) 三「`script-format` 1.1」。Parser 只多收集紧跟镜头标题的 `> key: value` 引用块行；`Shot` 快照加 `meta`（有序键值列表）。旧剧本零变化。

## 五、`src/Asset`

### CR-40 · 官方清单 `format` 只允许 `glb|obj` → FND-32

- 为分发 Skill 包与角色包，`asset-manifest` 升到 2：`format` 增加 `skill`；Asset 增加可选字段 `kind`（`model | skill | character`）与 `rig`。见 [`33`](33-CONTRACT-DELTA.md) 四。`OfficialCatalog` 的下载、校验、缓存流程**一行不改**；只是 `entrypoint` 对 `skill` 是 `SKILL.md`。

### CR-41 · `Asset::Library` 记录缺 `origin/version` → FND-16 前置

- 见 CR-15。索引 JSON 加两个可选字段，旧索引缺失时按「本地」处理。

## 六、`src/Export`

### CR-50 · 只出 PNG → FND-21

- 新增 `Export::WriteShotPackage(const ShotPackageInput&, const std::string& dir) -> Core::Result<ShotPackageOutput>`：写 `<shot-id>.png` + `<shot-id>.shot.json`。相机 FOV → 35mm 等效焦距换算在 Export 内做（纯数学，`focal = 12 / tan(vfov/2)` 以 24mm 画幅高为基准），**不改 `Camera::OrbitCamera`**。格式见 [`33`](33-CONTRACT-DELTA.md) 五。
- Export 目前不 include Script / Scene；镜头包需要剧本正文与场景节点列表 → 由 **App** 组装成 `ShotPackageInput` 值对象传入，Export 不新增对领域模块的依赖。

## 七、`src/Storyboard`

### CR-60 · 卡片只有标题 → FND-22

- `Storyboard::CardModel` 加 `metaLine`（App 从 `Shot.meta` 拼「中景 · 推 · 3s」）；`BoardComposer` 在标题下画一行小字。Storyboard 仍不 include Script。

## 八、测试与文档

| 项 | 现状 | 改法 | 任务 |
|----|------|------|------|
| App 层无 Command 语义测试 | 109 cases 全在领域模块 | CR-10 完成后补 `tests/App/Dispatch*.cpp` | FND-10 |
| `docs/RELEASE-CHECKLIST.md` 假设本机打包 | — | 改为 tag 触发 | FND-09 |
| `README.md` 中文单语 | — | 顶部英文摘要 + 截图 + 30 秒上手 + 徽章 | FND-07 |
| 无 `.github/ISSUE_TEMPLATE`、`PULL_REQUEST_TEMPLATE.md` | — | Bug / Feature / Asset 三个模板；PR 模板要求勾「未新建模块 / AI 仍冻结 / 更新了 03」 | FND-08 |
