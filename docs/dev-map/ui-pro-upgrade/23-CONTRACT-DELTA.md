# 23 - 契约增量（CONTRACT DELTA）

> 本版要新加的接缝，全部落在**已有文件**里：`include/DirectorDesk/Core/Command.h`、`include/DirectorDesk/UI/IPanel.h`、`src/App/Application.cpp`、`src/UI/*.cpp`。
> 命名遵守 `../05` 第二节：`PascalCase` + `Command` 后缀，稳定字符串 ID 用小写连字符。
> 现有 53 个 Command 变体一律复用，不改签名、不改语义。

## 一、新增 Command（4 个）

加在 `include/DirectorDesk/Core/Command.h`，并追加进末尾的 `using Command = std::variant<...>`。

```cpp
// 切换工作区模式。modeId ∈ {"script", "set", "shoot", "review"}
struct SetWorkspaceModeCommand {
    std::string modeId = "shoot";
};

// 按当前模式重建默认 dock 布局（用户把布局拖乱后的恢复出口）
struct ResetLayoutCommand {};

// 为指定镜头新建一台相机并立即绑定。shotId 为空表示当前选中镜头
struct BindShotToNewCameraCommand {
    std::string shotId;
};

// 选择导出分辨率。resolutionId 复用 Export::TryParseResolution 接受的 ID
struct SelectExportResolutionCommand {
    std::string resolutionId = "1080p";
};
```

### visit 分发语义（写在 `src/App/Application.cpp` 现有 visit 链里，不新开文件）

| Command | 做什么 | 必须同时做 | status 文案 |
|---------|--------|------------|-------------|
| `SetWorkspaceModeCommand` | 校验 `modeId`，写入 App 持有的 `workspaceModeId` | 置 `layoutRebuildRequested = true`，让下一帧 `ApplyDefaultDockLayout` 按新模式重建 | 「已切换到掌机模式」等；未知 ID 时「未知工作区模式」且不改状态 |
| `ResetLayoutCommand` | 置 `layoutRebuildRequested = true` | — | 「已重置布局」 |
| `BindShotToNewCameraCommand` | `shotId` 空则取 `script.SelectedShotId()`；`cameras.Add()` 后用新相机 id `links.Set(shotId, cameraId)` | `projectDirty = true`；`storyboard.MarkShotStale(shotId)`；`thumbScheduler.NotifyBusy(NowMs())` | 「已为 <镜头> 新建并绑定 <相机>」；无当前镜头时「请先选择镜头」 |
| `SelectExportResolutionCommand` | 用 `Export::TryParseResolution` 校验后写入 App 持有的 `exportResolutionId` | 后续 `ExportCurrentShotCommand` 若 `resolutionId` 为空则用这个值 | 未知时「未知导出分辨率」 |

> `BindShotToNewCameraCommand` 只是把现有两条 Command 的效果串起来并给统一反馈；它不新增任何领域能力，Camera 与 Link 的规则仍在各自模块。

### 追加（Wisdom 2026-08-21）

超出原 4 个上限，由 Wisdom 点名要求：

```cpp
struct DeleteShotCommand { std::string shotId; };
struct RemoveLibraryAssetCommand { std::string assetId; };
```

| Command | 做什么 |
|---------|--------|
| `DeleteShotCommand` | `Script::Document::RemoveShot`；`links.ClearShot`；只从左侧镜头表右键发出 |
| `RemoveLibraryAssetCommand` | `Asset::Library::Remove` 从索引删掉记录（含缺失源文件的条目）；不删磁盘源文件 |

镜头检查器可选已有相机（`LinkShotToCameraCommand`）或「为该镜头建机位」。相机面孔的占用列表仍不把当前画面镜头列为「关联」目标。删相机只在左侧相机表右键（`RemoveCameraCommand`）。

## 二、新增 `AppViewState` 字段（6 组）

加在 `include/DirectorDesk/UI/IPanel.h` 的 `AppViewState`，由 App 每帧填充。**生命周期规则不变**：`const char*` 必须指向 App 拥有且当帧存活的 `std::string`；容器一律传 `const std::vector<T>*`。

```cpp
struct ExportIssueView {
    std::string shotId;
    std::string shotTitle;
    const char* reason = "";   // "未关联相机" / "缩略图过期" / "渲染失败" / "缺少缩略图"
};

struct ExportLogView {
    std::string label;         // "1080p" / "2k" / "board"
    std::string shotTitle;     // 分镜总览时为空
    std::string path;
    bool ok = false;
    std::string message;       // 失败原因；成功时为空
};

struct AppViewState {
    // ... 现有字段保持不变 ...

    // UI-PRO：工作区模式
    const char* workspaceModeId = "shoot";

    // UI-PRO：统一选择模型
    const char* selectionKind = "none";   // "shot" | "node" | "camera" | "none"
    const char* selectionId = "";
    const char* selectionLabel = "";

    // UI-PRO：导出就绪清单与导出记录
    const std::vector<ExportIssueView>* exportIssues = nullptr;
    const std::vector<ExportLogView>* exportLog = nullptr;

    // UI-PRO：当前导出分辨率（与 SelectExportResolutionCommand 配对）
    const char* exportResolutionId = "1080p";
};
```

