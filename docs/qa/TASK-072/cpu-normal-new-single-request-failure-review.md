# TASK-072 单请求CPU超时与既有CPU/Vulkan资源事实

2026-10-05，只读实际session、引擎源码与锁定llama.cpp主源。未启动UE、模型或HTTP，未修改Root/Source/Content/参数/冻结数据，未执行Git/hash。两次运行均由Root串行执行，本稿不增加运行信用。

CPU唯一可证首个推理错误是UE `/v1/chat/completions` 的120.00秒HTTP业务超时；ready和输入计数此前已成功。缺少llama server输出，当前无法分解CPU prefill/decode，也无法断言CPU性能、缓存、网络或OOM造成超时。Vulkan同自然场景单请求正常取得候选，为后端对照结果；单次对照仍不能替代CPU阶段证据。

## 实际顺序

以下时间采用engine.log原值；`2026.10.04-21`对应Asia/Shanghai `2026-10-05 05`。主机秒数来自正常公共RC轮询，包含轮询间隔，与原生日志时间分别保留。

| CPU原生日志时间 | 行 | 事实 |
|---|---:|---|
| 21:45:19.162 | 1819 | CSV开始 |
| 21:45:19.217 | 1834 | 实际server PID16864，backend=cpu |
| 21:45:29.175 | 1835 | Runtime ready；原生start→ready9.958秒 |
| 21:45:29.485 | 1836 | full_relevant计数3173，dropped为空 |
| 21:45:29.486 | 1837 | generation request #1 |
| 21:47:29.488 | 1840 | localhost:53222 completion HTTP超时120.00秒 |
| 21:47:29.529 | 1841 | success0/code0；submission起总耗时130.35秒 |
| 21:47:29.554 | 1842 | MODEL_UNAVAILABLE，未执行动作 |
| 21:47:30.460 | 1850 | 正常CSV结束并写出11727帧 |
| 21:47:31.624 | 1855 | 正常quit RequestExit0 |
| 21:47:32.494 | 1892 | Game engine shutdown |

原生generation→HTTP timeout为120.002秒。主机Ready11.0133秒、terminal131.1972秒；实际一次Submit和一次generation，raw为空、output_tokens0、无candidate，source wood16保持。源码[HTTP失败分支](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:485)先`StopServer()`再`Fail()`，因此终态PID0不能证明server先崩。`LastLatency=0`和output0表示未接收到并接受成功完整响应，无法证明未开始decode。[原始CPU engine.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-7ae656be-42d4-4667-9573-33c5799cdbcb/engine.log:1834)、[results](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-7ae656be-42d4-4667-9573-33c5799cdbcb/results.json)。

Runtime原实现CreateProc的stdout/stderr pipe为空，未指定log-file。只读检查该session全部文件与实际bundle/bin/cpu目录，未找到llama/stdout/stderr日志。该次实际版本/ctx/线程运行时打印与服务端断开时刻未归档；锁定配置和源码参数仍为b10964、4096ctx、1并发、4+4线程、CPU0GPU层。不能用配置值冒称本次server日志复核。

Vulkan actualserver PID36688，21:50:44.311启动、21:50:56.798 ready、21:50:56.896 generation1；21:51:24.318成功timings：prompt_n3173/cache_n0/prompt14.786473秒，predicted69/decode12.575846秒，submission→response40.034秒。主机Ready13.6851/terminal40.9054秒。它证明本场景可成功返回，未建立暖请求P95。[实际Vulkan engine.log](G:/GameFactory/Hearthward/.agent-local/task051/Saved/Task072/standalone-rc-smoke-29dd5b02-c8e5-44d6-a84d-385afca4ee94/engine.log:1834)。

## CSV和资源边界

| 实际CSV字段 | CPU | Vulkan |
|---|---:|---:|
| 帧数/累计帧时长 | 11727 /131.246724秒 | 5890 /61.601223秒 |
| p99 FrameTime | 15.9546ms | 16.5024ms |
| 1% Low | 53.2438FPS | 52.3017FPS |
| >50ms | 4 /0.034109% | 0 |
| MemoryFreeMB最低 | 149.3516MiB | 189.7695MiB |
| 可用<1024MiB累计CSV帧时长 | 68.944063秒 /6056帧 | 7.898980秒 /689帧 |
| PhysicalUsedMB范围 | 4061.2539–4860.6797MiB | 4567.6133–4836.7031MiB |
| VirtualUsedMB范围 | 6811.3477–7136.5195MiB | 6910.6758–7129.5000MiB |
| GPUMem/LocalUsedMB有效非0范围 | 2313.1406–2409.1406MiB | 2377.3906–2409.3906MiB |
| GPUMem/LocalBudgetMB有效值 | 7189MiB | 7189MiB |
| GPUTime范围 | 6.4046–9.7720ms | 6.3457–14.1894ms |

内存/GPU列的MB是bytes/1024²，报告明确为MiB。`MemoryFreeMB`来自AvailablePhysical，Windows GetExtraDevelopmentMemorySize继承通用值0；UE `PhysicalUsedMB`来自当前UE进程WorkingSetSize，`VirtualUsedMB`来自该进程commit charge。它们没有单独提供llama进程RSS/commit。RHI GPU列来自UE运行的RHI内存统计，不能作为全系统显存或独立模型显存。CPU头3帧和Vulkan头1帧GPU列为0；此值保留在JSON，按尚未初始化统计处理，不计真实最低显存0。

