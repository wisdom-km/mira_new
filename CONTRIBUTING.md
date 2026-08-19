# 贡献指南

感谢关注 DirectorDesk。本仓库用开发地图约束范围和架构，避免跨贡献者漂移。

## 开始之前

1. 按 [docs/BUILD.md](docs/BUILD.md) 完成 Windows 或 macOS 构建
2. 按顺序阅读 `docs/dev-map/` 中的 `00` 到 `05`、`07`，以及当前工作对应的 `modules/*.md`。讨论架构或加删功能时加读 `08`
3. 确认 `docs/dev-map/03-CURRENT-STATUS.md` 中的当前状态
4. P0 已完成后遵守 `07` 的模块数锁定与功能落点；不要为小功能新建模块，也不要提前做未批准范围。AI 入口见根目录 [AGENTS.md](AGENTS.md)

## 开发规则

- 核心用户路径优先
- 新功能按名词落进现有模块；禁止为单个按钮或预设新建库
- 模块依赖单向；UI 只发 Command、只读展示状态
- 公共接口不泄漏 GLFW、bgfx、libcurl、spdlog 类型
- 内部字符串一律 UTF-8
- 新增行为要有测试，失败路径要有日志
- 提交前格式化改动文件；不要全库机械重排

## 提交

- 使用清晰的英文或中文提交说明
- 不要提交密钥、`build/`、`vcpkg_installed/` 或生成的 shader 二进制
- Phase 完成 tag 由维护者在验收后创建
- 不要对 `main` 强制推送

官方资产通过独立仓库 [obj-3d-models](https://github.com/wisdom-km/obj-3d-models) 的 Pull Request 提交，不在本应用内上传。
