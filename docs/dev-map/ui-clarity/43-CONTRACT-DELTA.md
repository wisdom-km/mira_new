# 43 - 契约增量（CONTRACT DELTA）

> 本版新接缝全部落在已有文件：`include/DirectorDesk/Core/Command.h`、`include/DirectorDesk/UI/IPanel.h`、`src/App/`、`src/UI/*.cpp`、`src/Platform/`、`backends/imgui/ImGuiGlfwBackend.cpp`、`backends/bgfx/BgfxRenderer.cpp`。
> 命名遵守 `../05` 第二节。现有 Command 一律复用。
> **本版新 Command 上限 2 个**（Wisdom Q9）。FOUNDATION [`33`](../foundation-upgrade/33-CONTRACT-DELTA.md) 的 Command / 字段不在此重复定义，U1 任务只引用它们。

## 一、新增 Command（2 个，U2）

加在 `Command.h` 并进入 `using Command = std::variant<...>`。

```cpp
// 按剧本顺序选相邻镜头。delta = +1 下一镜，-1 上一镜；越界回绕；无镜头时不改状态
struct SelectAdjacentShotCommand { int delta = 1; };

// 用系统文件管理器打开文件或其所在文件夹。utf8Path 必须是本地绝对路径，禁止 ://
struct RevealPathCommand { std::string utf8Path; bool folder = false; };
```

### visit 分发语义

| Command | 做什么 | 必须同时做 | status 文案 |
|---------|--------|------------|-------------|
| `SelectAdjacentShotCommand` | 按 `scriptScenes` 镜头顺序取当前 `SelectedShotId()` 的邻项；空文档或无当前镜头则取第一镜（`delta>0`）或最后一镜（`delta<0`） | 与 `SelectShotCommand` 同一条选择记录（`selectionKind` / 剧本选中） | 无（静默）或「已选中 <名>」 |
| `RevealPathCommand` | `Platform` 打开路径；`folder==true` 打开父目录并尽量选中该文件 | 路径含 `://` 或空 → 失败，不调系统 | 失败：「无法打开路径」；成功无 status |

`RevealPath` 的平台实现放在 `src/Platform/`（Win32 `SHOpenFolderAndSelectItems` / macOS `NSWorkspace`），**不新建模块**。UI 不得自己 `ShellExecute`。

### 不需要新 Command 的意图

| 意图 | 走哪条 |
|------|--------|
| 模式 `1`–`4` | 既有 `SetWorkspaceModeCommand` |
| 锁定导出比例、网格、三分线、安全框、坐标轴、左栏折叠 | **UI 本地偏好**（面板成员或 `WorkspacePanel` 静态）。U0–U1 不进工程、不进用户文件。UIC-34 才写用户设置 |
| 取景框开关 | 同上；视图菜单勾选，默认开（Q1） |
| 检查器「场景」面孔（点左栏「场景」段头） | **面板本地 bool**，不是业务状态，不加 Command |

## 二、新增 / 扩展快照字段（U1）

加在 `IPanel.h`。生命周期不变：`const char*` 指向 App 当帧 `std::string`；纹理仍是 `std::uint16_t`，`0xFFFF` 无效。

```cpp
// FOUNDATION FND-17 的枚举在本版多一个值（U1 与 FND-17 同批或紧随其后写入）
enum class SelectionKind : std::uint8_t { None, Shot, Node, Camera, Asset };

struct ShotHudView {
    const char* shotTitle = "";
    const char* cameraName = "";
    float focalLength35mm = 0.0f;   // 0 = 未知，HUD 省略第三段
};

struct LibraryAssetView {
    // ... 现有字段保持不变 ...
    std::uint16_t previewTexture = 0xFFFFu;
};

struct AppViewState {
    // ... 现有与 FOUNDATION 33 第二节保持不变 ...

    const ShotHudView* shotHud = nullptr;     // UIC-21；无选中镜头时 nullptr
    int scriptSelectedLineStart = 0;          // UIC-22；1-based 含；0 = 不高亮
    int scriptSelectedLineEnd = 0;
};
```

