# 03 - 当前状态（CURRENT STATUS）

> 这是项目当前位置的唯一真实来源。每次工作开始先读，工作结束必须更新。

## 当前快照

- **当前 Phase**：Phase 10 已完成。**P0（Windows）验收 100%**；macOS 暂缓开发与验收（Wisdom 2026-09-15）
- **上一版本**：**UI-PRO（UI 专业化升级）** 已随 **0.1.2** 打包。见 [`ui-pro-upgrade/`](ui-pro-upgrade/README.md)
- **当前版本工作**：**FOUNDATION（地基、镜头包与 Skills 融合）**，F0 已随 **`v0.1.3`** 发布（含并行 UI-CLARITY）。见 [`foundation-upgrade/`](foundation-upgrade/README.md)，进度以 [`foundation-upgrade/36-FOUNDATION-STATUS.md`](foundation-upgrade/36-FOUNDATION-STATUS.md) 为准
- **最后更新**：2026-09-16
- **更新者**：Cursor AI
- **当前分支**：`main`
- **最近完成 tag**：`v0.1.3`
- **下一个允许执行的工作**：UI-CLARITY **UIC-58 待批准**（AI「测试连接」，需第 3 个 Command）。U3 其余（UIC-50～57 / 59 / 60 / 22 / 26 / 32）已写入。FOUNDATION FND-10～56 已齐。F1～F5 未打中间版本 tag。不得新建 CMake 模块；`Application.cpp` 允许在 App 目标内分文件（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）

## 已完成

- [x] Phase 0–10：从骨架到 P0 发布准备
- [x] tag `phase-10-p0`、`phase-10-p1`、`v0.1.2`、`v0.1.3`
- [x] Windows 安装包脚本：`packaging/windows/`

## 进行中

**FOUNDATION**：F0 已随 `v0.1.3` 发布。FND-10～FND-56 已完成（F5 AI 解冻 + 分镜 Skill 走文本模型）。本版代码行已齐。

**UI-CLARITY（界面重设计）**：U0 + U1/U2（含 UIC-22 / 26 / 32）+ U3 UIC-40～57 / 59 / 60 已写入。进度以 [`ui-clarity/46`](ui-clarity/46-UI-CLARITY-STATUS.md) 为准。下一允许行 **UIC-58 待批准**。

**UI-PRO 0.1.2**：W1–W3 已写入并打 Windows 安装包，已收口。

P0 路线图已完成；升级版之外的加删功能仍以 `07` 为准。

## 阻塞项

- **无 P0 阻塞项**。macOS 云测 / 实机暂缓，不挡验收（Wisdom 2026-09-15）；云上 macOS job 仍关
- **FOUNDATION**：F1～F5 任务 FND-10～56 已齐；未打中间版本 tag
- **UI-CLARITY**：U3 允许行已写入。下一允许行 **UIC-58 待批准**。Catch2 **223 / 1394**。未拍本轮四模式截图

## 已确认决策

