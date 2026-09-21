# TASK-026｜任务单交接

更新时间：2026-09-21（Asia/Shanghai）。记录人：Codex。

## 定位与授权

- 用户指定下一顺序任务为大地图自然场景构建，本轮只编写任务单。
- 工作目录：`G:/GameFactory/Hearthward`；文档分支：`codex/TASK-026-natural-world-plan`。
- 读取基线：`39e8610a490ea93467219656d0690ec50735e658`；开始时main工作树干净。未获取远端最新引用，未核验实时Issue、评审人和资产锁，未编辑二进制资产。
- 实施分支拟为 `codex/TASK-026-natural-world`；真实Issue及独立评审人未分配，任务保持Backlog。
- 用户随后明确授权将026任务单推送到main。本次发布使用独立工作目录与 codex/TASK-026-publish 分支，仅提交7份任务文档；未纳入原工作区新增的技术调查文件。
- 发布前已fetch确认origin/main仍为上述基线；最终提交与推送结果以Git记录为准。

## 已完成与边界

已阅读项目工作流、设计相关条款、004任务和资源汇总，核对源资产及Content目录。004已交付77个源文件与两份SOURCE记录；自然素材尚未导入UE，人物与房屋缺项不阻塞026。

新增 [任务说明](../tasks/TASK-026.md)和[机器快照](../tasks/TASK-026.json)，并同步README、START_HERE、PROJECT_STATE和任务索引中相关状态。新任务约4×4km、6km²连通面积、主环线及性能数值为规划提案，未来实施启动时确认；未改写GDD或宣称Owner已批准具体参数。

未制作地形、水系、植被、材质、场景或资产；未启动UE、下载素材、安装依赖、执行026玩法或改动004源文件。

## 验证

本轮使用现有 `scripts/validate_repo.py` 的 `validate_task`、`check_markdown_links` 和 `collect_scope_changes` 做定向检查：TASK-026 JSON结构、6份Markdown的本地链接、恰好7个文档变更均PASS；`git diff --check`通过。`agent_context.py --task TASK-026`正常输出只读回执。检查对象为上述HEAD上的未提交文档工作树，未运行全仓测试或归档哈希检查。

全部场景验收A1—A8、UE构建、PIE、Standalone、视觉和性能为NOT_RUN，无游戏运行证据。

本单在基线尚无任务快照，不能用新写的允许路径宣称“基线已批准范围检查通过”。未修改仓库校验规则或伪造Issue/审批记录。

## 后续接手

1. Owner启动实施时确认总体尺度/可探索面积/性能门槛及World Partition/OFPA工程决定，补齐Issue、评审、写者和资产锁安排。
2. 以已批准任务快照建立实施分支，按任务单盘点选中自然素材、完成总体图和小面积样段，再展开地图。
3. 完成真实通行、视觉、流送与性能验证并交接；人文场景和玩法接入另开任务。

本次只有文本文件变更，不持有新增资产锁。撤回任务单时仅处理本轮文档差异，保留004资源和所有既有游戏成果。
