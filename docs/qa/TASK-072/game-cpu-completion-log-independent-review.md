# Game Development CPU 单请求独立审阅

本次 pool `99246739-0d4c-46d7-b4aa-8004fa5253bc` 实际失败于120秒HTTP业务超时。一次公开提交“采集1份木材送入营地仓库”，generation=1，input=3177，output=0，raw为空，终态为`MODEL_UNAVAILABLE`，无候选，木材来源库存16→16。只读取已完成日志、results及CSV，未启动UE、模型、HTTP或构建。

[results.json](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-99246739-0d4c-46d7-b4aa-8004fa5253bc/results.json)与[engine.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-99246739-0d4c-46d7-b4aa-8004fa5253bc/engine.log:1105)确认UE PID32572、模型PID20820；模型8.604秒ready后成功接受完整上下文，随后发出第1次请求。请求相关首错为engine第1111行HTTP120秒超时，发送→超时120.001秒；第1112–1113行记录HTTP失败和`MODEL_UNAVAILABLE`。正常Quit返回true，stop请求成功。Root另行确认两个PID已不存在；本审阅没有查询进程。

[local-ai.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-99246739-0d4c-46d7-b4aa-8004fa5253bc/local-ai.log:11)确认同一slot/task依次完成2048、2620、2661个prompt tokens，累计prefill79.91、104.57、106.70秒，最后24.94 tokens/s、progress=0.84。至少106.70秒花在prefill阶段，完整prefill与decode timing没有记录，末尾阶段仍无法确定。最终`output_tokens=0`属于失败回调结果，不能单独证明模型从未开始decode。

| 实测字段 | Editor CPU日志诊断 | Game Development CPU |
|---|---:|---:|
| input tokens | 3173 | 3177 |
| 最后可见prefill累计 | 2657 tokens / 102.85 s | 2661 tokens / 106.70 s |
| 最后可见prefill速率 | 25.83 tokens/s | 24.94 tokens/s |
| UE CSV PhysicalUsedMB范围 | 4419.72–4859.18 MiB | 2482.33–2748.29 MiB |
| 系统CSV最低可用RAM | 6.97 MiB | 612.95 MiB |
| 可用RAM低于1024 MiB累计 | 85.56 s | 121.01 s |
| 请求终态 | HTTP120秒超时 | HTTP120秒超时 |

原话相同，但prompt不完全相同：Game输入多4 tokens，实际`own_bag`中`Arrow/Armor/Medicine`的FName显示大小写有差异。Game UE内存占用明显降低，最低系统可用RAM增加；本次CPU最后可见prefill速率仍未改善。两次冷启动单样本不足以判定性能差异，现有证据不能把慢prefill归因于内存、缓存、CPU频率或硬缺页。

本次[frame-report.json](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-99246739-0d4c-46d7-b4aa-8004fa5253bc/frame-report.json)覆盖全部13553个数值帧、129.6783524秒：p99=14.3746 ms，1%Low=58.16995 FPS，超过50 ms为1帧（0.00737844%），最大66.7757 ms。1%Low仍低于60 FPS门槛；p99与长帧比例分别符合当前阈值，整体帧门槛未通过。最大帧发生于CSV起点0.111秒，GameThread=68.2386 ms、GPU=7.1611 ms，早于实际generation请求，不能把该帧算成模型输出造成的停顿。

CSV资源读取结果：系统可用RAM612.9492–5137.6797 MiB，低于1024 MiB为连续7.9177113–128.9289735秒、12808帧，共121.0112622秒。UE PhysicalUsedMB=2482.3281–2748.2930 MiB，VirtualUsedMB=4236.9609–4475.1562 MiB。RHI LocalUsed有效值2343.2031–2343.4531 MiB，预算7189 MiB；开头2个0为尚未初始化的RHI统计，不参与有效最小值。GPUTime=6.3079–9.5627 ms。RHI统计只描述该引擎的GPU统计，不能充当模型或系统全局显存峰值。

Root提供的一次运行中独立WorkingSet记录为Game2612088832 bytes、模型4474679296 bytes；该单点没有峰值信用。本pool的系统`runtime-resource-snapshot.json`采于终态清理后，已从本运行资源分析排除。

此材料只覆盖Game Development正常new单场景、冷请求一次；五场景双后端、暖p95、首帧UI反馈、TTFT、Shipping、第二机器和正式RC均未验收。模型回复未完成，无语义或执行成功信用。结构化数据及逐段时间位于[独立JSON](G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-072/game-cpu-completion-log-independent-review.json)。