CPU最低可用内存位于CSV起2.780666–2.792654秒的frame265；Vulkan位于3.415369–3.426835秒frame329。均在冷加载期间。CSV复用引擎采样统计，下面区间是该列<1024时所覆盖的帧时长，不能解释为更高频独立OS采样。JSON保留全部区间、0值数量及实际列最小/最大，没有补造峰值。

cpu：

| CSV起止秒 | 首末帧（0起） | 该区间最低可用MiB | 累计帧时长秒 |
|---|---:|---:|---:|
| 2.503857–2.860337 | 241–270 | 149.3516 | 0.356480 |
| 3.434534–17.640907 | 322–1595 | 326.6094 | 14.206372 |
| 74.420259–75.064890 | 6760–6814 | 988.6328 | 0.644632 |
| 75.621601–76.223822 | 6861–6912 | 1007.7461 | 0.602222 |
| 76.792945–77.504513 | 6963–7024 | 1013.5625 | 0.711568 |
| 77.514792–77.540348 | 7026–7027 | 1023.9961 | 0.025556 |
| 77.800011–78.338205 | 7051–7099 | 1017.0742 | 0.538195 |
| 78.513191–78.709755 | 7116–7132 | 1022.5625 | 0.196563 |
| 78.754167–130.416642 | 7137–11651 | 791.7188 | 51.662475 |

vulkan：

| CSV起止秒 | 首末帧（0起） | 该区间最低可用MiB | 累计帧时长秒 |
|---|---:|---:|---:|
| 3.246040–3.490548 | 314–335 | 189.7695 | 0.244508 |
| 7.898221–15.197789 | 696–1331 | 469.7422 | 7.299568 |
| 15.213293–15.238410 | 1333–1334 | 1019.2188 | 0.025117 |
| 32.379123–32.708910 | 2937–2965 | 1003.0391 | 0.329787 |

CPU四个>50ms帧位于CSV累计约10.073、41.412、102.515、130.402秒。两帧紧邻41.369635、102.469870秒的GC事件；末帧与HTTP超时/StopServer清理相邻。CSV事件只支持时间关联，不能据此把全部长帧归因GC或进程终止。实际GPUTime在四帧均约7.1–7.4ms；尾帧GameThreadTime76.7445ms。没有将模型等待131秒记作UI单帧时长。

Vulkan既有`runtime-resource-snapshot.json`只提供一次globalGPU snapshot：86°C、5505/8188MiB、83.53W；当时AvailablePhysical1147817984bytes、memoryload93%、availablecommit3352653824bytes。该时点不代表全程峰值。psutil缺失导致进程采样未取到，保留NOT_RUN；不安装、不补终点或模型RSS信用。

两次1%Low均低于60FPS。单场景尚未满足全部三项帧门槛；CPU整体还因无完整模型回复FAIL。可用RAM<1GiB是真实余量风险；未发现本次日志OOM，不等于通过完整no-OOM/内存稳定门槛。五场景、暖P95、首次反馈≤0.2秒、Shipping、双机器与全程模型进程/全局GPU资源均未覆盖。

## 最小后续诊断

已提供[仅model_csv日志环境增量](./normal-new-model-server-log-proposal.patch)：在自有UE launch之前设`LLAMA_ARG_LOG_FILE=<本次out>/local-ai.log`并记录路径。UEClient Popen没有显式env覆盖，Windows UE CreateProcess环境参数nullptr，两层继承host进程环境。只作用于这次host及其子进程；host退出后不修改调用shell/注册表/正式配置，也不改变模型、线程、GPU层、预算、输入、采样、HTTP120秒或权限。

锁定[b10964 common/arg.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/common/arg.cpp#L3758-L3764)把LLAMA_ARG_LOG_FILE映射到内置日志文件。锁定[server-context.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-context.cpp#L534-L605)具备info等级prompt/decode进度和完成timings。默认info阶段日志有真实节流：prefill累计≥3秒才打印progress；decode须n_gen≥100且距离上次≥3秒才打印进度。因此69token等短回复可能仅有完成timings，不能预先承诺每token、全部stdout或完整最后一个batch日志。[common/log.cpp](https://github.com/ggml-org/llama.cpp/blob/b10964/common/log.cpp#L97-L155)将内置日志异步写文件，每条已写记录fflush；TerminateProc可能丢掉尚在队列的末尾记录。env在参数解析中先于CLI加载，现有Runtime没有log-file CLI覆盖。slot new-prompt/context是TRACE，逐token是DEBUG；cancel分支仅release，不打印final timings。文件存在与实际覆盖仍须下次核对。

建议Root仅再执行一次CPU相同normal_new单请求，保留既定参数和原话。先核验真实日志文件存在，再用task/slot时间和token推进定位prefill停滞、decode耗时或server报错，并与CSV RAM低余量时间对齐。文件不存在即记录日志接入失败，不再据终态猜瓶颈。不重复已完成Vulkan。若仍被120秒终止，保留业务失败及能取得的截止前日志，不延长超时或改门槛。
