# 33 - 契约增量（CONTRACT DELTA）

> 本版所有新接缝的确切签名。命名遵守 `../05` 第二节：`PascalCase` + `Command` 后缀，稳定字符串 ID 用小写连字符。
> 现有 59 个 Command 变体一律复用；只有 `InsertShotCommand` **加字段**（默认值保持旧语义）。
> 第三至六节是数据格式。执行时先把它们抄成 `../modules/` 下的独立文件（文件名已给出），再写代码——契约先于代码。

## 一、新增 Command

加在 `include/DirectorDesk/Core/Command.h`，并追加进 `using Command = std::variant<...>`。

```cpp
// ---- F1 · 地基 ----

// 删除场景节点。只从层级面板右键发出；被引用的 gpuModelId 由 App 释放
struct DeleteNodeCommand { std::string nodeId; };

// 复制场景节点：新 ID、同资产、位置 +X 偏移 0.5m；复制后成为当前选择
struct DuplicateNodeCommand { std::string nodeId; };

// 显隐。对应 .ddproj 已有的 nodes[].visible；隐藏节点不渲染、不进导出
struct SetNodeVisibleCommand { std::string nodeId; bool visible = true; };

// 【改字段】afterShotId 为空 = 旧行为（追加到当前场次/文档末尾）
struct InsertShotCommand { std::string afterShotId; };

// ---- F2 · 镜头包 ----

// 改写一条镜头元数据；value 为空表示删除该键。落点 Script::Document
struct SetShotMetaCommand { std::string shotId; std::string key; std::string value; };

// 导出镜头包（PNG + .shot.json）。shotId 空 = 当前镜头；resolutionId 空 = 当前选择
struct ExportShotPackageCommand { std::string shotId; std::string resolutionId; };

// ---- F3 · Skills 融合 ----

// 弹文件对话框选 storyboard-import.json
struct ImportStoryboardCommand {};

// 直接按路径导入。mode ∈ {"replace", "append"}
struct ImportStoryboardFromPathCommand { std::string utf8Path; std::string mode = "append"; };

// ---- F4 · 角色包与体验 ----

struct UndoCommand {};
struct RedoCommand {};
struct ExportStoryboardPdfCommand {};

// ---- F5 · AI 解冻 ----

struct GenerateShotImageCommand { std::string shotId; };
struct GenerateShotVideoCommand { std::string shotId; };
struct CancelAiJobCommand {};
struct SetAiSettingsCommand {
    std::string provider;
    std::string baseUrl;
    std::string apiKey;
    std::string imageModel;
    std::string videoModel;
};
struct RunSkillCommand { std::string assetId; };
```

Skill 分发**不需要新 Command**：`DownloadOfficialAssetCommand` / `CancelOfficialDownloadCommand` / `SetOfficialCategoryCommand` 已覆盖；只是资产的 `format` 变成 `skill`。

### visit 分发语义

