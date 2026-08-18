# P0 发布检查清单

完成下列全部项后再打 `phase-10-p0`。

## 范围

- [ ] `docs/dev-map/00` 列出的 P0 功能均已实现
- [ ] `00` 明确不做项均未进入菜单或文档推荐路径
- [ ] 无供应商 SDK、密钥 UI、真实 AI 网络调用
- [ ] 无自定义第三方资产源、无软件内上传

## 文档

- [ ] README 反映当前 Phase，并能按文档构建
- [ ] BUILD / USER-GUIDE / CONTRIBUTING / THIRD_PARTY 与代码一致
- [ ] 示例工程 `examples/cafe.ddproj` 可打开

## Windows（必须本机过）

- [ ] Debug 构建与 `DirectorDeskTests` 全绿
- [ ] 打开示例工程：剧本、立方体、分镜画布可见
- [ ] 中文路径下保存工程、导出 1080p PNG
- [ ] 资源库「在线」可刷新清单；断网时不崩溃
- [ ] 快捷键 Ctrl+S / Ctrl+E 可用（焦点不在文本框时）
- [ ] 导出 PNG 不含菜单、选框和地面格网

## macOS

- [ ] GitHub Actions `macOS` job 构建与测试通过
- [ ] 有条件时再做一次实机：启动、打开示例工程、导入、保存、导出

## 仓库卫生

- [ ] 未提交 `build/`、`vcpkg_installed/`、密钥或生成 shader
- [ ] 无无归属 TODO / 临时代码开关
- [ ] CI Windows + macOS 全绿
