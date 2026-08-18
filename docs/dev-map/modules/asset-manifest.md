# 官方资产清单契约

格式版本：1（P0）

## 一、信任边界

- 应用只读取编译配置中固定的官方 HTTPS 清单地址和资源基地址
- P0 不提供修改地址、添加源或上传资源的 UI/API
- 清单中的 URL 必须是相对于官方资源基地址的相对路径
- 禁止 `..`、绝对路径、反斜杠和路径穿越；最终 URL 必须仍属于允许的 HTTPS 主机
- 每个下载文件必须提供 SHA-256 和字节数，验证成功后才进入缓存
- 清单下载成功但校验/解析失败时，继续使用最后一个有效缓存

官方仓库和发布地址在 Phase 8 开始前由项目负责人确定，记录到 `03-CURRENT-STATUS.md`；不得由实现者自行臆造。

## 二、清单示例

```json
{
  "schemaVersion": 1,
  "revision": "2026-08-19.1",
  "generatedAt": "2026-08-19T00:00:00Z",
  "categories": [
    { "id": "character", "name": { "zh-CN": "角色", "en": "Characters" } },
    { "id": "scene", "name": { "zh-CN": "场景", "en": "Scenes" } },
    { "id": "prop", "name": { "zh-CN": "道具", "en": "Props" } }
  ],
  "assets": [
    {
      "id": "basic-chair",
      "version": "1.0.0",
      "name": { "zh-CN": "基础椅子", "en": "Basic Chair" },
      "description": {
        "zh-CN": "低多边形基础椅子。",
        "en": "A basic low-poly chair."
      },
      "category": "prop",
      "tags": ["chair", "furniture"],
      "format": "glb",
      "entrypoint": "model/chair.glb",
      "preview": "preview/cover.png",
      "files": [
        {
          "path": "model/chair.glb",
          "url": "assets/basic-chair/1.0.0/chair.glb",
          "sha256": "64个小写十六进制字符",
          "size": 123456
        },
        {
          "path": "preview/cover.png",
          "url": "assets/basic-chair/1.0.0/cover.png",
          "sha256": "64个小写十六进制字符",
          "size": 12345
        }
      ],
      "license": {
        "spdx": "CC0-1.0",
        "name": "CC0 1.0 Universal",
        "url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "attribution": ""
      },
      "author": {
        "name": "DirectorDesk Community",
        "url": "https://example.invalid"
      }
    }
  ]
}
```

示例 URL 为结构占位，不代表最终官方地址。

## 三、字段约束

### 根对象

| 字段 | 类型 | 规则 |
|------|------|------|
| `schemaVersion` | integer | P0 只接受 `1`；更高版本拒绝并使用旧缓存 |
| `revision` | string | 清单修订号，用于更新检测，不作为排序时间 |
| `generatedAt` | RFC 3339 string | 展示和诊断用途 |
| `categories` | array | Category ID 唯一 |
| `assets` | array | `(id, version)` 唯一 |

### Asset

| 字段 | 规则 |
|------|------|
| `id` | `[a-z0-9][a-z0-9_-]{0,63}`，稳定且不可复用 |
| `version` | 合法 SemVer；内容变化必须升版本 |
| `name` | 至少有 `zh-CN` 或 `en` |
| `description` | 可本地化纯文本，UI 不按 HTML 渲染 |
| `category` | 必须引用已声明 Category ID |
| `tags` | 小写搜索关键字，去重 |
| `format` | P0 仅 `glb` 或 `obj` |
| `entrypoint` | 必须引用 `files[].path`，扩展名与 `format` 一致 |
| `preview` | 必须引用 PNG/JPEG/WebP 文件；预览失败不阻止模型使用 |
| `files` | 至少一个；OBJ 的 MTL/纹理作为独立文件列出并保持相对目录 |
| `license` | 必填；P0 官方准入为 `CC0-1.0` 或经过审核的 `CC-BY-4.0` |
| `author` | 必填；CC-BY 时 `attribution` 不得为空 |

### File

- `path`：资产缓存根目录下的 POSIX 相对路径
- `url`：官方资源基地址下的 URL 相对路径
- `sha256`：文件内容 SHA-256，小写十六进制
- `size`：非负整数；下载前用于空间检查与进度，实际字节数必须一致

未知字段必须忽略，以允许兼容扩展；缺失必填字段则拒绝对应资产。单个资产非法不应使全部合法资产不可用，但清单根对象非法必须整体拒绝。

## 四、缓存布局与原子性

```text
<用户数据目录>/DirectorDesk/assets/
├── manifests/
│   ├── current.json
│   └── previous.json
├── official/
│   └── <asset-id>/<version>/<files.path>
└── temp/
    └── <download-id>.part
```

下载流程：

1. 校验清单和目标路径；
2. 检查可用空间；
3. 下载到 `temp`，支持取消；
4. 校验实际字节数与 SHA-256；
5. 创建版本目录并原子移动；
6. 全部必需文件成功后，才把该版本标记为“已下载”；
7. 失败时清理临时文件，不破坏旧版本。

缓存按 `id + version` 不可变。P0 不自动删除旧版本，避免破坏已有 `.ddproj` 引用。

## 五、下载状态

统一状态：`NotDownloaded`、`Queued`、`Downloading`、`Verifying`、`Ready`、`Failed`、`Cancelled`。

失败至少区分：网络不可达、超时、HTTP 错误、磁盘空间不足、写入失败、大小不符、哈希不符、清单非法、用户取消。

## 六、最小测试集

- 合法 GLB 与多文件 OBJ 资产
- 根版本过高、未知字段、缺失字段
- 重复 ID/version、非法 SemVer、非法哈希
- `../`、绝对路径、主机跳转等路径攻击
- 下载中断、大小不符、哈希不符
- 旧缓存可用、清单更新失败
- 中文本地化与无中文回退到英文
