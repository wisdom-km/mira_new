# Lucide 子集（UIC-33）

`lucide-dd.ttf` 是 [Lucide](https://lucide.dev) 图标字体的 40 个字形子集，给 Dear ImGui `MergeMode` 用。不是 vcpkg 包，也不是运行时下载。

- 上游：`lucide-static` 图标字体（2026-09-13 自 unpkg `@latest` 子集）
- 许可证：**ISC**（Lucide）。表内部分字形源自 Feather，那些图标另见 Feather **MIT**。全文见 <https://lucide.dev/license>
- 落地：`Paths::UiIconFontFile()` 先找可执行文件旁 `fonts/lucide-dd.ttf`，再向上找仓库 `assets/fonts/lucide-dd.ttf`
- 构建：现有 `DirectorDesk` `POST_BUILD` 拷到 exe 旁 `fonts/`，不新建 CMake 目标

## 字形

`book-open` `box` `video` `layout-grid` `clapperboard` `layers` `camera` `package` `image` `download` `search` `plus` `ellipsis` `eye` `eye-off` `copy` `trash-2` `rotate-ccw` `focus` `grid-3x3` `columns-3` `circle-check` `triangle-alert` `circle-x` `folder-open` `chevron-left` `chevron-right` `chevron-down` `file-input` `refresh-cw` `x` `save` `chevron-up` `check` `sliders-horizontal` `film` `upload` `scan` `link` `settings`

码点与 UTF-8 转义见 `src/UI/UiIcons.h`。