| Command | 做什么 | 必须同时做 | status 文案 |
|---------|--------|------------|-------------|
| `DeleteNodeCommand` | `scene.Remove(nodeId)`；释放 `gpuModelId` | `projectDirty = true`；若是当前选择则 `selectionKind = None`；所有引用该场景的镜头缩略图 `MarkStale` | 「已删除 <名>」 |
| `DuplicateNodeCommand` | `scene.Duplicate(nodeId)` 返回新 ID；复用原节点 `gpuModelId`（同一 GPU 模型可多节点共享，App 需引用计数或在删除时检查是否仍被引用） | `projectDirty`；选择切到新节点；缩略图 `MarkStale` | 「已复制为 <名>」 |
| `SetNodeVisibleCommand` | `scene.SetVisible` | `projectDirty`；缩略图 `MarkStale`；`RenderScene` 跳过不可见节点 | 无（静默） |
| `InsertShotCommand{afterShotId}` | `script.InsertShot(afterShotId)`；不存在的 ID 回落到追加并诊断 | 既有行为（标脏、重排画布） | 既有 |
| `SetShotMetaCommand` | `script.SetShotMeta(shotId, key, value)`：改写紧跟标题的 `> key: value` 行 | `scriptDirty`；卡片 `metaLine` 重组 | 无 |
| `ExportShotPackageCommand` | App 组装 `Export::ShotPackageInput`（当前相机、场景节点、剧本正文与 meta、分辨率）→ 离屏渲染 → `Export::WriteShotPackage` | 复用现有导出路径对话框与覆盖确认；写 `exportLog`（`label = "package"`） | 「已导出镜头包 <path>」 |
| `ImportStoryboardCommand` | `Platform` 文件对话框 → 推 `ImportStoryboardFromPathCommand` | — | — |
| `ImportStoryboardFromPathCommand` | 读文件 → `Script::ImportStoryboard(json)` → 按 `mode` 生成 Markdown → 走 `SetScriptTextCommand` 同一条路径 | 若当前剧本 dirty 且 `mode == "replace"`，先弹保存提示（复用工程提示三件套的模式） | 「已导入 N 场 M 镜」；失败列前三条诊断 |
| `UndoCommand` | 弹出置景快照栈，恢复 Scene / Camera / Link | 剧本文本不动；空栈静默 | 「已撤销」 |
| `RedoCommand` | 弹出重做栈 | 同上 | 「已重做」 |
| `ExportStoryboardPdfCommand` | App 组装每页格子图 → `Export::WriteBoardPdf` | 复用导出路径与覆盖确认；写 `exportLog`（`label = "pdf"`） | 「已导出分镜 PDF <path>」 |
| `GenerateShotImageCommand` | 组装 `ImageGenRequest`（提示词来自 meta / 正文，参考图为当前镜头 PNG）；`mock` 本地拷贝，`openai-compat` 经 Worker 调 HTTPS | 无密钥且非 mock 则拒绝；写 `exportLog`（`label = "ai-image"`） | 「已生成图像 <path>」或错误 |
| `GenerateShotVideoCommand` | 同上，走 `VideoGenRequest` | 同上；`label = "ai-video"` | 「已生成视频 <path>」 |
| `CancelAiJobCommand` | 取消进行中的生成 | 空闲时静默 | 「已取消生成」 |
| `SetAiSettingsCommand` | 写入 `UserSettings` 的 AI 字段并保存 `settings.json`；`apiKey` 空表示保留原密钥 | 不进 `.ddproj`；不记日志 | 「已保存 AI 设置」 |
| `RunSkillCommand` | `AI::RunSkill`：`copy-output` 同步导入；`openai-compat-json` 用文本模型经 Worker 调 chat completions，成功则 append 导入 | 无密钥且非 mock 则拒绝；无 `skill.json` 则提示在 Agent 中运行 | 「已导入 N 场 M 镜」 |

## 二、新增 `AppViewState` 字段

加在 `include/DirectorDesk/UI/IPanel.h`。生命周期规则不变：`const char*` 指向 App 当帧存活的 `std::string`；容器传 `const std::vector<T>*`。

