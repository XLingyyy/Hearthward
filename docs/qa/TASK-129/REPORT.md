# TASK-129｜真实动物踪迹与可验证的观察狩猎

2026-10-14；当前本地分支 `codex/TASK-112-round2-foundation`，基线 `dbf6acd4df31ed1f7154464ac9b635e52e173806`，未提交、未合并、未发行。

鹿、野猪四个现有槽位在营地周围 500 米观察区采样真实位置，每个体最多 24 点、90 日历分钟有效；死亡停止采样，生命代次隔离，保留原狩猎掉落与钓鱼。

当前集成构建：`.agent-local/qa/TASK-133/scene-build/build_result.json`。本轮原生入口 `scripts/qa/run_engine_gate.py --tests Hearthward.Round2.` 共发现并执行 15 项，全部成功；两个无上下文夹具的 DestroyActor 警告保留在 [逐项结果](../TASK-133/native-round2.json)。原生结果只覆盖其断言，不能替代 PIE、真人和性能用例。

|用例|状态|覆盖与限制|
|---|---|---|
|T129-C01 真实来源|PASS|RealIdentityDeathRefreshExpiryAndBudget 读取真实 Nature 状态与位置生成有界样本。|
|T129-C02 可观察寻找|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|
|T129-C03 代次隔离|PASS|原生死亡、刷新代次、过期与旧个体隔离通过。|
|T129-C04 回档再现|PASS|实际死亡/五秒拾取/重复存读档及独立进程读回数量守恒通过。|
|T129-C05 数量上限|PARTIAL|原生验证样本上限与过期；长期场景资源预算尚未实测。|
|T129-C06 生态完整性|NOT_RUN|尚未完成本用例规定的实际操作与证据；不以已编译或其他层测试代替。|

源码与参数见 [实施记录](../../planning/TASK-112/IMPLEMENTATION-20261014.md)。机器证据在隔离 `.agent-local/qa/` 中；真实玩家档未用于修改。人工签收在最终汇总集中处理，当前报告不声称验收完成。

2026-10-15 真实保存中的4组踪迹，经独立Continue和再次LoadPoint逐字段恢复，库存一致；整轮34项实景检查通过。早一轮夹具立即移动导致灯具尚未生成的失败保留。自然跟踪/死亡掉落组合未覆盖，C04保留PARTIAL。证据：../TASK-133/trail-restart-20261015.json。

2026-10-15：动物死亡/真实五秒拾取/重复存读档数量守恒1项通过，独立UE进程读取原始两节点再验证1项通过，均0警告。前两轮缺少生态地面标记的夹具失败保留；仅修正夹具，未改生产掉落。结合真实地图踪迹重启证据，T129-C04=PASS。自然跟踪、狩猎输入与实景观察另列。见 docs/qa/TASK-133/animal-death-loot-load-20261015.json。矩阵30 PASS、43 PARTIAL、1 FAIL、58 NOT_RUN。
