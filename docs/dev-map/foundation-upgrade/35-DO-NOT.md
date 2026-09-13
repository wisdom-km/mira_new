# 35 - 停手清单（DO NOT）

> 本版每轮动手前扫一遍。第一节是范围外，第二节是会把「开源工具」做回「作者自用 Demo」的习惯，第三节是改了就会安静弄坏东西的技术约束，第四节是本版**明确改写**的旧决策——只有这几条可以和 `../00`–`../08`、`../03` 的旧文字不一致。
> 与 `../07` 第六节「一票否决」和 `../ui-pro-upgrade/25` 不冲突，三份叠加生效。

## 一、现在不要做（范围外）

| 不要做 | 依据 |
|--------|------|
| 在软件内运行 Skill（Node / Python 子进程）、内嵌任何脚本运行时 | `../07` 第二节 AI 冻结；[`32`](32-SKILLS-INTEGRATION.md) L3 |
| MCP 服务端、HTTP API、任何让外部 Agent 驱动软件的通道 | 同上；镜头包 JSON 是本版唯一的对外接口 |
| 「运行 Skill」按钮、Node 路径设置、Skill 运行状态快照字段 | `../08` 第十节：不为幻想预留房间 |
| 新建 `src/<Module>/` 或 CMake 目标（含 `src/Skill/`、`src/Import/`） | `../07` 第二节；Skill 是官方资产，导入落在 Script，镜头包落在 Export |
| 升 `.ddproj` 格式版本 | 本版所需字段（`visible`、`official`）在版本 1 里都已定义，是代码没实现 |
| 深度图 / 法线图 / 姿态图导出 | 镜头包 v2 议题 |
| 运行时蒙皮、骨骼动画、姿态编辑器 | F4 只做静态显示；姿态用预烘焙 GLB 切换 |
| 时间轴、关键帧、自由绘制、多人协作 | `../00` 第五节 |
| 把 `AppViewState` 改成增量 / 脏标记更新 | 当前无性能问题 |
| 「智能合并」导入的分镜与现有剧本 | 只做 `replace` / `append`；合并是 Agent 的活 |
| 给 `meta` 做 schema 或枚举校验 | 键是自由文本；约定只在 UI 快捷项 |
| 未批准就动 F4 的 FND-41 / 42 / 43 | 各自涉及第三方依赖或 `00` 红线 |

## 二、不要保留的「自用 Demo」习惯

| 习惯 | 为什么伤开源 | 改成 |
|------|--------------|------|
| CI 红着也发版 | 贡献者第一眼就知道没人维护 | FND-01/02 之后 CI 红 = 不许打 tag |
| 版本号写在三处 | 每次发版都漏一处 | 只在 `CMakeLists.txt`，其他地方读它 |
| 本机脚本手工打包 | 别人复现不了 | tag 触发 Action |
| README 只有中文、无图 | 海外用户直接关页 | 英文摘要 + 截图 + GIF 置顶 |
| 同步 IO 卡主线程 | 用户以为崩了 | 后台回值对象，主线程只上传与写回 |
| 用中文文案当业务判断 | 改文案 = 改逻辑 | 枚举 |
| 用标题匹配对象 | 同名即错 | 稳定 ID |
| 只在作者脑子里的格式 | 下游没法接 | 每个对外文件都有 `modules/*.md` 与 `format` / `formatVersion` 字段 |
| 「以后再补测试」 | 永远不补 | 新 Command 必带 Dispatch 测试 |

## 三、改了就会安静弄坏东西（硬约束）

