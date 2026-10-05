# TASK-072 本地模型延迟诊断

2026-10-04。Owner / Reviewer：XLingyyy。无 Issue。

按已批准072与ACCEPTANCE调查；原始运行由根独占 UE / 模型。此子代理只读代码、日志、C01结果与上游主源，随后按根登记窗口提供两文件诊断增量。没有启动额外模型、改变锁定参数、运行 UE / build、修改资产或 Git 写入。根已审读并合入诊断补丁，构建中；新增计时字段和timings日志尚待实际运行。

本报告为组件诊断。TASK070资产与正式联合场景尚未完成；没有正式联合性能PASS、暖p95、可见UI绘制终点或第二机器验收结论。

## C01实际结果与计时更正

| 观测 | CPU | Vulkan |
| --- | --- | --- |
| 输入 / 输出token | 3075 /83 | 3075 /85 |
| 投影 / 生成次数 | full_relevant /1 | full_relevant /1 |
| raw / e2e /执行 | PASS /PASS /PASS | PASS /PASS /PASS |
| 旧 getter latency_seconds | 116.009440s | 23.440676s |
| 提交时间UTC | 2026-10-03 20:13:35.309715 | 2026-10-03 20:18:13.039915 |
| 实际 runtime started | 20:13:35.329，pid3420，backend=cpu | 20:18:13.065，pid28764，backend=vulkan |
| 实际 runtime ready | 20:13:42.014 | 20:18:20.814 |
| 实际 context accepted | 20:13:42.114，3075 | 20:18:20.895，3075 |
| 实际 Generate发出日志 | 20:13:42.115 | 20:18:20.896 |
| started→ready 单次观测 | 6.685s | 7.749s |
| ready→Generate 单次观测 | 0.101s | 0.082s |
| Generate→成功响应解析近似段 | 约109.2s | 约15.6s |

两次请求均在提交后新建模型进程，并等待ready。因此116.009与23.4407都包含cold启动等待，不能称为暖请求或暖p95。Generate段是基于墙钟日志与旧单调计时值的近似拆分，不是已测prompt/decode时长。

原始证据：

- CPU：根 `Saved/Task068/cpu-compact-c01/cases.jsonl`；`docs/qa/TASK-068/cpu-compact-c01-results.json`。
- Vulkan：根 `Saved/Task068/vulkan-compact-c01/cases.jsonl`；其中输出token85，单次旧latency23.44067620113492。
- CPU生命周期／请求日志：根 `Saved/Logs/Hearthward-backup-2026.10.03-20.15.42.log`。
- Vulkan生命周期／请求日志：根 `Saved/Logs/Hearthward-backup-2026.10.03-20.18.47.log`。日志转存前该段位于 `Hearthward.log`；本文引用转存后的稳定路径。

这些是自然语言C01的实际业务成功证据；各运行仅一个表达，结果task_complete=false，不能替代每后端60表达质量矩阵，也没有完成联合性能场景。

## 实际启动与锁定参数

`HearthwardLocalAIRuntime.cpp:77` 生产启动参数为：context4096、并发1、生成线程4、prompt/batch线程4、CPU GPU卸载0／Vulkan默认16、reasoning off、jinja、非WebUI、禁用prompt缓存。`Config/DefaultGame.ini` 是Backend=cpu / GpuLayers=16；开发C01实际启动选择各自后端且传16层配置。

`-t4` 与 `-tb4` 分别对应生成与batch/prompt计算线程，含义按 [b10964 server参数说明](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/README.md) 核对。32是GPU层参数clamp上限；本次没有“32个生成线程”的证据。

请求保持max_tokens256、temperature0、stream=false、cache_prompt=false、enable_thinking=false、已有JSON Schema。policy输入上限3328，实际C01输入3075；没有调低目标、换模型、增加并发或改变线程／GPU层来过测。