```cpp
enum class SelectionKind : std::uint8_t { None, Shot, Node, Camera };
enum class CardKind : std::uint8_t { Scene, Shot };

struct ShotMetaView {
    std::string key;      // 原样键名，如 "景别"
    std::string value;
};

struct AppViewState {
    // ... 现有字段保持不变 ...

    // F1 · 用枚举替代字符串判断（字符串版 selectionKind / card.kind 保留到 F1 末删除）
    SelectionKind selectionKindEnum = SelectionKind::None;

    // F1 · 异步加载进度（0 表示无待加载）
    std::uint32_t sceneLoadPending = 0;
    std::uint32_t sceneLoadTotal = 0;
    bool projectSaveInProgress = false;

    // F2 · 当前镜头元数据（检查器编辑用）
    const std::vector<ShotMetaView>* selectedShotMeta = nullptr;

    // F3 · 导入结果诊断（导入后一帧到用户关闭之间有效）
    const std::vector<std::string>* importDiagnostics = nullptr;

    // F4
    bool canUndo = false;
    bool canRedo = false;
    bool gizmoActive = false;
    float gizmoView[16] = {};
    float gizmoProj[16] = {};
    float gizmoWorld[16] = {};

    // F5
    const char* aiProvider = "openai-compat";
    const char* aiBaseUrl = "";
    const char* aiImageModel = "";
    const char* aiVideoModel = "";
    const char* aiChatModel = "";
    bool aiHasApiKey = false;
    bool aiBusy = false;
    const char* aiJobStatus = "";
    const char* aiJobMessage = "";
    float aiJobRatio = 0.0f;
    const char* lastAiOutputPath = "";
    bool canGenerateAi = false;
    bool canRunSelectedSkill = false;
};

// 既有结构的增量
struct StoryboardCardView {
    // ... 现有 ...
    CardKind kindEnum = CardKind::Shot;
    std::string metaLine;   // "中景 · 推 · 3s"，App 拼好，面板只画
};

struct ExportLogView {
    // ... 现有 ...
    std::string shotId;     // 回点用；分镜总览为空
};

struct SceneNodeView {
    // ... 现有 ...
    bool visible = true;
    bool hasSkin = false;   // F4：角色包静态显示徽标
};

struct LibraryAssetView {
    // ... 现有 ...
    bool canRunSkill = false; // F5：安装目录有 skill.json
    bool skillUsesLlm = false; // FND-56：builtin openai-compat-json
};
```

## 三、`script-format` 1.0 → 1.1（改 `../modules/script-format.md`）

**新增规则（只增不改）**：

- 镜头标题**之后、第一行正文之前**，连续的 Markdown 引用块行 `> key: value` 视为该镜头的**元数据**；空行终止。
- `key` 为去掉首尾空白的任意非空文本（中英皆可，冒号用半角 `:` 或全角 `：`）；`value` 为冒号后去首尾空白的文本，可为空。
- 同键重复：保留最后一次，给警告诊断。
- 场次标题之后的引用块**不解析**（P0 不做场次元数据），仍是正文。
- 元数据行属于该镜头的文本区间；删除 / 移动镜头时随之移动。
- 解析器把元数据作为**有序键值列表**放进 `Shot::meta`，不做语义解释；以下键名是**推荐约定**（UI 检查器按这个顺序提供快捷填写，但不强制）：

| 推荐键 | 示例 | 用途 |
|--------|------|------|
| `景别` | 远景 / 全景 / 中景 / 近景 / 特写 | 卡片 metaLine、镜头包 |
| `运镜` | 固定 / 推 / 拉 / 摇 / 移 / 跟 | 同上 |
| `时长` | `3s` | 同上 |
| `提示词` | 任意文本 | 镜头包 `prompt` 字段 |
| `负面提示词` | 任意文本 | 镜头包 `negativePrompt` |

示例：

```markdown
### [shot:shot-cafe-001] 过肩
> 景别: 中景
> 运镜: 推
> 时长: 3s
> 提示词: 暖色午后光，浅景深，胶片质感

A 坐在窗边，看向街道。
```

**兼容**：1.0 剧本零变化——以前这些行是正文，现在成了元数据，卡片和镜头包多了信息，正文少了几行引用块。`script-format.md` 第五节「非目标」改为「P0 不解析人物、对白、情绪等语义；镜头元数据自 1.1 起以引用块表达」。

**测试**：全角冒号；空值；重复键；元数据与正文之间无空行（元数据直到第一个非 `>` 行）；场次下的引用块不解析；CRLF。

## 四、`asset-manifest` 1 → 2（改 `../modules/asset-manifest.md`）

**新增 / 放宽**：

| 字段 | 变化 |
|------|------|
| `schemaVersion` | 接受 `1` 与 `2`；`1` 的清单按旧规则解析 |
| `format` | 增加 `skill`。`skill` 的 `entrypoint` 必须是 `SKILL.md` |
| `kind`（新，可选） | `model`（默认）\| `skill` \| `character`。`character` 的 `format` 仍必须是 `glb` |
| `rig`（新，可选，仅 `kind == character`） | `{ "type": "humanoid" \| "custom" \| "none", "source": string, "boneCount": int }`；只校验类型，不校验骨骼内容 |
| `preview` | `skill` 可省略；缺省显示通用 Skill 图标 |
| `license` | `skill` 允许 `MIT` / `Apache-2.0` / `CC0-1.0` / `CC-BY-4.0` |