| 约束 | 出处 | 弄坏什么 |
|------|------|----------|
| 后台线程只能返回值对象（`Asset::ModelData`、`{path, sha256}`），不得触碰 `Scene` / `Renderer` / UI | `../01` 第五节 | 数据竞争；bgfx 非主线程调用崩溃 |
| `renderer.CreateModel` 只在主线程、只在帧序的 `RenderScene` 之前 | `../ui-pro-upgrade/25` 第三节帧序 | 撕帧、纹理闪 |
| 复制节点共享 `gpuModelId` 时，删除任一节点前必须确认无其他引用再释放 | FND-13 | 另一个节点画出垃圾或崩溃 |
| 隐藏节点必须同时在 `RenderScene`、缩略图离屏渲染、单镜头导出、镜头包 `sceneNodes` 四处生效 | FND-13 | 视口看不见但导出出现 |
| `Script::Document` 的插入 / 删除只能通过 `Parser` 行号区间操作文本，不得再写第二套标题识别 | FND-15 | 规则分叉，`####` 与围栏再次误伤 |
| 元数据引用块属于镜头文本区间的一部分 | [`33`](33-CONTRACT-DELTA.md) 三 | 删镜头留下孤儿 `> ` 行 |
| `Export` 不 include `Script` / `Scene` / `Camera` 头 | `../01` 依赖方向 | 依赖环 |
| `Storyboard` 不 include `Script` / `Link` / `Camera` / `Renderer` | `../01` 第四节 4 | 依赖环 |
| `dd_script` 对 nlohmann 只能 `PRIVATE` 链接；`include/DirectorDesk/Script/*.h` 不出现 json 类型 | `../07` 控制面 | 第三方类型泄进公共头 |
| `OfficialCatalog` 的下载 / 校验 / 原子缓存流程不因 `format: skill` 出现分支 | [`33`](33-CONTRACT-DELTA.md) 四 | 信任边界出现第二条路 |
| `kind == skill` 的资产不得进入 `AddLibraryAssetToSceneCommand` | 同上 | 场景里出现不可渲染节点 |
| `InsertShotCommand.afterShotId` 为空时行为与 0.1.2 完全一致 | 兼容 | 旧快捷键 / 菜单行为漂移 |
| 所有新 `const char*` 快照字段指向 App 当帧存活的 `std::string`（分文件后由 `FrameStrings` 持有） | `../ui-pro-upgrade/23` 二 | 悬垂指针 |
| `AppState` 分文件后 `Application.h` 公共签名不变；`main.cpp` 不改 | CR-10 | 入口漂移 |
| 窗口稳定 ID、拖放载荷名 `DD_ASSET_ID`、`0xFFFF` 无效纹理约定、视口 `InvisibleButton` + 清零滚轮、2px 阈值 | `../ui-pro-upgrade/25` 第三节 | 全部沿用，一条不改 |

## 四、本版明确改写的旧决策（唯一允许与旧文字不一致之处）

| 旧文字 | 位置 | 本版改为 | 批准 |
|--------|------|----------|------|
| 「暂缓：不拆分 `src/App/Application.cpp`」 | `../07` 第一节、`../03` 快照、`../ui-pro-upgrade/25` 第一节 | **允许在 App 目标内分文件**（`AppState` / `CommandDispatch*` / `ViewStateBuilder`），仍不新建模块、不改 `Application.h` 签名。理由：`Run()` 1200+ 行已达 `../08` 第 110 行「痛了再分」的条件；App 层无测试是 0.2 的主要风险 | Wisdom 2026-09-13（口头，随本文件夹） |
| 「工程文件仍用既有内置哈希」 | `../03` 决策表 | 统一 `Core::Sha256` | 同上 |
| `script-format` 1.0「P0 不解析…时长、镜头运动等语义」 | `../modules/script-format.md` 第五节 | 1.1：镜头元数据以引用块表达，仍不解析人物 / 对白 / 情绪 | 同上 |
| `asset-manifest` 1「`format` 仅 `glb` 或 `obj`」 | `../modules/asset-manifest.md` | 2：增加 `skill`、`kind`、`rig` | 同上 |
| 「Phase 2 变换入口 数值 DragFloat3；不引入 ImGuizmo」 | `../03` 决策表 | **未改**。FND-41 待批准 | — |
| 「P0 不做 Undo」 | `../00` 第五节 | **未改**。FND-42 待批准 | — |

执行模型改这几处旧文字时，必须在被改的那一行后加 `（FOUNDATION 版改写，见 foundation-upgrade/35 第四节）`，不要静默替换。

## 五、遇到冲突怎么办

1. 本文件夹与 `../00`–`../08` 冲突且不在第四节表内：以 `../00`–`../08` 为准，停工报告。
2. 本文件夹与 `../ui-pro-upgrade/` 冲突：界面主次与区域 ID 以 `ui-pro-upgrade` 为准；新 Command 与格式以本文件夹 [`33`](33-CONTRACT-DELTA.md) 为准。
3. 代码与文档冲突：停工，由 Wisdom 决定改哪边（`../08` 第八节）。
4. 某任务做到一半发现要新建模块或运行外部程序：落点切错了，回 [`34`](34-LANDING-CHECKLIST.md) 重拆，不要继续。