| 决策 | 结论 |
|------|------|
| P0 平台 | Windows（P0 验收）；macOS 暂缓开发与验收（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）；批准者 Wisdom 2026-09-15 |
| macOS CI job | 暂停云上 macOS job，不挡 P0；待有 Mac 再开；批准者 Wisdom 2026-09-13 / 2026-09-15 |
| Linux | P0 不支持，后续演进 |
| 代码许可证 | MIT |
| 官方资产许可证 | 优先 CC0，允许明确标注的 CC-BY |
| C++ 标准 | C++17 |
| 构建与依赖 | CMake + vcpkg manifest |
| 渲染/UI/窗口 | bgfx / Dear ImGui docking / GLFW |
| ImGui 后端 | GLFW + bgfx |
| 窗口图形 API | `GLFW_NO_API`，由 bgfx 创建交换链 |
| 模型加载 | cgltf（GLB）+ tinyobjloader（OBJ），通过 `IModelLoader` 扩展 |
| 网络与 JSON | libcurl + nlohmann-json |
| 完整性校验 | picosha2（SHA-256）；工程文件统一改用 `Core::Sha256`（FOUNDATION 版改写，见 foundation-upgrade/35 第四节；任务 FND-06） |
| 测试 | Catch2 v3 |
| 开发顺序 | 先走通含分镜画布的本地资产核心闭环，再实现在线资产库 |
| 项目持久化 | 版本化 JSON `.ddproj` |
| 分镜画布 | 剧本是 Scene/Shot 结构的唯一来源；自动布局、不可自由连线 |
| 分镜导出 | 支持单镜头参考图和完整分镜总览 PNG |
| vcpkg baseline | `c5a15727ee70fddf0296f0d8aafc3f58916fefac` |
| Phase 2 变换入口 | 检查器 DragFloat3 + 视口 ImGuizmo（FND-41）；输出 `SetNodeTransformCommand`（FOUNDATION 版改写，见 foundation-upgrade/35 第四节） |
| 外部 .gltf + 分离 bin/uri | P0 只保证 `.glb` |
| 地面格网 | 视口绘制、导出不含；批准者 Wisdom |
| 模型线框叠加 | P0 不做 |
| 预设机位朝向约定 | 物体默认朝 +Z；无选中对象时目标为 `(0, 0.5, 0)`、半径 `1` |
| 本地资源预览 | sidecar PNG 或纯色占位 |
| 镜头 3D 缩略图 | Phase 7 由 App 主线程离屏渲染 |
| 官方在线源 | 编译期固定唯一地址；无自定义源、无上传 |
| 官方地址 | 仓库 https://github.com/wisdom-km/obj-3d-models ；清单 `https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/manifest.json` ；资源基地址 `https://raw.githubusercontent.com/wisdom-km/obj-3d-models/main/` ；批准者 Wisdom |
| AI 接口 | 供应商无关；OpenAI 兼容 HTTPS（F5）；密钥在 `settings.json`；P0 曾无真实调用（FOUNDATION 版改写，见 foundation-upgrade/35 第四节） |
| Windows 分发 | Inno Setup 安装包，发布到 GitHub Releases；批准者 Wisdom |
| 应用 logo | 暂定 `img/dog.png`；Windows 图标为 `img/directordesk.ico`；批准者 Wisdom |
| Demo 迭代策略 | 部分模块化：控制面保留，模块数锁定；AI 已解冻为空岛上的 HTTPS 适配器（F5）；加删按 `07` 落点；原理见 `08`；`Application.cpp` 允许在 App 目标内分文件、不新建模块（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）；批准者 Wisdom |
| Skills 融合 | L1 消费 `storyboard-import`；L2 分发 `format: skill`；L3 可按 `skill.json` 运行（F5）；见 `foundation-upgrade/32`；批准者 Wisdom |
| 镜头元数据与镜头包 | 元数据以剧本引用块表达（`script-format 1.1`）；对下游 AI 的接口是镜头包 PNG + `.shot.json`；`.ddproj` 保持版本 1；批准者 Wisdom |
| 当前画面镜头与关联 | 镜头检查器可选已有相机或新建机位；相机面孔不把当前画面镜头列入占用关联；删镜头/删相机都在左侧右键；批准者 Wisdom |
| 资源库缺失条目 | 网格不显示；可删索引记录，不删磁盘源文件；批准者 Wisdom |
| UI-CLARITY Q1 取景框 | 视口默认锁导出比例，视图菜单可关；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q2 字号 | 默认 14，跟随 DPI；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q3 图标 | Lucide 子集 TTF，许可证 **ISC**，文件 `assets/fonts/lucide-dd.ttf`（说明见同目录 `README.md`；部分字形源自 Feather MIT）；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q4 色板 | 中性灰，只留橙色强调；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q5 3D 背景 | 视口与非透明导出改中性灰，透明导出不变；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q6 分镜 | 先做短期 UIC-14；长期 UIC-32 等 F2 之后；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q7 编剧底栏 | 隐藏，不留空槽；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q8 灯光 | 搬到检查器「场景」面孔；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q9 Command | 本版上限 2：`SelectAdjacentShotCommand`、`RevealPathCommand`；批准者 Wisdom 2026-09-13 |
| UI-CLARITY Q10 用户设置 | 用户目录 `settings.json`（`Paths::UserSettingsFile`），**不进** `.ddproj`；批准者 Wisdom 2026-09-13 |

