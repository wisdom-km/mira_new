# Skill 作者接入指南

Skill 是官方资产的一种（`format: skill`）。DirectorDesk 下载或本机安装后展示 `SKILL.md`。若同级有合法 `skill.json`，检查器提供「运行 Skill」，产出仍是 `storyboard-import.json`。

## 三步接入

1. **写 `SKILL.md`**  
   放在一个目录里，入口文件名必须是 `SKILL.md`。前 40 行显示在检查器。

2. **（可选）写 `skill.json`**  
   让软件能运行。格式见 `docs/dev-map/modules/skill-run.md`。示例分镜 Skill 使用 `"builtin": "openai-compat-json"`，会调用检查器里的文本模型。

3. **生成清单条目**  
   ```text
   python tools/make-skill-manifest.py --id my-storyboard --version 1.0.0 --dir path/to/skill --category skill --license MIT --name-zh "分镜 Skill" --name-en "Storyboard Skill"
   ```
   向官方清单仓库提 PR。用户也可在资源库本地页「安装示例分镜 Skill」，或文件菜单选 `SKILL.md`。安装会把同目录文件拷进用户资源库。
