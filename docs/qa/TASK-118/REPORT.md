# TASK-118｜门栓、短梯与障碍捷径

2026-10-14；当前本地分支 `codex/TASK-112-round2-foundation`，基线 `dbf6acd4df31ed1f7154464ac9b635e52e173806`，未提交、未合并、未发行。

内闩、短梯、横梁三类模板已接入。短梯启用双向导航连接，实际胶囊分段扫掠登降，取消恢复控制。Blender 制作九个模型，UE 导入并核对厘米尺寸。

当前集成构建：`.agent-local/qa/TASK-133/scene-build/build_result.json`。本轮原生入口 `scripts/qa/run_engine_gate.py --tests Hearthward.Round2.` 共发现并执行 15 项，全部成功；两个无上下文夹具的 DestroyActor 警告保留在 [逐项结果](../TASK-133/native-round2.json)。原生结果只覆盖其断言，不能替代 PIE、真人和性能用例。

|用例|状态|覆盖与限制|
|---|---|---|
|T118-C01 提交边界|PASS|Devices.SideCancellationCommitAndReload 验证外侧拒绝、移动/epoch 取消、合法完成与重复提交。|
|T118-C02 真实可走|PARTIAL|实际粮窖短梯下降及落地通过；完整双向正常路线待测。|
|T118-C03 伙伴净空|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T118-C04 持久捷径|PARTIAL|真实保存节点后独立进程继续，3项捷径及4项协作结果保持；奖励完整矩阵待测。|
|T118-C05 进度隔离|PASS|开门后旗帜和胜利事实保持原值，原生断言通过。|
|T118-C06 收益证明|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|

源码与参数见 [实施记录](../../planning/TASK-112/IMPLEMENTATION-20261014.md)。机器证据在隔离 `.agent-local/qa/` 中；真实玩家档未用于修改。人工签收在最终汇总集中处理，当前报告不声称验收完成。