## 已知风险

| 风险 | 应对 | 验证阶段 |
|------|------|----------|
| bgfx shader 跨平台编译复杂 | CMake 自动调用 shaderc | Phase 1 Windows 已验证 |
| 透明离屏渲染/回读 | Phase 1 技术切片 | Phase 1/7 Windows 已通过；macOS 暂缓 |
| Windows 中文路径 | Platform UTF-8 边界 | Phase 0–7、10 已测 |
| 大型剧本卡顿 | 防抖、可见区、缓存上限 | Phase 7 |
| raw.githubusercontent.com 在部分网络环境不可用 | 清单刷新失败时使用最后有效缓存 | Phase 8 |
| 源文件移动后 ID 变化 | 稳定路径键 | Phase 5 |

## 本次验证

- 2026-09-16：UIC-49 设置对话框四页；检查器只留生成按钮 + 「打开设置」。Catch2 **220 / 1369**。未 commit
- 2026-09-16 凌晨：UIC-48 格尺寸 / 网格线 / 栏高随缩放。2560 125% 镜头条格约 260×146。Catch2 **218 / 1346**。未 commit
- 2026-09-15 夜：UIC-47 界面缩放。首启 2560 显示器默认 125%；`settings.json` `uiScale`；切换不重启。Catch2 **218 / 1346**。未 commit
- 2026-09-15 夜：UIC-46 左右栏像素上下限。Catch2 **215 / 1327**。未 commit
- 2026-09-15 夜：UIC-45 资源库默认网格。Catch2 **215 / 1327**。未 commit
- 2026-09-15 夜：UIC-44 文件名 / 镜头条编号 / 审片无记录不占底栏 / 示例按钮只在空状态。Catch2 **215 / 1327**。未 commit
- 2026-09-15 夜：UIC-43 `projectIsEmpty` 由 App 给。无工程出开始板。Catch2 **215 / 1327**。未 commit
- 2026-09-15 夜：UIC-42 元数据两列表格，五个标签完整可见。Catch2 **214 / 1319**。未 commit
- 2026-09-15 夜：UIC-41 掌机「导出镜头」改 Lucide chevron-down，`rg "▾" src/UI` 空。Catch2 **214 / 1319**。未 commit
- 2026-09-15 夜：UIC-40 场景行 `Selectable` 不再用负宽。2560 宽掌机「立方体 / 桌子 / 杯子」可见、眼睛在右。Catch2 **214 / 1319**。未 commit
- 2026-09-15 夜：FND-56 分镜 Skill 可走检查器文本模型（`openai-compat-json` / `/v1/chat/completions`）。Catch2 **214 / 1319**。需重新安装示例 Skill。未 commit
- 2026-09-15 晚实机：打开 `cafe.ddproj`，编剧/置景/掌机/审片可切；安装并运行示例 Skill 后剧本为 **4 场 9 镜**（导入诊断来源 `directordesk/example-storyboard-skill`）；切镜机位会跳。官方清单仍下载失败。检查器 AI 区可见（无密钥则生成禁用）。未 commit
- FND-50～55：Wisdom 批准解冻 AI。OpenAI 兼容 HTTPS、`settings.json` 密钥、检查器生成图像/视频、示例 Skill 可安装并「运行」导入 JSON。Catch2 **209 cases / 1278 assertions**。无新 CMake 模块。未 commit

## 工作日志

### 2026-09-16：UIC-49 设置对话框

- 四页模态（常规 / 视口 / 导出 / AI）；AI 配置移出检查器。Catch2 **220 / 1369**。未 commit。下一行 UIC-50。

### 2026-09-16：UIC-48 格尺寸 / 网格 / 栏高

- 设计像素 × `UiScale()`；≥1920 镜头条格 208×117。网格主线提亮并随缩放加宽。Catch2 **218 / 1346**。未 commit。下一行 UIC-49。

