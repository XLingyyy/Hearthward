# TASK-075 仓库整理交接

- 用户来源：2026-10-05要求整理Hearthward文件；本地整理授权。
- 基线：`ac6a302ba24ea6531ccdaf880d83e9a03779c51e`，PR #60后的main。
- 原工作分支：`codex/TASK-075-repository-organization`；整理补丁随后完整带入 `codex/TASK-076-ui-gameplay-integration`，恢复点保留在本地stash。
- 本单整理阶段状态：当时仅本地完成，未运行UE。后续整理补丁完整带入TASK-076，用户已授权一并提交、推送并更新main；整合验证见TASK-076交接。

## 结果

根目录Resource、ui pic和日期目录中的23个受跟踪文件已整理：15个迁移、3个不同元数据另名保留、5个与既有制作源逐字节相同的文件归并。模型和图片内容未改；ZIP的局部LFS规则随源目录迁入。未触碰Source、Content、Config、Resources、Runtime及其他工作树。

更新README、START_HERE、PROJECT_STATE与AGENTS的过时入口；增加目录、制作源、QA和分支整合索引。新增本地IDE生成文件忽略项。修订TASK-020视觉比对脚本的一处参考目录，保持历史结果文件原样。

main现含053—074玩法批次，新版UI仍在`codex/TASK-053-title-wheel@fa1828ed`；14处合并冲突和TASK-053编号重复记录于分支整合清单。本单整理阶段没有修改或合并该分支；后续功能整合见[TASK-076](TASK-076.md)，该处冲突清单是整合前记录。

## 验证

- `python scripts/validate_repo.py`：PASS，76份任务快照，0错误。按工具原有规则检查；只证明仓库文档/元数据检查通过。
- 改动文档相对链接、任务JSON及本次变更路径范围：PASS。
- 15个迁移或归并后的ZIP/PNG/FBX路径LFS属性：全部为lfs。
- TASK-020比对脚本语法与其引用的8张原图解码：PASS；未执行比对计算，避免覆盖历史受测结果。
- Git diff空白检查：PASS。
- UE构建／游戏运行：NOT_RUN；本次无游戏实现及运行时路径变更。
- TASK-075在当前main没有已批准的任务快照，因此未将`--task --base`检查冒充通过；范围依据本会话用户授权的本地任务单，使用现有path_allowed/collect_scope_changes做明确检查。

验证临时输出位于`.agent-local/repository-organization/`，不作为游戏内容或团队唯一交付。

## 文件映射

| 原路径 | 当前路径 | 处理 |
|---|---|---|
| `2026-09-22/EvoX-21e5b7f8/Hearthward_资产构建需求清单.md` | `docs/assets/requests/2026-09-22/EvoX-21e5b7f8/Hearthward_资产构建需求清单.md` | 迁移 |
| `Resource/Tripo/主角/.gitattributes` | `art_source/TASK-004/Tripo/主角/.gitattributes` | 迁移 |
| `Resource/Tripo/主角/medieval+knight+3d+model-with-animations.zip` | `art_source/TASK-004/Tripo/主角/medieval+knight+3d+model-with-animations.zip` | 迁移 |
| `Resource/Tripo/主角/medieval+knight+3d+model.zip` | `art_source/TASK-004/Tripo/主角/medieval+knight+3d+model.zip` | 迁移 |
| `Resource/Tripo/弟弟/.gitattributes` | `art_source/TASK-004/Tripo/弟弟/.gitattributes` | 迁移 |
| `Resource/Tripo/弟弟/ChatGPT Image 2026年9月22日 13_56_24.png` | `art_source/TASK-004/Tripo/弟弟/ChatGPT Image 2026年9月22日 13_56_24.png` | 逐字节相同，归并 |
| `Resource/Tripo/弟弟/dark+fantasy+armor+3d+model.zip` | `art_source/TASK-004/Tripo/弟弟/dark+fantasy+armor+3d+model.zip` | 迁移 |
| `Resource/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/3c04be2d-8e04-4743-ae29-fabdc022b1aa_model.fbx` | `art_source/TASK-004/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/3c04be2d-8e04-4743-ae29-fabdc022b1aa_model.fbx` | 逐字节相同，归并 |
| `Resource/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/3c04be2d-8e04-4743-ae29-fabdc022b1aa_rendered.webp` | `art_source/TASK-004/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/3c04be2d-8e04-4743-ae29-fabdc022b1aa_rendered.webp` | 逐字节相同，归并 |
| `Resource/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/task.json` | `art_source/TASK-004/Tripo/弟弟/outputs/3c04be2d-8e04-4743-ae29-fabdc022b1aa/task.legacy-resource.json` | 保留不同元数据 |
| `Resource/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/a2153533-a819-4dd0-a426-231bd998fab0_pbr.fbx` | `art_source/TASK-004/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/a2153533-a819-4dd0-a426-231bd998fab0_pbr.fbx` | 逐字节相同，归并 |
| `Resource/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/a2153533-a819-4dd0-a426-231bd998fab0_rendered.webp` | `art_source/TASK-004/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/a2153533-a819-4dd0-a426-231bd998fab0_rendered.webp` | 逐字节相同，归并 |
| `Resource/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/rig_pipeline.json` | `art_source/TASK-004/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/rig_pipeline.legacy-resource.json` | 保留不同元数据 |
| `Resource/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/task.json` | `art_source/TASK-004/Tripo/弟弟/outputs/a2153533-a819-4dd0-a426-231bd998fab0/task.legacy-resource.json` | 保留不同元数据 |
| `ui pic/任务栏.png` | `art_source/ui-reference/TASK-020/任务栏.png` | 迁移 |
| `ui pic/地图.png` | `art_source/ui-reference/TASK-020/地图.png` | 迁移 |
| `ui pic/对话.png` | `art_source/ui-reference/TASK-020/对话.png` | 迁移 |
| `ui pic/技能树.png` | `art_source/ui-reference/TASK-020/技能树.png` | 迁移 |
| `ui pic/普通状态.png` | `art_source/ui-reference/TASK-020/普通状态.png` | 迁移 |
| `ui pic/暂停界面.png` | `art_source/ui-reference/TASK-020/暂停界面.png` | 迁移 |
| `ui pic/标题页.png` | `art_source/ui-reference/TASK-020/标题页.png` | 迁移 |
| `ui pic/箱子.png` | `art_source/ui-reference/TASK-020/箱子.png` | 迁移 |
| `ui pic/背包.png` | `art_source/ui-reference/TASK-020/背包.png` | 迁移 |

## 本地保护与恢复

前轮同步前的11项受跟踪改动和51个未跟踪文件仍保存在stash：`6112002feaf01446959c320ab4c620fcd6f0820e`，名称`codex-pre-main-sync-20261005 TASK-027 local files`。原角色分支和`.agent-local/task051`工作树保留。需要恢复时先完成当前补丁的保存，再回到原TASK-027分支，用`git stash apply 6112002feaf01446959c320ab4c620fcd6f0820e`恢复；不要把这份旧配置和UI删除记录直接套到新main。

5份归并的原文件另保留在`.agent-local/repository-organization/duplicate-backup/Resource/`，既有art_source原件也仍在；未进行Git历史清理或缓存删除。历史JSON中的原机器路径和历史任务允许路径保持原记录，新位置按上述表查找。
