# 分镜导入契约

版本：1（FOUNDATION F3）

## 一、目标

把外部工具产出的 `storyboard-import.json` 转成符合 `script-format` 1.1 的 Markdown。落点 `Script::ImportStoryboard`；`dd_script` 私有链接 nlohmann，公共头不出现 json 类型。运行 Skill 落在 AI 模块（见 `skill-run.md`）。

## 二、JSON 示例

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

## 三、规则

- `format` 必须严格相等；`formatVersion` 只接受 `1`
- `scenes[].id` / `shots[].id` 可选；缺失或不合法（不满足 `[a-z0-9][a-z0-9_-]{0,63}`）时生成 `scene-<短码>` / `shot-<短码>` 并给提示级诊断。`append` 模式下与现有剧本冲突的 ID 同样重生成并诊断
- `title` 缺失 → 「未命名」+ 警告；`body` 缺失 → 空
- `meta` 是对象，键值都转为字符串；保持 JSON 里的键顺序
- 生成的 Markdown 严格符合 `script-format` 1.1：`# title` → `## [scene:id] title` → 正文 → `### [shot:id] title` → `> k: v` 行 → 空行 → 正文。`append` 模式省略文档标题行
- 未知字段忽略；单个镜头非法不影响其他镜头，但根对象非法整体拒绝
- 导入来源 `source.tool` 不持久化，只出现在 status 与诊断

## 四、模式

| mode | 行为 |
|------|------|
| `replace` | 用生成文本整体替换当前剧本 |
| `append` | 把生成的场次接到当前剧本文本末尾 |

当前工程（或剧本）dirty 且 `replace` 时，先走未保存工程的三按钮提示。

## 五、最小测试集

- 完整 round-trip（导入 → 解析 → 与 JSON 结构一致）
- 缺 ID
- 非法 ID
- `append` 冲突
- 中文键
- 空 `scenes`
- `formatVersion: 2` 拒绝
