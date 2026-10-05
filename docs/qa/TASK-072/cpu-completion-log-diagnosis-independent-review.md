# TASK-072 CPU completion日志诊断独立审阅

2026-10-05。Root自有运行结束后，只读其local-ai/engine/results/真实CSV；没有启动UE/模型/HTTP或改参数/Source/Git。分析只写自身QA072。实际pool1356d3d4-f148-41f6-b343-150ee67de779，UE PID30100，CPU server PID46992，原话仍为“采集1份木材送入营地仓库”，正常new/自然图，source16未确认保持，generation1。

**已确认主要等待阶段包含至少102.85秒prompt处理；最后可见记录仍在处理2657/3173（约84%）输入。** 第一次CPU运行没有server日志，本次日志接入成功，增加了阶段证据。没有取得CPU完整prompt结束、decode開始/结束或最终timings；最后约17秒的实际进展仍未知，不能断言decode始终未开始，也不能据日志空白判定卡死。

## 实际server阶段

[local-ai.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-1356d3d4-f148-41f6-b343-150ee67de779/local-ai.log)共13行。该server真实打印verbosity3、n_threads4、slots1、ctx4096，并于uptime8.418109秒loaded。锁定模型路径是原bundle Qwen3.5-4B-Q4_K_M.gguf；本次文件没有实际build版本行，不用锁文件代替运行打印。

| 日志行 | Server运行秒 | 实际记录 | 累计prompt秒 | 累计prompt速率 |
|---|---:|---|---:|---:|
| 10 | 8.979485 | slot0/task0 processing | — | — |
| 11 | 85.854898 | n_tokens2048，progress0.65 | 76.88 | 26.64t/s |
| 12 | 109.826777 | n_tokens2617，progress0.82 | 100.85 | 25.95t/s |
| 13 | 111.832916 | n_tokens2657，progress0.84 | 102.85 | 25.83t/s |

`processing task`位于launch尾部，此前这次common_sampler_init已返回；不能把这段已确认路径归因为初始sampler未完成。随后仍有独立slot.init_sampler及prompt批次工作。INFO prompt记录的计时在批次decode完成后更新，在途batch无连续心跳；默认decode进度又要求≥100输出token。具体路径见锁定[b10964 server-context.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-context.cpp#L1635)。

[engine.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-1356d3d4-f148-41f6-b343-150ee67de779/engine.log:1836)：22:04:09.992启动→22:04:18.995 ready（原生9.003秒）→22:04:19.165 accepted3173 full_relevant/dropped空→19.166 generation1→22:06:19.167 HTTP120.00秒超时。generation到首个请求错误120.001秒，随后22:06:19.174 StopServer、19.202 MODEL_UNAVAILABLE。主机Ready9.985160秒、terminal130.081577秒，差异由提交起点及1秒轮询定义造成，分别保留。

HTTP业务超时仍是首个请求错误；模型成功ready且prompt有实质推进，日志无server error/OOM。终态raw空/output0/PID0来自没有接受成功完整回复及主动清理路径，不表示无计算。正常quit true/stop请求true，engine22:06:22.067 shutdown；没有给模型实际退出时刻新增独立进程采样信用。

## 与既有Vulkan对照

同自然normal_new输入3173，Vulkan原[成功timings](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-29dd5b02-c8e5-44d6-a84d-385afca4ee94/engine.log:1838)为prompt3173/cache0/14.786473秒（214.588t/s），decode69/12.575846秒，submission→response40.034秒；主机Ready13.685063/terminal40.905367秒。CPU末条累计prompt25.83t/s约为这次Vulkan累计速率的1/8.31，但CPU未取得全prompt完成值，不能据此计算真实完整CPU总耗时或暖P95。不能把不同窗口的冷加载、缓存或热状态当作已控制因果变量。

## 同次CSV资源

| 字段 | 实际值 |
|---|---:|
| 帧数/累计帧时长 | 11697 /130.124902秒 |
| p99 | 15.8573ms |
| 1%Low | 55.829269FPS，低于60 |
| >50ms | 1 /0.0085492% |
| 最低可用物理内存 | 6.9727MiB |
| 可用<1024MiB覆盖的累计CSV帧时长 | 85.557679秒，5区间 |
| UE working set范围 | 4419.7227–4859.1758MiB |
| UE commit范围 | 6878.6641–7139.4258MiB |
| RHI GPU有效非0范围 | 2345.3906–2409.3906MiB |
| GPUTime范围 | 6.3633–8.9317ms |

最低内存位于冷加载3.418090–7.400988秒区间。最长低于1GiB区间56.970068–129.336799秒，共72.366731秒，包含三条已出现的prefill进度时段。其余完整区间及数值见[JSON](./cpu-completion-log-diagnosis-independent-review.json)。这些值按真实CSV字段及累计帧时长统计，未新增高频OS采样或补造峰值；UE工作集不代替模型RSS。

接近耗尽的可用RAM与慢prefill同时出现，证明余量风险和prompt慢阶段；未测硬缺页、模型RSS/commit、CPU温度/频率，不能据此宣称换页或节流已经成为唯一原因。p99和>50ms达单项门槛，1%Low未达。仍为单场景cold组件诊断，无五场景/暖P95/Shipping/双机验收信用。

## 后续最小范围

同Editor -game再重复无日志请求不会补齐首要证据。可优先查现有Development Game实际build/cook/stage/launch路径，降低Editor组件开销是否改善资源余量需要真实独立Game对照；该调查及后续诊断不要求先宣称074正式RC冻结。所有模型/画质/120秒参数保持。当前没有已批准或已执行的Game产物，完整诊断结果保留FAIL。
