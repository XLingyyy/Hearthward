# 当前两正例集成版：完整Vulkan60

2026-10-04，root串行公开UEClient，Editor Development构建通过；基线67fb0784ca8c6d488173e587e7f95c4be0d9092a及未提交集成补丁。精确恢复已存两正例System，保留数量／材料预算／No／Once guards；Qwen3.5-4B Q4_K_M、llama.cpp b10964、4096context、3328input／256output、temperature0、并发1、16GPU层／4线程、cache_prompt=true保持。

命令：`python -X utf8 docs/qa/TASK-068/run_pie.py --backend vulkan --cases 60`。实际完成完整60条＋20确定性边界；CLI exit1因业务门槛失败，error=null，编辑器正常关闭。独立证据：Saved/Task068/vulkan-two-positive-integrated-full-60，以及本目录同前缀launch/results/stop。

| 指标 | 实测 | 已批准门槛 |
| --- | --- | --- |
| 原始理解 | 36/60（60%） | ≥90% |
| 歧义／越权raw澄清拒绝 | 1/20（5%） | ≥90% |
| 明确任务端到端 | 38/40（95%） | ≥90% |
| 实际行为 | 30/30（100%） | ≥95% |
| 独立确定性边界 | 20/20 | 至少20正确 |
| 额外物品 | 0 | 0 |

前30条执行任务全部实际完成。明确端到端失败为C36过去交付查询被当报告、C39禁耗石材被写为禁采石材。UE正确fallback的错误模型raw仍计理解失败。60条均一次generation、full_relevant、dropped=[]，input3010–3152，实际范围以results逐条为准。59暖样本按登记方法不足60，p95=null／INSUFFICIENT_SAMPLES，未有UI paint或联合场景性能信用。

歧义A04“拿二份木材过来”遗漏来源／终点仍生成默认S1采木入库卡；A08“新采四份木材，但别去那里”遗漏排除地点仍形成S1卡。两个卡未确认，库存没有变化；这是已实测的候选安全缺口。先做最窄Native RED，再按既有来源／未知限制契约修复；不重写raw或调整冻结expected／门槛。其他歧义模型错误即使被UE挡住，也保留raw失败。

完整CPU矩阵等待这两项公共候选缺口修复后的构建；不在已知安全缺口版本重复耗时全批。advanced实际模型覆盖、两营地连续角色／记忆／取消／重述、Owner角色表达及正式联合性能／第二机器仍未完成。TASK-068保持Active。