### 2026-09-15：UIC-47 界面缩放

- 视图菜单 100/125/150/200；`settings.json` `uiScale`；宽屏首启默认 125%。Catch2 **218 / 1346**。未 commit。下一行 UIC-48。

### 2026-09-15：UIC-46 面板像素宽

- 左右栏改像素上下限（左 300–420 / 右 340–480）。Catch2 **215 / 1327**。未 commit。下一行 UIC-47。

### 2026-09-15：UIC-45 资源库默认网格

- `libraryViewMode` 默认 `grid`。Catch2 **215 / 1327**。未 commit。

### 2026-09-15：UIC-44 四条小改

- 剧本只显示文件名；镜头条「1.1 过肩」+ 三点；审片无记录不占底栏；示例按钮只在空状态。Catch2 **215 / 1327**。未 commit。

### 2026-09-15：UIC-43 projectIsEmpty

- 快照新字段由 App 填。无工程出开始板；「打开剧本」未误勾。Catch2 **215 / 1327**。未 commit。

### 2026-09-15：UIC-42 元数据两列表格

- 检查器元数据左列标签、右列 `##key` 输入。Catch2 **214 / 1319**。未 commit。下一行 UIC-43。

### 2026-09-15：UIC-41 主按钮箭头

- 字面 `▾` 改为 `Icon::ChevronDown`；切镜帮助与 tooltip 的 `↑↓` 改为图标。`rg "▾" src/UI` 空。Catch2 **214 / 1319**。未 commit。下一行 UIC-42。

### 2026-09-15（晚）：功能 / 设置分界，Skills 进设置（文档）

- Wisdom 提出「API 应在设置、Skills 也应在设置」。`ui-clarity/47` 增 5.1 三问归位规则与归位表；设置对话框改五页（+Skills）；编剧模式加「分镜」段 `[生成分镜 ▾]`。新任务 UIC-59 / 60；UIC-52 放弃并入 59。`foundation-upgrade/32` 「资源库多一个类别 Skills」改写为 Skills 设置页。无新 Command。仅文档。

### 2026-09-15：UIC-40 场景行名字

- B-01：`Selectable` 宽改为 `avail.x - 28`。咖啡馆示例层级「立方体 / 桌子 / 杯子」可见、眼睛在右。Catch2 **214 / 1319**。无新 Command。未 commit。下一行 UIC-41。

### 2026-09-15：UI-CLARITY 全流程实机审查（`47`）

- 依据 Wisdom 22 张 2576×1408 截图写 `ui-clarity/47`：适配 S-01～08（百分比面板 + 固定 14px 在 2560 宽失效）、缺陷 B-01～13（场景行无名 = `Selectable` 负宽；`▾` 不在字体；`InputText` 右侧标签被裁；默认立方体使开始板不出现；审片单列；AI 全局设置塞进检查器）、位置对照表、目标线框、U3 任务 `UIC-40～58`。
- `44` 增 U3 波次与 2560×1440·125% 验收档；`46` 增 U3 表。仅文档改动。

### 2026-09-15：F5 AI 解冻（FND-50～55）

- 地图解冻；`IHttpClient::Post`；OpenAI 兼容适配器；Command 生成/取消/设置/运行 Skill。FND-56 示例改为 `openai-compat-json`。Catch2 **214 / 1319**。未 commit。下一行 UIC-22。

### 2026-09-15：FND-41/42/43 + 本地 Skill 安装

- ImGuizmo / 置景 Undo 20 步 / A4 横向 PDF；`SKILL.md` 可导入资源库。Catch2 **199 / 1206**。未 commit。未打 0.2/0.3/0.4/0.5 tag。下一行当时为 UIC-22。

### 2026-09-15：FND-40 + UIC-21

- 角色包静态加载（bind pose + 「含骨骼」徽标）；HUD 第三段焦距与镜头包 JSON 同公式。Catch2 **191 / 1162**。未 commit。未打 0.2/0.3/0.4 tag。

## 下一步清单