**行为**：

- 下载、校验、原子缓存、状态机**不变**。
- 资源库类别由清单 `categories` 决定，建议官方清单新增 `{ "id": "skill", "name": { "zh-CN": "Skills" } }`。
- `kind == skill` 的资产：**不能**被 `AddLibraryAssetToSceneCommand` 拖进场景（App 拒绝并 status「Skill 不是模型」）；检查器显示 `SKILL.md` 前 40 行纯文本、安装路径、「打开文件夹」。
- 旧版本（0.1.x）读到 `schemaVersion: 2` 会整体拒绝并用旧缓存——这是既定行为，可接受；官方清单切到 2 时在 Release note 写明。

## 五、镜头包 `shot-package` 1（新建 `../modules/shot-package.md`）

`Export::WriteShotPackage` 写两个文件到用户选的目录：`<shot-id>.png`（与现有单镜头导出完全相同）与 `<shot-id>.shot.json`：

```json
{
  "format": "DirectorDeskShotPackage",
  "formatVersion": 1,
  "generatedBy": "DirectorDesk 0.3.0",
  "generatedAt": "2026-10-01T12:00:00Z",
  "project": { "name": "咖啡馆短片", "projectId": "proj-..." },
  "scene":   { "id": "scene-cafe-day", "title": "咖啡馆 - 日 - 内", "body": "场次说明……" },
  "shot": {
    "id": "shot-cafe-001",
    "title": "过肩",
    "body": "A 坐在窗边，看向街道。",
    "meta": [ { "key": "景别", "value": "中景" }, { "key": "运镜", "value": "推" } ],
    "prompt": "暖色午后光，浅景深，胶片质感",
    "negativePrompt": ""
  },
  "image": {
    "color": "shot-cafe-001.png",
    "width": 1920, "height": 1080,
    "transparentBackground": false
  },
  "camera": {
    "id": "camera-main", "name": "主镜头",
    "position": [0.0, 1.6, 5.0],
    "rotation": [0.0, 0.0, 0.0, 1.0],
    "lookAt": [0.0, 1.0, 0.0],
    "verticalFovDegrees": 45.0,
    "horizontalFovDegrees": 71.0,
    "focalLength35mmEquivalent": 29.0,
    "aspect": 1.7778,
    "preset": "front"
  },
  "sceneNodes": [
    { "id": "node-chair-01", "name": "椅子", "assetId": "basic-chair", "assetName": "基础椅子",
      "position": [0.0, 0.0, 0.0], "rotation": [0,0,0,1], "scale": [1,1,1], "visible": true }
  ],
  "lighting": { "preset": "studio-basic" }
}
```

规则：

- `prompt` / `negativePrompt` 是 `meta` 中 `提示词` / `负面提示词` 的**副本**，方便下游直接取；无则为空串。
- 焦距换算以 24mm 画幅高为基准：`focal = 12 / tan(vfov / 2)`；`hfov = 2 · atan(aspect · tan(vfov / 2))`。纯数学，在 Export 内完成。
- `sceneNodes` 只列可见节点；`assetId` 对官方资产是 `id`，对本地资产是索引 ID。
- 不含深度图 / 法线图（v2 议题）。`formatVersion` 升到 2 时才允许加。
- Export 不 include Script / Scene / Camera：以上全部由 App 装进 `Export::ShotPackageInput` 值对象。

分镜总览导出（`ExportStoryboardBoardCommand`）在 F2 末追加可选 `board.json`：`{ "format": "DirectorDeskBoardIndex", "shots": [ { "id", "title", "image", "metaLine" } ] }`，让下游知道每格是哪个镜头。