真实后端证据来自Runtime在CreateProc成功之后打印的实际pid/backend，结合其按Backend选择 `bin/cpu` 或 `bin/vulkan` 的可执行路径与实际Args构造；没有仅靠QA目录名或报告backend字符串判断。当前未保存这两个已关闭子进程的独立CIM命令行快照、ggml设备选择／实际卸载层数／驻留显存日志。因此16层是生产实际启动参数链，未额外声称独立验证了GPU驻留层数。查询当前llama-server进程时两次C01均已结束；没有因此补启进程。

版本由 `Config/local-ai.lock.json` 固定为llama.cpp b10964与Qwen3.5-4B-Q4_K_M；本次不重复做散列校验，也没有捕获这两次运行的独立server build-info输出。

## 旧计时包含和遗漏的阶段

实际调用链：

1. SubmitPlayerTextInternal完成基本沟通检查、取得ticket与记忆记录后，在 `HearthwardLocalAISubsystem.cpp:253` 设置RequestStartedAt；之后才StartServer。
2. 未ready则Tick等待Runtime的 `/v1/models` 200健康响应；ready后SendInference。
3. SendInference读取角色知识、检索、捕获一次UE权威snapshot、构建投影及消息。
4. CountRequest顺序POST `/apply-template`，再POST `/tokenize`。输入超预算才换tier重数；实际两次C01首次full_relevant即3075，只有一次generation。
5. Generate POST `/v1/chat/completions`，非流式；业务HTTP总超时／活动超时均120秒。
6. 成功HTTP回调验证响应、choices、finish_reason和HearthwardAgent::Parse，读取usage；旧LastLatencySeconds在这里截止。
7. 后续Tick才ApplyProposal／StageCandidate，复核实际UE条件并完成候选／状态；屏幕刷新与绘制又在其后。

所以旧getter包含：需要时的模型cold、知识／投影、模板与token计数、HTTP与服务端处理、回调调度、成功解析。它遗漏：后续UE proposal/状态复核、UI刷新／绘制，以及用户确认和真实行为执行。

本机UE5.8 `HTTP/Private/GenericPlatform/HttpRequestCommon.h:142` 默认CompleteOnGameThread；项目没有覆盖该policy。`HttpManager.cpp:537–540` 在游戏线程完成请求。因此旧getter也包含HTTP已完成到主线程执行回调的等待；现有日志无法单独量化这一段。

两次C01没有任何prefill/decode原始timings留存，尚不能把约109.2秒或15.6秒归因于prompt计算、逐token生成、模型队列、HTTP或主线程拥堵。没有用猜测的缓存、网络或并发故障替代证据。

## b10964实际timings字段与最小诊断