1. UI-CLARITY：**UIC-22**（剧本当前镜头行高亮）。UIC-26 / UIC-32 仍开
2. FOUNDATION F5 已齐（密钥设置、OpenAI 兼容出图/视频、示例 Skill 可安装并运行）；未打中间版本 tag
3. macOS 暂缓；有 Mac 时再按 `docs/RELEASE-CHECKLIST.md` 补云测与实机（不挡 P0）

## 工作日志（续）

### 2026-09-15：F2 + F3（FND-20～35）

- 镜头元数据、镜头包、`board.json`、分镜导入、manifest 2 skill、作者指南与示例。Catch2 **186 / 1131**。未 commit。未打 0.2/0.3/0.4 tag。下一行 FND-40。

### 2026-09-15：P0 Windows 100% + FND-17 / FND-18

- Wisdom 批准 macOS 暂缓；P0 验收只要求 Windows。`00`/`01`/`02`/`35`/`RELEASE-CHECKLIST` 已同步。
- FND-17：`SelectionKind` / `CardKind`（含 `Asset` / `Root`）；面板不再用 `"shot"` / `"未关联"` 做逻辑。字符串版快照字段仍保留。
- FND-18：导出记录按 `shotId` 回点同名镜头。Catch2 **163 / 1014**。未 commit。未打 `v0.2.0`。下一行 FND-20。

### 2026-09-15：FND-16 官方资产按 official 三元组持久化

- `.ddproj` 见 `"source": "official"`；换机器未下载也能打开。Catch2 **161 / 1003**。未 commit。下一行 FND-17。

### 2026-09-14：FND-15 删 / 插镜头改用解析器行号

- Parser 给 Scene/Shot `lineStart`/`lineEnd`。删除整段含 `####` 与围栏；CRLF 保存仍 `\r\n`。Catch2 **158 / 948**。未 commit。下一行 FND-16。

### 2026-09-14：FND-14 镜头插在选中之后

- 空 `afterShotId` 仍追加末尾。右键「在此后插入镜头」。Catch2 **154 / 916**。未 commit。下一行 FND-15。

### 2026-09-14：FND-13 节点删 / 复制 / 显隐

- 三 Command；复制共享 GPU 引用计数；隐藏节点不进 `BuildSceneView`。检查器对齐地面 / 面向相机 / 复位。Catch2 **148 / 884**。未 commit。下一行 FND-14。

### 2026-09-14：FND-12 保存免重算哈希

- `CaptureProject` 优先索引哈希；变化才后台重算。状态栏「正在校验资产」；再次保存排队。Catch2 **143 / 831**。未 commit。下一行 FND-13。

### 2026-09-14：FND-11 异步模型加载

- 打开工程不再同步解析模型。占位盒先显示；`sceneLoadPending/Total`；状态栏「加载模型 x/y」。5 个 scene-load 用例。Catch2 **141 / 794**。未 commit。

### 2026-09-14：FND-10 App 内分文件 + Dispatch 测试

- `AppState` + `Dispatch` + `ViewStateBuilder`；`Application.cpp` 283 行。6 个 Catch2 Dispatch 用例锁定现有语义（含 `InsertShot` 仍追加末尾、`RemoveLibraryAsset` 不标脏）。Catch2 **136 / 775**。未 commit。

### 2026-09-14：v0.1.3

- `CMakeLists.txt` 版本 0.1.3。F0 止血 + UI-CLARITY（U0 与不依赖 FND 的 U1/U2 行）一并入库。Windows CI 在 F0 基线已绿；macOS job 仍暂停。

### 2026-09-14：UIC-20 资源库预览纹理

- `LibraryAssetView.previewTexture`；App 主线程加载 sidecar / 官方 preview；网格 96×72，`0xFFFF` 画格式色块。无新 Command。Catch2 **130 / 735**。已随 `v0.1.3` 入库。

### 2026-09-14：UIC-34 用户设置

- `settings.json` 在 Platform 用户数据目录；取景框锁 / 网格 / 轴 / 三分线 / 安全框 / 视口背景 / 左栏折叠。无新 Command、不进 `.ddproj`。未 commit。

