# Skill 运行描述 `skill-run` 1

Skill 目录里与 `SKILL.md` 同级的可选 `skill.json`。有这份文件，资源库检查器才启用「运行 Skill」。没有则仍只展示说明并打开文件夹，交给外部 Agent。

```json
{
  "format": "DirectorDeskSkillRun",
  "formatVersion": 1,
  "output": "storyboard-import.json",
  "builtin": "openai-compat-json",
  "promptFile": "prompt.md"
}
```

| 字段 | 规则 |
|------|------|
| `format` | 必须严格等于 `DirectorDeskSkillRun` |
| `formatVersion` | 只接受 `1` |
| `output` | 相对 Skill 目录的导入 JSON 文件名；运行成功后 App 走既有导入路径（默认 `append`） |
| `builtin` | 可选。`copy-output`：拷贝目录内已有 `output`。`openai-compat-json`：用检查器里的文本模型 `POST {base}/v1/chat/completions`，把当前剧本文本发给模型，校验返回的 `DirectorDeskStoryboardImport` 后写入 `output`。`mock` 提供者不联网，写一份演示 JSON |
| `promptFile` | 可选，仅 `openai-compat-json`。相对目录的提示词文件（默认 `prompt.md`）。缺失则用内置分镜提示 |
| `argv` | 可选字符串数组。若存在且非空，在 Skill 目录起子进程（`Platform::RunProcess`），等待进程退出后再读 `output`。超时或非 0 退出码失败 |

未知字段忽略。根对象非法整体拒绝。落点 `AI::RunSkill`；不新建 CMake 模块。`openai-compat-json` 走 Worker，不嵌 Node、不接 MCP、密钥不进日志。

