# TASK-123｜现有族人短线一：把工具放回哪里

2026-10-14；当前本地分支 `codex/TASK-112-round2-foundation`，基线 `dbf6acd4df31ed1f7154464ac9b635e52e173806`，未提交、未合并、未发行。

civilian_initial_02 三段人物线使用 met/visited/remembered 状态；原工具已交付事实承接现场步骤。选择营地/故乡决定陈列归属，原岗位优先。

当前集成构建：`.agent-local/qa/TASK-133/scene-build/build_result.json`。本轮原生入口 `scripts/qa/run_engine_gate.py --tests Hearthward.Round2.` 共发现并执行 15 项，全部成功；两个无上下文夹具的 DestroyActor 警告保留在 [逐项结果](../TASK-133/native-round2.json)。原生结果只覆盖其断言，不能替代 PIE、真人和性能用例。

|用例|状态|覆盖与限制|
|---|---|---|
|T123-C01 人物绑定|PARTIAL|三名正式ID的隐藏人物拒绝推进、显现后开始通过；人口快照未增加。实际营救/场景生成唯一性仍待验证。|
|T123-C02 三段体验|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T123-C03 物件唯一|PASS|Personal.ChoicesVictoryUniqueDisplayAndStaleUI 验证选择只提交一次、同事实移动后只有一个展示、没有库存副本。|
|T123-C04 日常后果|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T123-C05 选择恢复|PARTIAL|人物分支节点跨进程恢复已测；实际路线与后果表现仍缺验收。|
|T123-C06 人物辨识|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|

源码与参数见 [实施记录](../../planning/TASK-112/IMPLEMENTATION-20261014.md)。机器证据在隔离 `.agent-local/qa/` 中；真实玩家档未用于修改。人工签收在最终汇总集中处理，当前报告不声称验收完成。

2026-10-15：三人物结尾守恒回归通过、零警告；现场观察及Victory由隔离夹具提供，未代替正式路线或多日推进。见 [原生证据](../TASK-133/story-conservation-20261015.json)。

2026-10-15 三人两分支实际SavePoint/LoadPoint通过；第二UE进程从原始存档副本恢复18个认识/现场/结尾节点，逐项核对选择、对白记忆及库存。现场与胜利使用明确夹具前置，不计真实路线、分支外观或其他任务继续游玩。C05保留PARTIAL，证据：../TASK-133/story-load-branches-20261015.json。

2026-10-15 补陈列恢复：同一18节点在新进程中核对六个正式槽位ID的实际StaticMesh组件，认识/现场节点隐藏、结尾节点恢复对应命名资产；写/读两进程各1项测试通过且0警告。这些组件为原生夹具，不计原场景槽位通行或正常路线。证据：../TASK-133/story-load-branches-20261015.json。