### 2026-09-13：UIC-33 Lucide 子集

- `assets/fonts/lucide-dd.ttf` 40 字形；ImGui MergeMode；审片/窄窗 48px 图标条与浮层。无新 CMake 目标、无第 3 个 Command。未 commit。

### 2026-09-13：UIC-31 打开导出文件

- `RevealPathCommand`；`://` / 空 / 相对路径被拒。审片导出记录与状态栏「打开」「文件夹」。本版 Command 上限 2 已用完。Catch2 **118 cases / 680 assertions**。未 commit。

### 2026-09-13：UIC-30 键盘切镜

- `SelectAdjacentShotCommand`；顶栏 ‹ › 与 `[` `]` / `↑↓`。30 镜回绕测试绿。Dispatch 等 FND-10。Catch2 **115 cases / 664 assertions**。未 commit。

### 2026-09-13：UIC-27 打开后自动选第一镜

- `OpenProject` / `LoadScript` / `--project` 后若无选中镜头，复用 `SelectShotCommand` 记录选第一镜并切到绑定机位。咖啡馆打开即过肩 1/8。Catch2 **112 cases / 616 assertions**。未 commit。

### 2026-09-13：咖啡馆示例本机全流程点过

- `--project examples/cafe.ddproj` 四模式可开。镜头条点过肩/特写/柜台正视，机位与画面跟着切。`1`/`2`/`4` 能进编剧/置景/审片。stderr 无 Assertion。导出另存为对话框未自动点。未 commit。

### 2026-09-13：咖啡馆测试剧本扩成 3 场 8 镜

- `examples/scripts/cafe.md` 与 `cafe.ddproj` 补全机位、三物体、暖光。开始板「打开示例」仍指向这两份文件。未 commit。

### 2026-09-13：热修 BeginSection abort

- 左栏段头不再用 `SetCursorScreenPos` 撑窗。未 commit。

### 2026-09-13：UIC-13 / UIC-14 / UIC-15

- 资源库一排 chrome；分镜短期画法；术语表全量替换。U0 01–17 齐。未 commit。

### 2026-09-13：UIC-16

- 上手三步纵排 + 快捷键表格。未 commit。

### 2026-09-13：UIC-12

- 分辨率 Combo；机位 3×2 + `eye-level`；段头统一 `+`。未 commit。

### 2026-09-13：UIC-11

- 检查器面孔互斥；`selectionKind="asset"`；灯光搬到场景面孔。未 commit。

### 2026-09-13：UIC-02 / UIC-03

- 3D / 非透明导出背景改中性灰；网格与轴降饱和。HUD 去掉 `LIVE` / 像素。未 commit。

### 2026-09-13：UIC-17 / UIC-01

- 空工程藏左栏底栏；视口锁定导出比例。未 commit。

### 2026-09-13：UIC-04 / UIC-05

- 左栏两段固定；镜头条 Down SideBar 132px。未 commit。

### 2026-09-13：UIC-06 / UIC-10

- 状态栏 28px；顶栏 segmented + 主按钮。未 commit。

### 2026-09-13：UIC-09 / UIC-07

- 中性色板；选中底改为 accent 18%。`AutoHideTabBar` + 层级透明段头。未 commit。

### 2026-09-13：UIC-08

- 默认 body=14 + DPI；`src/UI/UiFonts.h` 动态字号。未 commit。

### 2026-09-13：UI-CLARITY 开 U0

- 设计集六条修订已写入 `41`/`43`–`46`。U0 从 UIC-08 起。

### 2026-09-13：UI-CLARITY 设计集

- Wisdom 对 `40` 第九节 Q1–Q10 全部表态。新增 `41` `43` `44` `45` `46`。过目前不改 UI 代码。
- Lucide ISC 已记入决策表；子集字体在 `assets/fonts/lucide-dd.ttf`（UIC-33）。

### 2026-09-13：UI-CLARITY 调研报告