[b10964 server-task.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-task.cpp) 的OpenAI chat final响应在stats.is_set()时写入顶层timings。字段序列化由 [server-common.cpp:84](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-common.cpp#L84) 定义：

| 字段 | 用于区分的内容 |
| --- | --- |
| cache_n | 复用的prompt token数。 |
| prompt_n / prompt_ms | 实际prompt阶段token数／毫秒。 |
| prompt_per_token_ms / prompt_per_second | prompt阶段单位token时间／速率。 |
| predicted_n / predicted_ms | 生成token数／生成阶段毫秒。 |
| predicted_per_token_ms / predicted_per_second | 生成阶段单位token时间／速率。 |

生成时间字段是predicted_ms，不能打印虚构的eval_ms字段。实际source按stats结构提供这些量，无需开另一个server、请求不同接口或改变推理参数。

诊断增量 `latency-diagnostics-production.patch` 已由根整合：

- 仅 `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp` 成功解析处，旧LastLatencySeconds赋值后、bResponseReady之前，TryGetObjectField读取Root.timings并用既有LocalAIJson记录一条UE_LOG；附generation次数与submission_to_response。不存在timings时无log；未增加fallback、整份回复落盘、stdout管道、公共接口或输出字段。
- 当前CreateProc的stdout/stderr管道参数为空，未重定向server日志；正常成功响应也没有持久化完整JSON。旧C01的timings已经无法从保留的角色JSON重建。此次只记录已有返回对象，避免另搭遥测层。
- 下一次实际运行从UE日志取Local AI timings，分别看prompt_ms与predicted_ms，再与提交／ready／generation阶段日志对齐。服务端两阶段之外的余量仍可能包含排队、HTTP、解析和game-thread调度，不能自动把全部余量称为网络耗时。

## 新QA计时与统计口径

根同时整合了 `docs/qa/TASK-068/verify_model_matrix_pie.py` 的小范围增量：

- 原latency_seconds保留为旧getter值并标明FPlatformTime endpoint；不改模型输入或已有质量／执行／零复制断言。
- 新ue_verified_latency_seconds使用Python time.monotonic，提交前起点到实际!busy、读取已完成proposal/status的终点；在用户确认／行为执行前结束。通过现有IsBusy真实状态消费，不制造完成Fact。
- 记录提交前IsModelReady。首次未ready为cold_first；已有ready为warm_followup；后续重启未ready为cold_restart。cold样本不进入warm统计。
- 暖样本不足、等待失败或没有有效角色回复，不产生p95 PASS；缺回复独立标INCOMPLETE_REPLIES，样本不足为INSUFFICIENT_SAMPLES且p95=null。
- 本次方法要求minimum_sample_count等于现有完整60表达数据集。首次冷C01加59暖不会称为暖60；没有因此新增warmup请求。足样本后使用nearest-rank p95，CPU30秒／Vulkan10秒。此60样本要求是本次统计方法，不能被解释为已经完成正式联合验收。
- warm_component_latency单独报告；原业务report.ok AST与阈值保持一致。组件超过时限仍明示FAIL，不通过修改业务阈值隐藏；组件PASS也不能当作UI／联合场景PASS。
- ui_paint_status与joint_scenario_status均NOT_RUN。若请求失败后的Busy清除也被测到，其完成耗时会保留，但缺角色回复明确不能获得成功回复或p95信用。

静态验证：Python AST通过；业务report.ok表达式与根基线一致；统计样本的cold排除、单样本、59暖不足、CPU31秒失败、Vulkan9秒足样本、缺回复六个边界通过。没有由这些纯统计检查宣称真实硬件性能已通过。两文件按当时根CRLF保存；git diff --check退出0。根随后实际运行已取得有效timings和ue_verified_latency_seconds，下面登记该次诊断；缓存候选尚未做A/B。

## 2026-10-04 Vulkan实际诊断与进程快照

根执行的真实Vulkan矩阵因业务理解门槛已不可能达到而提前停止：29条已完成，raw/e2e通过18、raw失败11；剩余31条即使全通过也只有49/60，低于54。未完成／在途请求不计成业务失败，boundary保持NOT_RUN。记录是诊断局部批次，暖样本28，不能给正式p95 PASS。后续CPU／Vulkan完整60应绑定根正在修复的System源码，旧29条不得混入新版本完整验收。

稳定业务报告为根树 `docs/qa/TASK-068/vulkan-semantics-red-29-results.json`，原始29行为 `Saved/Task068/vulkan/cases.jsonl`，同时有 `Saved/Task068/vulkan/results.json`。本次读取时timings仍位于 `Saved/Logs/Hearthward.log`，以2026.10.03-20.44.24启动pid26612至本批关闭为边界；日志随后轮换时由根登记对应backup名。

先读取的18条timings中，C01冷请求23.048秒，prompt6.070秒、decode9.242秒，余项7.737秒；runtime ready日志显示启动到ready7.625秒。其余17条暖请求prompt5.177—6.023秒、decode8.966—14.504秒，旧getter总时间14.221—20.151秒。减去两个server阶段的余项仅0.071—0.093秒，C02—C04的UE复核另增加约0.013—0.025秒。该批已表明主要时间在服务端prompt与生成阶段；尚无证据把数秒延迟归为HTTP或UE回调。每条cache_n=0，与当前两处显式关闭一致。

常规进程／设备查询在2026-10-04 04:49—04:50（Asia/Shanghai）串行／并行取得以下快照；不同工具时间不完全同步，未读取私密内存，也没有终止或改变模型进程。

| 对象 | 实际观测 |
| --- | --- |
| 子进程 | pid26612，parent32080；2026-10-04 04:44:24.4645305+08:00创建；实际Exe为 `G:/GameFactory/Hearthward/Runtime/LocalAI/bin/vulkan/llama-server.exe`。 |
| 全部公开CLI | model `G:/GameFactory/Hearthward/Runtime/LocalAI/models/Qwen3.5-4B-Q4_K_M.gguf`；host127.0.0.1、port59697、api-key已遮盖、alias hearthward-qwen-local；`-c4096 -np1 -t4 -tb4 -ngl16 --reasoning off --jinja --no-webui --no-cache-prompt`。 |
| 已加载后端 | Vulkan目录下ggml-vulkan.dll、ggml-cpu-alderlake.dll、ggml-rpc.dll、ggml.dll、ggml-base.dll和llama相关DLL；Windows vulkan-1.dll。 |
| 模型进程主存 | WorkingSet3,256,950,784 B，PrivateMemory2,824,724,480 B；累计CPU654.03125秒，OS线程37。37含HTTP／驱动等工作线程，不能把它解释为生成-t参数变成37。 |
| NVIDIA快照04:49:15.078 | RTX4060Laptop，driver577.00；显存总8188MiB、已用6010MiB、空闲1948MiB；GPU48%、memory21%、80°C、52.24W。WDDM逐进程used_gpu_memory为N/A。 |
| Windows GPUProcessMemory 04:50:02.664+08:00 | pid26612的LUID0x152DE DedicatedUsage1,806,233,600 B、SharedUsage49,958,912 B、TotalCommitted1,856,258,048 B；pid32080同LUID Dedicated3,418,550,272 B、Shared213,671,936 B。另一LUID上的server用量为0。 |
| 同次主存容量 | TotalVisibleMemory16,387,188KiB，FreePhysical1,379,504KiB；TotalVirtual32,774,376KiB，FreeVirtual6,192,156KiB。该次可用RAM约1.32GiB，高于1GiB风险阈值但余量有限。CPU为i7-13650HX，14核／20逻辑处理器。 |

CPU后端DLL与Vulkan后端同时存在符合部分offload；DLL加载本身不能证明全CPU回退。实际GPU分配证明该server使用了设备内存，当前未捕获server逐层分配日志，所以16是实际请求层数，不能把它冒充独立测得的驻留层数。全卡显存包含UE及其他桌面进程；温度／利用率／RAM单次样本不足以证明降频、换页、OOM或某一进程抢占。

只读GGUF metadata（未读tensor权重、未计算hash）确认实际文件2,740,937,888 B、GGUF v3、426 tensors、46 metadata；architecture=qwen35，block_count32、embedding2560、metadata原生context262144。生产-c仍为4096，未改上下文。按 [b10964 llama-model.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/src/llama-model.cpp#L1496) 默认层分配公式，32 blocks与-ngl16的配置对应15个repeating blocks及output层计划卸载，输入和其他前置层保留CPU；这是源码推导，实际分配日志仍未采到。

## Prompt复用来源、语义和两行候选

Git只读历史定位：关闭server缓存及请求cache_prompt=false最早一并来自4a75c75（2026-09-18，TASK-013初接本地Qwen）。04239f5把启动移动到Runtime；6d1ca5e重写上下文Body时沿用false。未找到当时解释为何关闭的ADR。当前local-ai.lock.json deployment、npc-agent.policy，以及已批准053—074共同文件／072、025／040 ADR均没有查到明确禁止prompt reuse的条款。锁定模型／版本、3328输入／256输出、16 GPU层、单并发和一次生成仍保持；040要求一次可信快照、同一完整Body计数后生成，072要求实测瓶颈修复保持能力。因而它是可供根审阅的局部性能候选，不能仅因缺少禁令就宣称性能／安全已经验证。

[b10964 common/arg.cpp:3561](https://github.com/ggml-org/llama.cpp/blob/b10964/common/arg.cpp#L3561) 明确支持正flag --cache-prompt。[server-schema.cpp:532](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-schema.cpp#L532) 将全局cache_prompt作为请求默认，schema提供同名bool字段。两个开关保持一致可使配置意图清楚。

[server-context.cpp:3217](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-context.cpp#L3217) 在开启时比较当前完整input tokens与旧prompt的相同前缀；关闭时n_past=0。后续3409记录cache_n、3414保留前缀，旧suffix不继续作为新请求内容。[server README](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/README.md) 提醒不同batch组织可使logits有数值差异，temperature0也不能替代业务回归。hybrid／recurrent模型在checkpoint不足时可按3349—3383重新处理全部prompt，故开关true不保证cache_n非0。

复用的是由当前完全相同token前缀推得的模型计算状态。NPC长期记忆仍由UE的显式Memory／Belief／Episode及保存管理，仍须构造当前完整message、过滤知识、硬约束与schema；不能靠旧server状态补消息或省略当前世界投影。CancelPending推进Serial，HTTP检查ExpectedSerial／StillCurrent，ApplyProposal再检查；Ticket／TimelineEpoch、记忆revision、候选ID、确认及真实执行权限保持原链。相同静态system前缀跨epoch复用不会赋予旧回复执行权限；旧suffix与旧回复仍须按生产链拒绝。

根已批准候选 `prompt-kv-prefix-candidate.patch`，仅Runtime.cpp:78由--no-cache-prompt换正flag、Subsystem.cpp:451由false换true；Runtime当时LF、Subsystem当时CRLF按实际保留。root基线git apply --check --whitespace=error通过，numstat各+1/-1；未改System文本、messages、Schema、projection、CountRequest、参数、统计或任何公共接口。尚未由本agent写生产／构建／运行，最终是否保留由根A/B决定。

最小A/B保留同样原话及上下文，只检查实际cache_n、prompt_ms、predicted_ms、ue_verified_latency与原理解／拒绝／执行／零复制、旧epoch边界；模型参数不变，也不增加新缓存目录／手工KV／context截断。真实收益依赖共有前缀与hybrid checkpoint，不能用满额5—6秒prefill作为保证。已有C18 decode14.504秒，即便prompt计算降到0仍超过10秒；所以缓存本身无法证明所有暖样本达标。后续若仍失败，应依据decode和联合资源证据定位，16层／4线程的锁参数保持，避免以减输出能力或调宽门槛掩盖失败。

## 联合性能仍需取得的证据

[Epic Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine) 提供Timing的CPU/GPU/帧轨道、Memory和Slate分析入口。下一轮由根在批准的五场景、1080p原生极高及锁定供电／驱动条件下串行采集；不改画质／模型参数，并将取trace的观测条件记入报告。

目前没有可归属于这两个C01的Timing／Memory／Slate原始trace，因此不能量化帧回调等待、UI绘制、进程内存／显存竞争或把单一组件请求当作联合场景。后续最小顺序是先用已返回timings定位prompt与decode占比，再只追踪该瓶颈及UE callback附近的帧；需要内存归因时再取对应Memory证据。

冷ready与暖完整回复分开统计；业务120秒超时不等于性能目标。非流式TTFT仍NOT_RUN。等待反馈0.2秒、各场p99／1%Low／>50ms比例、RAM／VRAM／页文件／OOM和独立第二机器仍需真实证据。070未完成及065真人证据暂缺不能被本报告当作完成条件。
