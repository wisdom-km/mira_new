# 分镜 Skill（DirectorDesk 示例）

在资源库选中后点「运行 Skill」：软件会用检查器里保存的 **OpenAI 兼容文本模型**（地址 + 密钥），根据**当前剧本**生成 `storyboard-import.json` 并追加导入。

需要在检查器 AI 区填写 HTTPS 地址、密钥和文本模型名（如 `gpt-4o-mini`、`deepseek-chat`）。选「本地模拟」则不联网，写一份演示 JSON。

也可以把本文件夹交给 Claude Code / Codex 等 Agent 当说明文档。

## 产出

最后一步必须是 DirectorDesk 导入格式：

- `format`: `DirectorDeskStoryboardImport`
- `formatVersion`: `1`
- `scenes[].shots[]` 含 `id` / `title` / `body`，可选 `meta`

字段说明见仓库 `docs/dev-map/modules/storyboard-import.md`。运行描述见 `docs/dev-map/modules/skill-run.md`。

## 不要做

- 不要把 Skill 拖进 3D 场景（它不是模型）
- 不要在 Skill 里起未经 `skill.json` 声明的进程
