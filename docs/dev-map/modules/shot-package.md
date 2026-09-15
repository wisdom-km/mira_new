# 镜头包契约

版本：1（FOUNDATION F2）

## 一、目标

把一个镜头的画面、剧本、相机与可见场景节点打成下游可读取的一对文件：`<shot-id>.png` 与 `<shot-id>.shot.json`。Export 只接收 App 装好的值对象，不 include Script / Scene / Camera。

## 二、文件

`Export::WriteShotPackage` 写到用户选择的目录：

| 文件 | 内容 |
|------|------|
| `<shot-id>.png` | 与现有单镜头离屏导出相同的 PNG |
| `<shot-id>.shot.json` | 本契约 JSON |

## 三、JSON 示例

```json
{
  "format": "DirectorDeskShotPackage",
  "formatVersion": 1,
  "generatedBy": "DirectorDesk 0.1.3",
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

## 四、规则

- `format` 必须为 `DirectorDeskShotPackage`；`formatVersion` 为 `1`
- `prompt` / `negativePrompt` 是 `meta` 中 `提示词` / `负面提示词` 的副本；无则为空串
- 焦距以 24mm 画幅高为基准：`focal = 12 / tan(vfov_rad / 2)`；`hfov = 2 · atan(aspect · tan(vfov_rad / 2))`。纯数学，在 Export 内完成
- `sceneNodes` 只列可见节点；`assetId` 对官方资产是清单 `id`，对本地资产是索引 ID
- 不含深度图 / 法线图（v2 议题）
- `generatedBy` 读 CMake `PROJECT_VERSION`，形如 `DirectorDesk 0.1.3`

## 五、分镜总览索引

`ExportStoryboardBoardCommand` 成功写出 PNG 后，在同目录写 `board.json`：

```json
{
  "format": "DirectorDeskBoardIndex",
  "formatVersion": 1,
  "image": "project-storyboard.png",
  "shots": [
    { "id": "shot-cafe-001", "title": "过肩", "image": "project-storyboard.png", "metaLine": "中景 · 推 · 3s" }
  ]
}
```

每格对应 `id` / `title` / `image` / `metaLine`。总览是单张 PNG，各格 `image` 指向该 PNG 文件名。

## 六、最小测试集

- 固定 `ShotPackageInput` 写出 JSON，字段与焦距公式一致
- `verticalFovDegrees = 45`、`aspect = 16/9` 时 `focalLength35mmEquivalent` 与公式相符
- 隐藏节点不出现在 `sceneNodes`
- `board.json` 格数与镜头卡一致
