# 21 - 新主路径（WORKFLOW V2）

> 本文件是 UI-PRO 的产品路径正文，覆盖 `../00` 第二节的 Demo 顺序。
> 每一步都映射回现有名词：Script / Scene / Camera / Asset / Storyboard / Link / Export / App。标「新」的是本版要扩的接缝，签名见 [`23-CONTRACT-DELTA.md`](23-CONTRACT-DELTA.md)。

## 一、核心转向

主线不是「一条线性流程」，而是一个常驻对象：**Shot**。

```text
Script ──生成──▶ Shot ◀──绑定── Camera
                  │
                  ├── Storyboard 给它缩略图与三点状态
                  └── Export 以它为单位交付
```

界面的任务只有一句：**让用户始终知道「我在做哪一颗镜头，它差什么」。** 这也是 Premiere 用 Sequence、Blender 用 Scene 组织一切的同一手法。

## 二、打开软件第一件事

| | Demo 现在的答案 | UI-PRO 的答案 |
|---|-----------------|---------------|
| 首屏 | 空 3D 视口占中央 + 三个面板各说一句空话 | 一张「开始板」：打开示例工程 / 新建工程 / 打开工程 |
| 隐含模型 | 这是一个工具 | 这是一个**文档**（`.ddproj`），文档的时间线是镜头表 |
| 依据 | — | Photoshop 先要画布，Premiere 先要 Project + Sequence，Blender 先要 Scene；3D 视口是取景器，不是文档 |

## 三、主次判定

| 对象 | 定位 | 为什么 |
|------|------|--------|
| 镜头表 / Shot | **全程主线（常驻）** | 唯一贯穿剧本、相机、缩略图、导出的实体 |
| 3D 视口 | 置景 / 掌机模式的主舞台 | 它是取景器；编剧与审片阶段让它占中央是浪费主视觉 |
| 分镜总览 | 审片模式的主舞台 | 它是交付物，不是边角料 |
| 剧本 Markdown | 上游源；编剧模式主舞台，其余模式收成侧栏树 | 写作是集中式一次性活动；摆镜头时只需要结构 |
| 资源库 | 置景模式主要辅助，其余收成抽屉 | 选资产集中在置景阶段 |
| 场景层级 + 变换 | 置景模式辅助 | 场景只有一份、被所有镜头共享，属于「布景一次」 |
| 灯光预设 | 检查器里的一行 | 三个预设不值得一个独立区块 |
| 缩略图渲染 / 资产下载 / 导出回读 | 后台 | 已经异步；界面只在镜头条与状态栏体现进度 |

## 四、四个工作区模式

需要模式的理由：同一套面板要服务四种截然不同的注意力分配，靠用户手动拖 dock 不可接受。**模式只重排界面，不改变任何领域状态**——同一个 Shot、同一份场景、同一批相机在四个模式里始终是同一份。

| 模式 ID | 名称 | 它回答的问题 | CENTER_STAGE | 升为主 | 隐藏或压缩 |
|---------|------|--------------|--------------|--------|------------|
| `script` | 编剧 | 这场戏要拍哪些镜头？ | 剧本 Markdown 编辑器（全宽） | RIGHT_INSPECTOR = 诊断 + 场次统计 | LEFT_LIBRARY 隐藏 |
| `set` | 置景 | 这场戏的舞台长什么样？ | 视口（含地面格网） | LEFT_LIBRARY 升为半屏资产网格 | BOTTOM_STRIP 压成一行 |
| `shoot` | **掌机（默认）** | 这一颗镜头怎么拍？ | 视口 LIVE + HUD | RIGHT_INSPECTOR = 镜头检查器；BOTTOM_STRIP = 镜头条 | LEFT_LIBRARY 折叠抽屉 |
| `review` | 审片 | 整片成立吗？能交付了吗？ | 分镜总览全尺寸 | RIGHT_INSPECTOR = 交付检查器 | 左列整列隐藏；视口降为小监视器 |

逐区域对照见 [`22-AREA-CONTRACT.md`](22-AREA-CONTRACT.md) 第四节。

## 五、统一选择模型：一个 focus，三种面孔

不删除现有三份所有权，只在上面加「最后聚焦的是谁」。入口复用现有三个 `Select*Command`，由 App 在分发时顺手记录 focus，**不需要新 Command**。

| `selectionKind` | 数据来源（已有） | RIGHT_INSPECTOR 显示 | 规则 |
|-----------------|------------------|----------------------|------|
| `shot` | `script.SelectedShotId()` | 标题、所属场次、绑定相机、缩略图状态、导出状态、机位预设、灯光 | 已绑定则自动切到该相机（现有行为，`Application.cpp:1116-1125`）；未绑定时首行就是「为该镜头建机位」 |
| `node` | `scene.SelectedId()` | 名称 + 位置/旋转/缩放 三组 `DragFloat3` | 不改变当前 Shot；顶部注明「场景对象为所有镜头共享」 |
| `camera` | `cameras.SelectedId()` | 改名、最近应用的预设、删除、**哪些镜头正用这台相机** | 不改变当前 Shot；反向列出占用者，避免误删 |
| `none` | 三者皆空或空工程 | 上手三步清单 | 不画空的属性表格 |

## 六、新主路径 10 步

每步格式：**用户看见 → 用户点什么 → 系统用哪个模块 → 发哪些 Command**。

### 01 建档（EMPTY，无模式）

- 看见：CENTER_STAGE 是开始板，三个主行动 + 一行流程提示（建档 → 编剧 → 置景 → 掌机 → 审片 → 交付）
- 点：打开示例工程（首次用户推荐路径）
- 模块：App · ProjectFile
- Command：`NewProjectCommand` · `OpenProjectCommand` · `OpenProjectFromPathCommand{exampleProjectPath}`