### 谁填充、从哪来

| 字段 | 来源 | 备注 |
|------|------|------|
| `workspaceModeId` | App 持有的字符串，由 `SetWorkspaceModeCommand` 改写 | 启动默认 `"shoot"` |
| `selectionKind` / `selectionId` / `selectionLabel` | App 在分发 `SelectShotCommand` / `SelectNodeCommand` / `SelectCameraCommand` 时记录「最后聚焦」 | **不需要新 Command**。三份所有权仍在 Script / Scene / Camera；这里只是一个指针 |
| `exportIssues` | App 遍历自己已组装的 `storyboardCards`（`kind == "shot"` 的 `preview` / `link`）逐条生成 | `Storyboard::CountExportPreviewIssues` 只返回 stale/failed/missing 三个计数，继续用于总数校验；逐条清单在 App 侧组装，不改 Storyboard 接口 |
| `exportLog` | App 维护的固定容量环形缓冲（建议 20 条），在导出完成分支里追加 | 不持久化，退出即丢 |
| `exportResolutionId` | App 持有，由 `SelectExportResolutionCommand` 改写 | 与 `Export::ShotResolution` 一一对应 |

## 三、新增窗口稳定 ID（3 个）

| 区域 ID | 窗口标题###稳定ID | 在哪个 `.cpp` 里 `Begin` | 说明 |
|---------|-------------------|--------------------------|------|
| `LEFT_HIERARCHY` | `层级###Hierarchy` | `src/UI/WorkspacePanel.cpp` | 从 `导演台###Workspace` 拆出 |
| `RIGHT_INSPECTOR` | `检查器###Inspector` | `src/UI/WorkspacePanel.cpp` | 从 `导演台###Workspace` 拆出 |
| `BOTTOM_STRIP` | `镜头条###ShotStrip` | `src/UI/StoryboardPanel.cpp` | 与分镜画布共用 `storyboardCards` |

规则：

- `###` 后缀一旦写下不再改；改它等于丢用户的 `imgui.ini` 布局。
- 现有五个 ID（`###Viewport` `###Workspace` `###Script` `###Library` `###Storyboard`）**保留不动**。`导演台###Workspace` 拆空后可以退化成「工程信息 + 快速开始」，也可以在最后一波移除入口——移除时按 `../07` 第五节分层剥，不要留死窗口。
- `TOOL_STRIP` 与 `STATUS_BAR` 不是 dock 窗口，用 `ImGui::BeginViewportSideBar` 挂在主视口上下沿，DockSpace 会自动让位；两者都设 `ImGuiWindowFlags_NoDocking`。

## 四、`ApplyDefaultDockLayout` 的改造契约

现状（`src/UI/WorkspacePanel.cpp`）：

```cpp
void ApplyDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& size) {
    if (ImGui::DockBuilderGetNode(dockspaceId) != nullptr) {
        return;   // 只构建一次，之后完全交给 imgui.ini，没有重置出口
    }
    ...
}
```

目标：

```cpp
// modeId 决定各窗口落位；force 为 true 时无条件重建（响应 ResetLayoutCommand 或模式切换）
void ApplyDockLayout(ImGuiID dockspaceId, const ImVec2& size, const char* modeId, bool force);
```

- 切分顺序保持「先左、再右、再从中央切下」，这样 `BOTTOM_STRIP` 落在 `CENTER_STAGE` 之下而不是全宽——与 [`22`](22-AREA-CONTRACT.md) 的线框一致。
- 各模式比例见 [`22`](22-AREA-CONTRACT.md) 第四节；隐藏某区域用 `DockBuilderDockWindow` 不派发 + 该帧不 `Begin`，不要用 0 尺寸节点。
- 重建后必须 `DockBuilderFinish`。

## 五、持久化决定

| 项 | 决定 | 理由 |
|----|------|------|
| `workspaceModeId` | **本版不进 `.ddproj`**，启动固定 `"shoot"` | 改工程格式要先改 `../modules/project-file.md` 并由 Wisdom 批准；模式属于「此刻的工作方式」，不属于作品数据 |
| dock 布局 | 继续由 ImGui 写 `imgui.ini` | 已有行为，本版只补一个重置出口 |
| `exportLog` | 不持久化 | 会话内可追溯即可 |
| `selectionKind` 等 | 不持久化 | 派生状态 |

## 六、不要顺手做的事

- 不要把 `selectionKind` 变成第四份「真正的选择状态」——它只是指针，业务所有权仍在 Script / Scene / Camera。
- 不要在面板里推断三点状态（关联 / 缩略图 / 导出）：必须由 App 组装进 `storyboardCards`，面板只画。
- 不要为工作区模式做「可配置模式系统」。四个模式写死，`../08` 第十节：为变化预留接缝，不为幻想预留房间。
- 不要给新 Command 加返回值或回调；结果只经 `AppViewState` 反映（`../01` 第四节 1）。
- 不要在这一版新增第 5 个 Command。如果某个界面需求逼出第 5 个，先回 [`24-LANDING-CHECKLIST.md`](24-LANDING-CHECKLIST.md) 确认它不是搬家能解决的。
