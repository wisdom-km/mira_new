# 01 - 架构地图（ARCHITECTURE MAP）

> 本文件定义模块边界、依赖方向与关键技术契约。违反本文件即为架构漂移，禁止。

## 一、技术栈（锁定，含具体库选型）

| 层级 | 选型 | 说明 |
|------|------|------|
| 语言 | C++17 | 锁定标准，不用 C++20 特性（保证工具链兼容） |
| UI | Dear ImGui（docking 分支） | vcpkg feature: docking-experimental |
| 渲染 | bgfx + IRenderer 抽象 | shader 用 bgfx shaderc 编译，见下文 |
| 窗口/输入 | GLFW | |
| 数学库 | glm | |
| GLB 加载 | **cgltf** | 轻量 C 库 |
| OBJ 加载 | **tinyobjloader** | |
| 图片编解码 | **stb_image / stb_image_write** | 预览图加载 + PNG 导出 |
| JSON | **nlohmann-json** | 资产清单、工程文件 |
| 网络 | **libcurl**（HTTPS） | 官方资产库下载 |
| 完整性校验 | **picosha2** | 官方资产 SHA-256 校验 |
| 日志 | **spdlog**（封装在 Core::Log） | 业务代码禁止直接 include spdlog |
| 测试 | **Catch2 v3** | |
| 构建 | CMake ≥ 3.24 + vcpkg **manifest 模式** | `vcpkg.json` + builtin-baseline 锁版本 |
| CI | GitHub Actions | Windows + macOS 矩阵构建 + 测试 |
| 平台 | Windows（优先） / macOS | |

**shader 管理**：bgfx shader 源码放 `shaders/`，CMake 自定义命令调用 shaderc 在构建期编译到输出目录，禁止手工编译后提交二进制 shader。

## 二、目录结构（强制，全小写顶层目录）

```text
DirectorDesk/
├── CMakeLists.txt
├── vcpkg.json
├── cmake/                  # CMake 模块（shaderc 编译规则等）
├── include/DirectorDesk/   # 公共接口头文件
│   ├── Core/     Platform/  Scene/   Asset/
│   ├── Camera/   Renderer/  Export/  Script/
│   ├── Link/     Storyboard/ AI/      UI/      App/
├── src/                    # 与 include 一一对应的实现
├── backends/               # 第三方后端胶水
│   ├── bgfx/  imgui/  glfw/  curl/
├── shaders/
├── tests/
├── assets/                 # 内置默认资源；用户缓存在系统目录，不在仓库
├── examples/
├── docs/
│   └── dev-map/
├── .github/workflows/
├── LICENSE  README.md  CONTRIBUTING.md
```

## 三、模块职责与依赖方向

依赖方向**单向、自下而上**，禁止反向或横向环：

```text
App ──▶ UI, Script, Asset, Scene, Camera, Link, Storyboard, Export, AI, Renderer
UI  ──▶ Core（仅 Command / 只读 State，零业务逻辑）
Script / Asset / Scene / Camera / Link / Storyboard / Export / AI ──▶ Core, Platform
Renderer(接口) ──▶ Core；backends/bgfx 实现 IRenderer
Platform ──▶ Core
Core ──▶ （无内部依赖）
```

| 模块 | 职责 |
|------|------|
| Core | 日志封装、事件、Command 定义、通用工具、UTF-8 处理 |
| Platform | 窗口、输入、文件对话框、**路径（UTF-8 ↔ 平台编码转换唯一入口）**、`IHttpClient`、线程池 |
| Scene | 简化场景节点（名称、Transform、模型引用）、场景图 |
| Asset | 本地资源扫描/导入、模型加载器注册表（IModelLoader，可扩展）、官方在线资产库（清单/下载/缓存） |
| Camera | 轨道相机、预设机位、多相机管理 |
| Renderer | IRenderer 接口；实时视口渲染 + 离屏渲染 |
| Script | Markdown 剧本解析（Scene/Shot）、编辑 |
| Link | 镜头 ↔ 相机关联 |
| Storyboard | 从 App 提供的不可变快照生成 Scene/Shot 图、确定性自动布局、卡片状态和可重建缩略图缓存调度 |
| Export | 单镜头离屏渲染、透明 PNG、分辨率选择；数据驱动的完整分镜总览 PNG 输出 |
| AI | IImageGenService / IVideoGenService 纯接口，P0 无实现 |
| UI | 全部 ImGui 面板（IPanel），含分镜画布交互视图；只发 Command、只读 State |
| App | 唯一主循环与生命周期管理者、Command 分发、工程文件读写编排、跨模块快照组装与选择同步 |