### 谁填充

| 字段 | 来源 | 依赖 |
|------|------|------|
| `LibraryAssetView.previewTexture` | App 主线程把 sidecar PNG / 官方 `preview` 上传为纹理；不可见条目保持 `0xFFFF` | UIC-20；`03` 已决策预览来源 |
| `ShotHudView.focalLength35mm` | 与镜头包同一公式：`12 / tan(vfov_rad / 2)`（FOUNDATION `33` 五），在 App 或 Export 纯函数里算，UI 不重算 | UIC-21、FND-21 |
| `scriptSelectedLineStart/End` | FND-15 解析器给出的当前镜头文本行号 | UIC-22、FND-15 |
| `selectionKind == "asset"`（字符串） | U0：`SelectLibraryAssetCommand` 写入；`SelectShot` / `SelectNode` / `SelectCamera` 清掉资产选中 | UIC-11；**不是新字段** |
| `SelectionKind::Asset`（枚举） | FND-17 与字符串取值一并写入枚举 | UIC-26 |

U0 **不改** `Command.h`，**不改** `SelectionKind` 枚举，只给已有 `selectionKind` 字符串加取值 `"asset"`。不加 `previewTexture`。

## 三、样式常量表（U0 · UIC-09 / UIC-02）

替换 `backends/imgui/ImGuiGlfwBackend.cpp` 的 `ApplyDirectorDeskStyle` 常量。只留**一种**强调色（Q4）。

| 角色 | 符号 | 值 | 用途 |
|------|------|----|------|
| 画布 | `kCanvas` | `0x121214` | 菜单、标题、Modal 遮罩 |
| 窗口 | `kWindow` | `0x1a1a1d` | 窗口底 |
| 表面 | `kSurface` | `0x202024` | 子区、未选 Tab |
| 浮起 | `kRaised` | `0x2a2a2f` | FrameBg、Button |
| 边框 | `kBorder` | `0x36363c` | 结构描边 |
| 文本 | `kText` | `0xe6e6e6` | 正文 |
| 次要文本 | `kMuted` | `0x9a9aa2` | 辅助 |
| 强调 | `kAccent` | `0xd89a4a` | 当前项、主按钮、进度（**唯一彩色强调**） |
| 强调热 | `kAccentHot` | `0xebb25f` | hover |
| 选中底 | `kSelection` | `kAccent` × 18% 透明 | **去掉饱和蓝** `0x31547d` |
| 成功 | `kSuccess` | `0x4fbf7f` | 状态点 |
| 警告 | `kWarning` | `0xe0894a` | |
| 错误 | `kError` | `0xd95c5c` | 新增 |
| 3D 背景 | — | `0x2b2b2e` | 视口与**非透明**导出（Q5）；透明导出仍 `0x00000000` |
| 取景框外 | — | `0x141414` × 90% | letterbox 暗幕 |
| 网格主线 | — | `0x5a5a60` | |
| 网格次线 | — | `0x3c3c42` | |
| X 轴 | — | `0xb04a4a` | 降饱和 |
| Z 轴 | — | `0x4a6ab0` | 降饱和 |

几何：间距 4 / 8 / 12 / 16；`WindowPadding` 12；`FramePadding` 8×4；`ItemSpacing` 8×6；圆角 4；按钮高 28；主按钮高 32。

`CollapsingHeader` / 段头：透明底 + caption 灰字 + 底部分隔线，不用 `ImGuiCol_Header` 填蓝。

Dock：单窗口节点 `ImGuiDockNodeFlags_AutoHideTabBar`。

## 四、字号层级（U0 · UIC-08）

只 **AddFont 一次**（系统 CJK，如 `msyh.ttc`）。启动读 `glfwGetWindowContentScale`，以 `body * scale` 为默认尺寸，并 `style.ScaleAllSizes(scale)`。其余档在绘制处用 ImGui 1.92 动态字号：`ImGui::PushFont(nullptr, sizePx * scale)`（`src/UI/UiFonts.h`；`scale` 取 `style.FontSizeBase / 14`，不要把 `GetFontSize()` 传给 `PushFont`）。跨屏 DPI 变化本版不处理。