## 六、导入格式 `storyboard-import` 1（新建 `../modules/storyboard-import.md`）

```json
{
  "format": "DirectorDeskStoryboardImport",
  "formatVersion": 1,
  "source": { "tool": "shuohao-skills/storyboard", "version": "0.3.0" },
  "title": "我的短片",
  "scenes": [
    {
      "id": "scene-cafe-day",
      "title": "咖啡馆 - 日 - 内",
      "body": "场次说明。",
      "shots": [
        {
          "id": "shot-cafe-001",
          "title": "过肩",
          "body": "A 坐在窗边，看向街道。",
          "meta": { "景别": "中景", "运镜": "推", "时长": "3s", "提示词": "……" }
        }
      ]
    }
  ]
}
```

规则：

- `format` 必须严格相等；`formatVersion` 只接受 `1`。
- `scenes[].id` / `shots[].id` **可选**；缺失或不合法（不满足 `[a-z0-9][a-z0-9_-]{0,63}`）时由应用生成 `scene-<短码>` / `shot-<短码>` 并给提示级诊断。`append` 模式下与现有剧本冲突的 ID 同样重生成并诊断。
- `title` 缺失 → 「未命名」+ 警告；`body` 缺失 → 空。
- `meta` 是对象，键值都转为字符串；保持 JSON 里的键顺序（解析用 `ordered_json`）。
- 生成的 Markdown 严格符合 `script-format 1.1`：`# title` → `## [scene:id] title` → 正文 → `### [shot:id] title` → `> k: v` 行 → 空行 → 正文。
- 未知字段忽略；单个镜头非法不影响其他镜头，但根对象非法整体拒绝。
- 落点 `Script::ImportStoryboard`；`dd_script` 目标链接 `nlohmann_json::nlohmann_json`（私有链接，公共头不出现 json 类型）。

**测试**：完整 round-trip（导入 → 解析 → 与 JSON 结构一致）；缺 ID；非法 ID；`append` 冲突；中文键；空 `scenes`；`formatVersion: 2` 拒绝。

## 七、`Asset::Library` 索引增量

索引 JSON 每条记录新增可选字段：`origin`（`"local"` 默认 / `"official"`）、`version`（官方版本）、`sha256`（已有则保留）、`sourceMtime`、`sourceSize`（用于 CR-12 免重算）。旧索引缺字段按默认处理，不需要迁移。

## 八、App 内文件切分（不改任何公共契约）

见 [`31`](31-CODE-REVIEW.md) CR-10。产物全部在 `src/App/`，加进 `dd_app`；`Application.h` 的公共签名不变。

## 九、持久化决定

| 项 | 决定 |
|----|------|
| 镜头元数据 | 进**剧本 Markdown**（引用块），不进 `.ddproj` |
| `visible` | 已在 `.ddproj` 1 中定义，本版只是接上入口，不升格式版本 |
| 官方资产引用 | 按 `.ddproj` 1 已定义的 `official` 三元组写，不升格式版本 |
| 导入来源（`source.tool`） | 不持久化；只出现在 status 与诊断 |
| Skill 安装位置 | 官方资产缓存目录，不进工程 |
| `.ddproj` 格式版本 | **本版保持 1** |
| AI 密钥与端点 | 用户目录 `settings.json`，不进工程 |

## 十、不要顺手做的事

- 不要把供应商 SDK 链进公共头；AI HTTP 只走 `IHttpClient`。
- 不要把 API 密钥写进日志或 `.ddproj`。
- 不要让 Script 依赖 Storyboard、Storyboard 依赖 Script（导入只生成文本）。
- 不要在 Export 里 include Script / Scene / Camera 头；镜头包输入全部由 App 装成值对象。
- 不要为 Skill 新建 `src/Skill/`——它就是一种官方资产。
- 不要给 `meta` 做 schema 校验或枚举限制；键是自由文本，约定只在 UI 层提供快捷项。
- 不要把 `SelectionKind` 枚举变成第四份选择状态；它仍是快照里的指针。
- 不要在本版升 `.ddproj` 格式版本。