### 02 编剧（`script` 模式）

- 看见：Markdown 编辑器占满 CENTER_STAGE；右侧是场次/镜头树与诊断；底部是「本剧本解析出 N 场 M 镜」
- 点：打开剧本… / 粘贴正文 / + 场次 / + 镜头，看诊断清零
- 模块：Script（剧本结构唯一来源）
- Command：`LoadScriptCommand` · `LoadScriptFromPathCommand` · `SetScriptTextCommand` · `InsertSceneCommand` · `InsertShotCommand` · `SaveScriptCommand`

### 03 镜头表成为主线

- 看见：BOTTOM_STRIP 变成横向镜头条，每个 Shot 一格，显示关联 / 缩略图 / 导出三点状态；这条在四个模式里都不消失
- 点：任意一格，作为当前工作单元
- 模块：Script + Link + Storyboard
- Command：`SelectShotCommand`

### 04 置景（`set` 模式，一次性共享舞台）

- 看见：CENTER_STAGE 是带地面格网的视口；LEFT_LIBRARY 升为半屏资产网格；RIGHT_INSPECTOR 是节点变换；顶部提示「场景是所有镜头共用的」
- 点：从资源库拖资产进视口 → 选中节点 → 拖 `DragFloat3` 摆位
- 模块：Asset · Scene
- Command：`AddLibraryAssetToSceneCommand` · `ImportModelCommand` · `SelectNodeCommand` · `SetNodeTransformCommand`

### 05 掌机（`shoot` 模式，核心工作态）

- 看见：CENTER_STAGE 是 LIVE 视口，HUD 一行显示当前 Shot 与绑定相机；RIGHT_INSPECTOR 是镜头检查器
- 点：五个机位预设之一 → 视口左键旋转 / 右键平移 / 滚轮推拉 → 「为该镜头建机位」
- 模块：Camera · Link
- Command：`ApplyCameraPresetCommand{front|side|over-shoulder|top|close-up}` · `OrbitDeltaCommand` · **新** `BindShotToNewCameraCommand`（等价于 `AddCameraCommand` + `LinkShotToCameraCommand`，省一次点击并给出统一反馈）

### 06 打光（检查器一行）

- 看见：镜头检查器里一行三个灯光切片
- 点：中性 / 暖 / 冷
- 模块：Camera（LightPreset）
- Command：`SetLightPresetCommand`

### 07 横向推进到全绿

- 看见：镜头条每格三点状态由灰变亮；TOOL_STRIP 显示「12 镜 / 7 已成镜」
- 点：下一格 Shot，重复 05；缩略图不对就单格重渲
- 模块：Storyboard（缩略图调度）+ App（离屏渲染）
- Command：`SelectShotCommand` · `RefreshStoryboardThumbnailCommand`

### 08 审片（`review` 模式）

- 看见：CENTER_STAGE 是全尺寸分镜总览；未就绪卡片描边高亮；视口缩成 RIGHT_INSPECTOR 顶部小监视器
- 点：适配全部 / 聚焦当前 / 点卡片跳转 / 折叠某一场
- 模块：Storyboard
- Command：`FitStoryboardCommand` · `FocusStoryboardSelectionCommand` · `SetStoryboardSceneCollapsedCommand` · `ReportStoryboardViewCommand`

### 09 交付前看就绪清单

- 看见：RIGHT_INSPECTOR 变成交付检查器：透明导出开关、分辨率、**逐条列出**未就绪镜头及原因，每条带修复按钮
- 点：把清单清干净
- 模块：Storyboard（总数由已有的 `CountExportPreviewIssues` 给出）+ App 组装逐条清单
- Command：`SetExportTransparentCommand` · `RefreshStoryboardThumbnailCommand` · `LinkShotToCameraCommand` · **新** `SelectExportResolutionCommand`

### 10 导出并留痕

- 看见：BOTTOM_STRIP 在审片模式下变成导出记录（分辨率、镜头、路径、结果）；STATUS_BAR 常驻显示 `statusText`
- 点：导出当前镜头 / 导出分镜总览
- 模块：Export + Renderer 离屏回读
- Command：`ExportCurrentShotCommand{"1080p"|"2k"}` · `ExportStoryboardBoardCommand` · `ConfirmExportOverwriteCommand` · `ConfirmStoryboardStaleExportCommand`

## 七、与 `../00` 的映射

| 新路径要素 | 映射到现有名词 | 是否要扩 Command |
|------------|----------------|------------------|
| 建档起步板 | App · ProjectFile | 不需要，复用 New / Open / OpenFromPath |
| 镜头表成为主线 | Script（结构）+ Link（绑定）+ Storyboard（状态） | 不需要；需要快照把三点状态送到镜头条 |
| 工作区模式 | UI 体验层（`../08` 第五节） | 需要 `SetWorkspaceModeCommand`（新） |
| 统一 focus | App 在分发 `Select*Command` 时记录 | 不需要新 Command；需要新快照字段 |
| 一键为镜头建机位 | Camera.Add + Link.Set | 建议 `BindShotToNewCameraCommand`（新）；也可先用两条现有 Command 串起来 |
| 导出就绪清单 | Storyboard 计数（已有）+ App 逐条组装 | 不需要新 Command；需要把逐条问题送进快照 |
| 常驻状态栏 | `statusText`（已有） | 不需要，纯搬家 |

**没有删掉 `00` 的任何一环**：剧本仍是结构唯一来源、资源库仍是素材入口、机位仍靠预设、画布仍自动布局、导出仍是终点。改的只是**谁占主视觉、什么时候出现、以什么密度出现**。