- 新增 `docs/dev-map/ui-clarity/`（README + `40`）：基于 `src/UI/*.cpp`、样式与四张实机截图的界面审计（所见非所得、布局不稳定、信息层级缺失、控件选择、文案、反馈六类共 40 条证据）、五条设计原则、八个区域的新内容方案、视觉系统（色板 / 字号 / 图标 / DPI）、术语表、改动清单草案 `UIC-xx`、待 Wisdom 决策 Q1–Q10。
- 区域 ID 沿用 UI-PRO；控制面约束不变。仅文档改动，未改 UI 代码。

### 2026-09-13：FND-07 产品截图

- `README.md` 补掌机 / 审片两张静图，以及编剧→置景→掌机→审片→文件导出菜单的 GIF。
- 启动参数 `--workspace-mode` 仅用于复拍；抓图脚本 `tools/capture-readme-shots.ps1`。

### 2026-09-13：暂停云上 macOS CI

- Wisdom：先只跑 Windows job；Mac 实机再编。`ci.yml` 矩阵去掉 `macos-latest`。

### 2026-09-13：CI macOS FileDialog 弃用

- macOS job 编过 metal 后死在 `FileDialogMac.mm` 的 `allowedFileTypes`（`-Werror`）。改为 `allowedContentTypes`。

### 2026-09-13：F0 FND-02–09

- shader 按平台裁剪；Ctrl+E 走当前分辨率；删资源库索引不标脏；视口 resize 只在变化时推；工程哈希改 `Core::Sha256Hex`。
- Issue/PR 模板；`release.yml` 打 `v*` 出草稿安装包；版本号只读 `CMakeLists.txt`。
- README 英文摘要、CI/Release 徽章、30 秒上手。截图 / GIF 未补。

### 2026-09-13：FND-01 Windows CI 生成器

- CI Windows runner 从 `windows-latest` 改为 `windows-2022`，对齐 `Visual Studio 17 2022` 预设。未改 Ninja。

### 2026-09-13：FOUNDATION 设计集落地

- 新增 `docs/dev-map/foundation-upgrade/`（README + `30`–`36`）：同类项目调研与差距、逐文件代码审查、Skills 三层融合、契约增量、五波任务表、停手清单、状态页。
- 依据 `v0.1.2` 全仓审查：CI 双红、打开 / 保存同步 IO、节点无删除、镜头只能追加、`RemoveShotSection` 文本扫描、官方资产持久化不符契约、两份 SHA-256、字符串判业务状态。
- 改写四条旧决策（App 内分文件、统一 SHA-256、`script-format 1.1`、`asset-manifest 2`），记录在 `foundation-upgrade/35` 第四节；Gizmo / Undo / PDF 待批准。
- 仅文档改动：未改业务代码、未新建 CMake 模块、AI 仍冻结。

### 2026-08-21：0.1.2 Windows 发布

- 产品版本升到 0.1.2：UI-PRO 四模式工作台、镜头/相机左侧右键删除、镜头检查器可选已有相机或建新机位、资源库隐藏并清理缺失条目、关闭弹窗文字对比度。
- 打包 `DirectorDesk-0.1.2-windows-x64.exe` / `.zip`，发布到 GitHub Releases tag `v0.1.2`。

### 2026-08-21：镜头可选已有相机、左侧删相机

- 镜头检查器增加已有相机下拉，并保留「为该镜头建机位」；当前选中镜头可以 `LinkShotToCameraCommand`。
- 左侧相机表右键删除；检查器不再删相机。相机面孔占用列表仍不把当前画面镜头列为关联目标。

### 2026-08-21：镜头关联、左侧删镜头、资源库清理缺失

- 当前画面镜头不再走 `LinkShotToCameraCommand`；检查器去掉「关联到当前相机」；相机面孔只让其他镜头关联。
- 删除镜头改到左侧镜头表右键菜单，检查器不再删镜头。
- 资源库不展示源文件缺失的本地条目；右键删除 +「清理缺失」去掉索引记录。Wisdom 批准追加 `DeleteShotCommand` / `RemoveLibraryAssetCommand`。
- Windows Debug：109 cases / 590 assertions。

