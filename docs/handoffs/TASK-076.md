# TASK-076 新版UI与玩法整合交接

用户2026-10-05要求采用新版UI、保留玩法、解决冲突，并追加授权提交、推送及整理更新main。来源main `ac6a302ba24ea6531ccdaf880d83e9a03779c51e`，UI `fa1828ed79d26518a5163f522ee0dbf124097209`，整合分支 `codex/TASK-076-ui-gameplay-integration`。

## 交付

- 14个文件冲突解决，双方31个共同改动文件按玩法/表现衔接；新UI与main玩法同一工程编译运行。
- 保留新版局部地图，增加世界地图入口接回探索/发现/任务定位/路标/传送；修复加载输入、放弃救援确认及150%暂停菜单命中。
- 保留统一日夜、伙伴攀越、3秒动作与道具互斥、全局失败和回档保护。现有Content、Config和Runtime没有本轮改动。
- TASK-075素材与资料整理一并交付；新旧两张地图预览移入`art_source/ui-reference/TASK-076/`，根目录保留正常启动和专项地图入口。
- main的TASK-053继续是通行任务。历史UI TASK-053资料按[分支整合清单](../planning/BRANCH_INTEGRATION.md)追溯；当前结果使用TASK-076，不引入四千多份历史QA试跑文件。
- 本机启动路径读取既有environment.json；新运行输出留在.agent-local，用户入口见[TestClient说明](../../TestClient/README.md)。README、状态和索引均同步。

## 验证

Development Editor构建通过；53项原生、2项地图渲染、144项独立游戏输入、307项背包、65项四栏道具、17项提示及33项工具测试通过。仓库自检77任务快照、0错误。详见[报告](../qa/TASK-076/REPORT.md)，其中保留复现入口、实际失败原因、修复及验证边界。

本轮完成代码整合和定向验证，原各任务的Owner视觉/完整体验验收不代填，任务元数据保持Active以保留该边界。没有发布安装包。

## 恢复点

同步前原用户改动保存在stash `6112002feaf01446959c320ab4c620fcd6f0820e`。TASK-075整理前置补丁另存stash `9575fa006d3883bb830a1e45150ec68200b476a2`。原TASK-027分支与`.agent-local/task051`工作树保留；不要把旧配置和UI删除记录直接apply到新main。素材迁移及去重逐项见[TASK-075交接](TASK-075.md)。
