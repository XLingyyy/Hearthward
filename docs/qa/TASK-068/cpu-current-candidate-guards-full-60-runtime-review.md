# 当前候选守卫版：完整 CPU60

2026-10-05接续，root串行UEClient，未提交集成补丁。当前两正例System与原intent-first Schema，保留数量、预算、No/Once、采集来源/地点及新增口语兼容守卫。Qwen3.5-4B Q4_K_M、llama.cpp b10964、4096context、3328input/256output、temperature0、并发1、4线程、CPU0GPU层保持；没有应用后续字段顺序候选。

实际运行 `run_pie.py --backend cpu --cases 60 --timeout 5400`，完整60条及20确定性边界完成，error=null，CLI exit1因正式门槛失败，stop_editor ok=true。完整原始报告、launch/stop及Saved独立目录使用 cpu-current-candidate-guards-full-60 前缀。

| 指标 | 实测 | 正式门槛 |
| --- | --- | --- |
| 原始理解 | 34/60（56.67%） | ≥90% |
| 歧义/越权raw | 1/20（5%） | ≥90% |
| 明确端到端 | 36/40（90%） | ≥90% |
| 实际执行 | 28/30（93.33%） | ≥95% |
| 独立确定性边界 | 20/20 | 至少20正确 |
| 额外物品 | 0 | 0 |

明确端到端失败为C14、C21、C36、C39：协助战斗被生成hold卡；货物应到弟弟背包却生成camp_to_player并被UE拒绝；过去交付查询误分类；禁耗石材被写为ban:stone并拒绝。C14/C21原始卡不符合预期，未确认，不代表正确模型卡的运行中结算失败。30项执行预期中其余28项实际完成。

A04/A08新采来源/地点守卫已正确澄清并candidate=false；C39错规则候选正确拒绝。保持raw失败，守卫不抵扣理解分数。没有改冻数据、阈值或原始模型JSON。

59暖样本低于登记60，p95=null/INSUFFICIENT_SAMPLES；所记端点是Submit→UE proposal/status读取完成，排除Slate paint。未计作联合场景、Shipping或第二机器性能通过。完整CPU与此前Vulkan受测源码不同（Vulkan早于新守卫），不能假称同一快照正式双后端已通过。

后续Schema字段顺序受控诊断已使用已有固定10实际运行，System/值域/分支/8字段/Parser/参数完全不动。raw5/10、明确端到端4/7、执行3/6，三个明确任务退化；已精确撤回，详见schema-npc-line-first-runtime-review.md。本报告保留其前驱基线。TASK-068保持Active。