### 2026-08-21：关闭确认弹窗文字对比度

- `WorkspacePanel` 三个 `BeginPopupModal` 的正文和按钮改为与标题相同的浅色 `#e9edf4`；ImGui 样式补齐 1.92 颜色并把弹窗背景设为不透明。
- 根因：自定义 ImGui 后端按命令顺序累计索引，忽略 `ImDrawCmd::IdxOffset`。模态变暗层会把 draw command 挪到列表前面，正文/按钮因此画错成暗色。

### 2026-08-21：UI-PRO W1–W3 代码落地

- 四个新 Command、统一选择快照、工作区模式 dock 重建、状态栏/工具条/层级/检查器/镜头条。
- 导演台九职责面板停用 `Begin`；空工程中央改为开始板。
- 未新建 CMake 模块、AI 仍冻结、未拆 `Application.cpp`。
- 测试当时 106 cases / 570 assertions。

### 2026-08-21：UI-PRO 设计集落地

- 新增 `docs/dev-map/ui-pro-upgrade/`（README + `20`–`26`）：UI 专业化升级版的范围、新主路径、区域契约、契约增量、任务表、停手清单与状态页。
- 依据 Phase 10 基线代码审计：`导演台###Workspace` 一栏九职责、三份互不相关的选择、`statusText` 藏在左栏底部、分镜总览只占中央下方三成。
- 定下八个稳定区域 ID、四个工作区模式（默认 `shoot`）、统一选择模型、15 个任务、4 个新 Command 与 6 组新快照字段。
- 仅文档改动：未改业务代码、未新建 CMake 模块、AI 仍冻结、未拆 `Application.cpp`。

### 2026-08-20：暂定应用 logo

- 源图 `img/dog.png`，Windows 多尺寸图标 `img/directordesk.ico`。
- `DirectorDesk.exe` 嵌入 `GLFW_ICON`；Inno Setup 使用同一图标和向导小图。

### 2026-08-19：Pages 目录按 00–08 正序

- `docs/index.html` 开发地图入口改为 00→08，不再 08、07 倒着排。

### 2026-08-19：知识库 `08` 与 GitHub 入口

- 新增 `docs/dev-map/08-SEAMS-AND-VIBE-CODING.md`：接缝先于功能、好处与弊端、vibe coding 能/不能。
- 根目录 `AGENTS.md` 只指向开发地图。Pages `docs/index.html` 改为目录枢纽，图与正文仍各一处。
- 未改业务代码，未拆 `Application.cpp`。

### 2026-08-19：`07` 生效

- Wisdom 确认 `07-ITERATION-AND-LANDING.md` 无改动意见；去掉草稿标记，作为 Demo 现行迭代政策。
- 未改业务代码，未拆 `Application.cpp`。

### 2026-08-19：Demo 迭代政策

- 新增 `docs/dev-map/07-ITERATION-AND-LANDING.md`：模块数锁定、按名词落点、加/删分层、AI 冻结、暂不拆 Application.cpp。
- `01`/`04`/`06`、`CONTRIBUTING.md`、`README.md` 增加指向，避免第二份架构正文。

### 2026-08-19：0.1.1 Windows 发布

- 视口滚轮闪屏修复随 `phase-10-p1` 打 Windows 安装包。

### 2026-08-19：视口滚轮缩放闪屏

- 视口窗口不再把滚轮当成滚动条；离屏缩略图不再 `bgfx::reset` 成 320×180。
- 缩略图改为异步回读，避免在主循环里额外 `bgfx::frame()` 把清空后的交换链呈上去。
- 视口用 InvisibleButton 吃输入，尺寸变化小于 2px 不重建 RT。

### 2026-08-19：Phase 10 体验打磨与发布准备

- 统一中文空状态、快捷键与对外文档。
- 示例工程 `examples/cafe.ddproj`。
- Windows Inno Setup 安装包脚本，发布到 GitHub Releases。

### 2026-08-19：Phase 9–0

- AI 接口、官方资产库、分镜画布、工程文件、资源库、机位、剧本与渲染骨架。
