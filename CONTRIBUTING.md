# 贡献指南

感谢关注 DirectorDesk。本仓库用开发地图约束范围和架构，避免跨贡献者漂移。

## 开始之前

1. 按顺序阅读 `docs/dev-map/` 中的 `00` 到 `05`，以及当前 Phase 对应的 `modules/*.md`。
2. 确认 `03-CURRENT-STATUS.md` 中的当前 Phase。
3. 只实现当前 Phase 允许的工作，不要提前做后续功能。

## 开发规则

- 核心用户路径优先
- 模块依赖单向；UI 只发 Command、只读展示状态
- 公共接口不泄漏 GLFW、bgfx、libcurl、spdlog 类型
- 内部字符串一律 UTF-8
- 新增行为要有测试，失败路径要有日志

## 提交

- 使用清晰的英文或中文提交说明
- 不要提交密钥、`build/`、`vcpkg_installed/` 或生成的 shader 二进制
- Phase 完成 tag 由维护者在验收后创建

官方资产通过独立 GitHub 仓库的 Pull Request 提交，不在本应用内上传。
