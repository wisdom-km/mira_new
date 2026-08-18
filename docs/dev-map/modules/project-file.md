# DirectorDesk 工程文件契约（`.ddproj`）

格式版本：1（P0）

## 一、基本规则

- `.ddproj` 是 UTF-8 JSON 文本，便于版本控制与人工诊断
- 单位为米，右手坐标系，Y 轴向上
- 旋转使用四元数 `[x, y, z, w]`
- 所有持久对象使用稳定 ID；禁止持久化内存地址或数组下标
- 保存采用“同目录临时文件 → flush/关闭 → 解析验证 → 原子替换”
- 打开失败不得改变当前内存中的工程

## 二、版本 1 示例

```json
{
  "format": "DirectorDeskProject",
  "formatVersion": 1,
  "createdBy": "DirectorDesk 0.1.0",
  "projectId": "proj-550e8400-e29b-41d4-a716-446655440000",
  "name": "咖啡馆短片",
  "script": {
    "path": { "kind": "project-relative", "value": "script/story.md" }
  },
  "assets": [
    {
      "refId": "assetref-chair",
      "source": "official",
      "assetId": "basic-chair",
      "version": "1.0.0",
      "entrypoint": "model/chair.glb"
    },
    {
      "refId": "assetref-room",
      "source": "project",
      "path": "assets/room/room.obj",
      "sha256": "64个小写十六进制字符"
    }
  ],
  "scene": {
    "nodes": [
      {
        "id": "node-chair-01",
        "name": "椅子",
        "assetRef": "assetref-chair",
        "parent": null,
        "transform": {
          "position": [0.0, 0.0, 0.0],
          "rotation": [0.0, 0.0, 0.0, 1.0],
          "scale": [1.0, 1.0, 1.0]
        },
        "visible": true
      }
    ]
  },
  "cameras": [
    {
      "id": "camera-main",
      "name": "主镜头",
      "projection": "perspective",
      "position": [0.0, 1.6, 5.0],
      "rotation": [0.0, 0.0, 0.0, 1.0],
      "verticalFovDegrees": 45.0,
      "nearPlane": 0.01,
      "farPlane": 1000.0,
      "orbitTarget": [0.0, 1.0, 0.0],
      "preset": "front"
    }
  ],
  "activeCamera": "camera-main",
  "shotLinks": [
    {
      "shotId": "shot-cafe-001",
      "cameraId": "camera-main"
    }
  ],
  "lighting": {
    "preset": "studio-basic"
  },
  "storyboard": {
    "layout": "left-to-right",
    "collapsedScenes": ["scene-street-night"]
  }
}
```

示例哈希为占位文字；实际文件中必须是合法 SHA-256。

## 三、路径与资产引用

### 路径对象

```json
{ "kind": "project-relative", "value": "script/story.md" }
```

`kind` 只有：

- `project-relative`：相对于 `.ddproj` 所在目录，优先使用
- `absolute`：UTF-8 绝对路径，只允许引用项目目录外的用户文件，UI 必须提示不可移植

规则：

- 写入时规范化为 `/` 分隔；读取后由 Platform 转换
- 项目相对路径不得逃逸工程目录
- 禁止静默依赖进程当前工作目录
- 用户“另存为”时，所有 project-relative 路径按新工程位置重新计算

### Asset Reference

`source` 支持：

1. `project`：模型位于工程目录，保存相对 `path` 和 `sha256`，可随工程移动
2. `user-library`：保存稳定 `assetId`、内容哈希，可选原始绝对路径作为定位提示
3. `official`：保存官方 `assetId + version + entrypoint`，必须精确版本，不自动升级

解析顺序：

- project：只在工程目录解析
- user-library：按本地索引的 ID 和内容哈希解析；找不到则标记缺失
- official：按缓存的精确版本解析；未下载时可在 Phase 8 后提示下载

缺失资产不阻止工程打开；对应节点显示占位盒并记录可修复错误。

## 四、字段约束

- `format` 必须严格等于 `DirectorDeskProject`
- `formatVersion`：P0 只支持 `1`
- `projectId`、Node/Camera/AssetRef ID 在各自域中唯一
- Transform、相机数值必须是有限数；scale 各分量不得为 0
- Quaternion 读取后归一化；长度接近 0 则拒绝该对象
- `nearPlane > 0`，`farPlane > nearPlane`
- `verticalFovDegrees` 限制在 `(1, 179)`
- `parent` 必须引用现有 Node 且场景图不得成环
- `activeCamera` 可为 `null`，否则必须引用现有 Camera
- 每个 Shot 最多关联一个 Camera；悬空 Shot/Camera Link 保留为诊断但不激活
- `storyboard.layout` 在 P0 只接受 `left-to-right`
- `storyboard.collapsedScenes` 只保存合法 Scene ID；悬空 ID 在加载时忽略并诊断
- 分镜节点坐标、缩放、平移和缩略图是可重建数据，不得写入工程文件
- 未知字段忽略；缺少可选字段使用文档默认值

## 五、兼容性与迁移

- 低于当前版本：通过显式迁移函数逐版升级，迁移前保留原文件
- 高于当前版本：拒绝打开并提示升级应用
- 不允许根据字段猜测版本
- 迁移必须有固定输入/输出测试，且结果再次通过当前 schema 验证
- P0 没有旧版本迁移，但必须保留版本分发结构

## 六、脏状态与保存

以下操作将工程标记为 dirty：场景、相机、灯光、镜头关联、资产引用、剧本路径、分镜场次折叠状态或项目名变化。

- 关闭、打开其他项目或退出时，如 dirty 必须提示保存/放弃/取消
- 视口大小、面板布局、搜索关键字不属于工程数据，存入用户设置或不持久化
- 自动保存不在 P0；禁止声称有自动恢复能力

## 七、最小测试集

- 版本 1 完整 round-trip
- 中文名称与中文/空格路径
- 工程目录整体移动后的相对路径解析
- 缺失资产、缺失剧本、悬空关联
- 重复 ID、场景图成环、非法数值
- 未知字段、过高版本、损坏 JSON
- 模拟保存中断，原工程保持完整
