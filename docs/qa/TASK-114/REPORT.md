# TASK-114｜三类敌人职责与一名行为精英

2026-10-14；当前本地分支 `codex/TASK-112-round2-foundation`，基线 `dbf6acd4df31ed1f7154464ac9b635e52e173806`，未提交、未合并、未发行。

复用 home_workshops_18 的原生命、奖励和阵亡身份；下劈为 1.1/0.2/1.3 秒。守卫、射手、重兵分别使用接近、射线调整和预兆反制；局部最多两个近战攻击槽。

当前集成构建：`.agent-local/qa/TASK-133/scene-build/build_result.json`。本轮原生入口 `scripts/qa/run_engine_gate.py --tests Hearthward.Round2.` 共发现并执行 15 项，全部成功；两个无上下文夹具的 DestroyActor 警告保留在 [逐项结果](../TASK-133/native-round2.json)。原生结果只覆盖其断言，不能替代 PIE、真人和性能用例。

|用例|状态|覆盖与限制|
|---|---|---|
|T114-C01 身份守恒|PASS|原生 EliteTimingIdentityAndCancellation 验证原 80 人、精英 ID/生命/奖励及阵亡快照；独立进程永久清除另属 C05。|
|T114-C02 职责可读|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T114-C03 协同边界|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T114-C04 精英反制|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T114-C05 永久清除|PARTIAL|阵亡快照解析通过；真实击杀后独立进程及离区返场未完成。|
|T114-C06 超界与复位|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|

源码与参数见 [实施记录](../../planning/TASK-112/IMPLEMENTATION-20261014.md)。机器证据在隔离 `.agent-local/qa/` 中；真实玩家档未用于修改。人工签收在最终汇总集中处理，当前报告不声称验收完成。