| 名 | 100% DPI 像素 | 用途 |
|----|---------------|------|
| `caption` | 12 | 段头、图例、卡片元信息 |
| `body` | **14** | 默认 UI（从 18 降下，Q2） |
| `title` | 16 | 检查器面孔标题、开始板小标题 |
| `display` | 20 | 顶栏当前镜头名、开始板主标题 |
| `editor` | 16 | 剧本正文 |

图标用 MergeMode 叠第二套（U2 · UIC-33）。不往 atlas 预载 4 个尺寸。

## 五、图标清单（U2 · UIC-33）

资源：Lucide 子集 TTF（**ISC**，记入 `../03`）。只收本表字形，放 `assets/fonts/lucide-dd.ttf`（落地时路径可微调，须在 `03` 同步）。**不是** vcpkg / CMake 新目标。

| 用途 | Lucide 名（建议） |
|------|-------------------|
| 编剧 / 置景 / 掌机 / 审片 | `book-open` · `box` · `video` · `layout-grid` |
| 镜头 / 场次 / 对象 / 相机 / 资产 | `clapperboard` · `layers` · `box` · `camera` · `package` |
| 机位 · 预览 · 导出（三点） | `camera` · `image` · `download` |
| 搜索 / 加号 / 更多 | `search` · `plus` · `ellipsis` |
| 眼睛开 / 关 | `eye` · `eye-off` |
| 复制 / 删除 / 复位 / 聚焦 | `copy` · `trash-2` · `rotate-ccw` · `focus` |
| 网格 / 三分线 | `grid-3x3` · `columns-3` |
| 成功 / 警告 / 错误 | `circle-check` · `triangle-alert` · `circle-x` |
| 打开文件夹 / 上一 / 下一 | `folder-open` · `chevron-left` · `chevron-right` |
| 折叠 / 展开 | `chevron-right` · `chevron-down` |
| 下载中 / 导入 | `download` · `file-input` |

U0 无图标字体时：三点状态与模式仍用文字 / 现有几何，不引入第二套 Unicode 花字符。

## 六、视口尺寸契约（U0 · UIC-01）与底栏

```text
available = GetContentRegionAvail()          // 含 letterbox 的整个中央
frame = 按 exportResolution 宽高比内接 available 的最大矩形，居中
ViewportResizeCommand { frame.w, frame.h }   // 仍 2px / 首帧阈值
AddImage 只画在 frame 内
InvisibleButton 覆盖整个 available（暗幕也可拖轨道）
```

视图菜单关闭「锁定导出比例」时：退回「RT = available」（0.1.2 行为），便于对照。

`镜头条###ShotStrip` 不是 dock 窗口：`BeginViewportSideBar(viewport, ImGuiDir_Down, 132.0f)`。`ApplyDockLayout` 不再切 `dockBottom`。状态栏同样是 Down SideBar（28px），视觉贴在镜头条之下。设计基准 1280×800。

## 七、持久化

| 项 | 决定 |
|----|------|
| 取景框开关、网格、三分线、左栏折叠、3D 背景选项 | U0–U1：**进程内**。UIC-34 写入用户数据目录（非 `.ddproj`） |
| `workspaceModeId` / 选择 | 仍不进工程 |
| `.ddproj` | **不升版本** |
| Lucide TTF | 仓库资源，ISC 声明在 `03` |

## 八、不要顺手做的事

- 不要第 3 个新 Command。逼出来先回 [`44`](44-LANDING-CHECKLIST.md)。
- 不要在 UI 里用 FOV 算焦距或用标题匹配镜头。
- 不要改八个区域 ID 或任何 `###` 后缀。
- 不要为图标引入 vcpkg 包或 Web 字体运行时下载。
- 不要把用户设置写进 `.ddproj`。
- 不要在 U0 改 `SelectionKind` **枚举**或加 `previewTexture`。U0 只给已有字符串 `selectionKind` 增加取值 `"asset"`。