## 四、关键契约

### 1. Command 系统（UI ↔ 业务的唯一通道）

- Command 是**值对象**（struct + `std::variant` 或基类），由 UI 构造后 push 进 `CommandQueue`
- App 在主循环中**同步、顺序**消费队列并调用对应业务模块；UI 不直接调用业务模块
- Command **无返回值**；执行结果通过 State 更新反映给 UI
- UI 读取状态：`UI::AppViewState` 是只含展示数据的快照 DTO，由 App 每帧填充并以 const 形式交给 UI；UI 禁止持有业务对象引用
- P0 **不实现 undo/redo**，但 Command 保持可序列化的简单结构，为将来预留

### 2. IRenderer 边界

最小接口草案（Phase 1 落地时可微调签名，不得改变职责边界）：

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Init(const RendererInitDesc& desc) = 0;      // 接管原生窗口句柄
    virtual void Shutdown() = 0;
    virtual void BeginFrame(uint32_t width, uint32_t height) = 0;
    virtual void RenderScene(const RenderSceneView& scene, const CameraView& view,
                             const RenderTargetDesc& target) = 0;  // target 可为默认帧缓冲或离屏 FBO
    virtual bool ReadbackTarget(const RenderTargetDesc& target,
                                PixelBuffer& out) = 0;         // 离屏像素回读（含 alpha），Export 依赖
    virtual void EndFrame() = 0;
};
```

- ImGui 的 bgfx/GLFW 渲染后端属于 `backends/imgui/`，不属于 IRenderer 职责
- `RenderSceneView` 是 Renderer 模块定义的只读提交快照；App 从业务 Scene 组装它，Renderer 不依赖 Scene 模块
- Export 模块只依赖 `RenderScene(离屏) + ReadbackTarget`，不接触 bgfx

### 3. 模型加载器（可扩展）

```cpp
class IModelLoader {
public:
    virtual ~IModelLoader() = default;
    virtual bool CanLoad(const std::string& extension) const = 0;
    virtual LoadResult Load(const Path& file, ModelData& out) = 0;
};
// Asset 模块维护 LoaderRegistry；新格式 = 新增一个 IModelLoader 实现 + 注册，零改动调用方
```

### 4. Storyboard 边界

- Storyboard 定义不含业务对象引用的 `StoryboardSourceSnapshot`，由 App 从 Script、Link、Camera 和导出状态组装
- Storyboard 输出只读布局/卡片快照；UI 负责绘制和发送 Command，不计算业务状态或修改剧本
- 画布结构只由 Script 快照驱动；Storyboard 不直接依赖 Script、Link、Camera、Scene 或 Renderer
- App 在主线程调度缩略图离屏渲染，并把结果作为值对象交回 Storyboard；Storyboard 不直接调用 Renderer
- App 为 Export 组装数据驱动的分镜导出快照；Export 不截取 UI 窗口，也不依赖 Storyboard 具体布局实现对象

### 5. 线程模型

- **主线程**：GLFW 事件、ImGui、渲染、Command 消费——全部业务状态只在主线程读写
- **后台线程池**（Platform 提供）：仅用于资产下载与大文件模型加载
- 后台任务完成后将结果（成功/失败 + 数据）push 进**线程安全的结果队列**，由主线程在每帧开头取出并应用；后台线程**禁止**直接触碰任何业务状态
- 除上述两条队列外，禁止新增跨线程共享可变状态

### 6. UTF-8 约定

- 全项目内部字符串一律 UTF-8 `std::string`
- Windows API 交互（宽字符路径等）全部封装在 Platform 模块内，其他模块禁止出现 `wchar_t` / `_wfopen` 等平台细节
- 必须用含中文的路径与文件名做测试用例（目标用户是中文环境）

### 7. 数据格式契约（详见 modules/）

- 剧本 Markdown 语法：`modules/script-format.md`
- 分镜画布同步与交互：`modules/storyboard-canvas.md`
- 资产清单 JSON schema：`modules/asset-manifest.md`
- 工程文件 `.ddproj`：`modules/project-file.md`

## 五、日志规范

- 统一入口 `DD_LOG_TRACE/DEBUG/INFO/WARN/ERROR`（Core 封装 spdlog）
- 同时输出控制台与文件（`<用户数据目录>/DirectorDesk/logs/`，按日期滚动）
- ERROR 仅用于用户可感知的失败；正常流程用 INFO 以下
