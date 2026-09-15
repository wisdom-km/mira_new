# AI HTTP 适配（openai-compat）

> `format` 无独立文件头；这是 DirectorDesk 调用文生图 / 图生视频时的 **HTTPS JSON 约定**。
> 实现落在 `AI::ExecuteImageGeneration` / `AI::ExecuteVideoGeneration`。公共头不出现 curl / nlohmann 类型。
> 供应商无关：任何实现了下列路径的网关（OpenAI、硅基流动、NewAPI 等）都可以填进设置里的 API 地址。

## 配置（`settings.json`，不进 `.ddproj`）

| 字段 | 含义 | 默认 |
|------|------|------|
| `aiProvider` | `openai-compat` 或 `mock` | `openai-compat` |
| `aiBaseUrl` | HTTPS 根，无尾斜杠 | `https://api.openai.com` |
| `aiApiKey` | Bearer 密钥；禁止写入日志 | 空 |
| `aiImageModel` | 图像模型名 | `gpt-image-1` |
| `aiVideoModel` | 视频模型名 | `sora-2` |
| `aiChatModel` | 文本模型名（分镜 Skill `openai-compat-json`） | `gpt-4o-mini` |

`mock` 不发起网络：把当前镜头参考 PNG 拷到输出目录，用来走通 UI 与 Command。没有密钥时 `openai-compat` 拒绝提交。

## 图像 `POST {baseUrl}/v1/images/generations`

请求（`Content-Type: application/json`，`Authorization: Bearer {key}`）：

```json
{
  "model": "gpt-image-1",
  "prompt": "暖色午后光，浅景深",
  "size": "1024x1024",
  "n": 1,
  "response_format": "b64_json"
}
```

规则：

- 只允许 `https://` URL。
- 参考图必须是本地路径（禁止 `://`）。若存在，把文件读成 base64 放进可选字段 `image`（网关不认则忽略，仍按文生图）。
- `size` 映射：两边 ≤1024 → `1024x1024`；宽≥高 → `1536x1024`；否则 `1024x1536`。
- 成功响应取 `data[0].b64_json` 或 `data[0].url`。`url` 必须是 `https://`，再 `GET` 存到本地。输出路径不得含 `://`。
- 错误取 `error.message`，否则用 HTTP 状态。

## 视频 `POST {baseUrl}/v1/videos/generations`

请求：

```json
{
  "model": "sora-2",
  "prompt": "推镜进入咖啡馆",
  "seconds": 4,
  "size": "1280x720"
}
```

若响应带 `id` 且 `status` 为 `queued` / `in_progress` / `processing`，在工作线程 `GET {baseUrl}/v1/videos/{id}` 轮询（间隔 2s，可取消），直到 `succeeded` / `completed` 或失败。成品取 `data[0].url` 或 `output.url`，同样只下 HTTPS 到本地 `.mp4`。

## 分镜文本 `POST {baseUrl}/v1/chat/completions`

`skill.json` 的 `builtin: openai-compat-json` 走这条。请求：

```json
{
  "model": "gpt-4o-mini",
  "temperature": 0.2,
  "response_format": { "type": "json_object" },
  "messages": [
    { "role": "system", "content": "（prompt.md 或内置分镜提示）" },
    { "role": "user", "content": "（当前剧本文本）" }
  ]
}
```

成功取 `choices[0].message.content`，抽出 JSON 对象，校验 `DirectorDeskStoryboardImport` 后写入本地文件。不认视觉输入；这是文本 LLM，不是生图模型。

## 线程

`Submit` 在主线程只登记任务。HTTP 只在 `Platform::Worker` 上跑。结果经 `ResultQueue` 回主线程。后台不得改 Scene / UI / Renderer。
