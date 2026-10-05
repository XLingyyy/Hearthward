# TASK-072 正常新游戏单请求＋CSV采样增量提案

2026-10-05。基于 Root 当前269行 RC host 的最小 opt-in QA 增量，文件为 `normal-new-model-csv-proposal.patch`；不复制启动/引用实现，不改旧 `run_performance.py` 或生产文件。Root已报告正常 RC reference smoke 实际通过：pool前缀1d8b6d41、host0、PID37560退出；本提案的模型和新 CSV **NOT_RUN**，candidate AST PASS（471行）。

## Root执行方法

将补丁集成到已有 `docs/qa/TASK-072/run_standalone_rc_smoke_proposal.py`，审阅后串行执行一次所选后端：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-072/run_standalone_rc_smoke_proposal.py' --model-csv --backend vulkan --bundle 'G:/GameFactory/Hearthward/Runtime/LocalAI' --label normal-new-vulkan-single-request --timeout 300
```

CPU独立生命周期仅把 `--backend` 换为 `cpu`、label换为 `normal-new-cpu-single-request`；本次先执行Root选定的一次，不自动双后端或重复请求。每次沿已有 UUID/pool/output/UserDir/自有PID机制。Root当前 worktree 的 Runtime 缺 GGUF/server；复用既有原目录 bundle，仅检查文件 metadata，无复制、下载、模型 bytes 或新 hash。

`--model-csv` 未设置时保留 reference-only行为。opt-in 向当前 owned `RemoteControl.ini` 的精确类/函数规则追加必要公开函数，同时仅该分支追加 `bAllowConsoleCommandRemoteExecution=True`；保留重复 `CustomAllowedRemoteFunctionCalls=` 无 `+`、quoted ClassPath/Optional FunctionName、AllowAny=false、child=false与未列 IsDedicatedServer 400控制。无受禁 settings CDO读回，无宽泛Class规则，无永久Config变更。

启动明确记录既有 `HearthwardAIBackend=cpu|vulkan`、GpuLayers=16和原bundle路径。现runtime仍CPU ngl0/Vulkan ngl16、server c4096/np1/t4/tb4、现4B模型、3328输入／256输出预算。无参数调优。实际backend/offload还需按原engine/runtime日志核对，公开getter不提供backend证明。

## 已实际定位的独立console gate

Parent报告CPU opt-in首次（PID39792）已通过正常Dialogue、CanCommunicate和Sourcewood16，在 `probe.set.sg.ViewDistanceQuality` HTTP400停下：Executing console commands remotely is not enabled。`WebRemoteControlInternalUtils.cpp:568–579` 对ExecuteConsoleCommand另查 `bAllowConsoleCommandRemoteExecution`，RemoteControlSettings.h363的Config默认false；单独函数allowlist不会开启该gate。此次无Submit、无CSV、无模型活动；public正常quit返回true、PID已退出。

最窄补充是只在 `if args.model_csv:` 向owned session ini的policy_lines追加上述True；默认reference smoke及正式Config仍无此值，AllowAny/child/函数规则保持原值。Root已加并开始下一次实际诊断。独立增量见 `normal-new-console-remote-gate-proposal.patch`：它接在main opt-in补丁后，Root现host若已包含此行无需重复应用。没有追加其他权限。

## 场景、输入与失败路径

沿现host先BootstrapLoading false→正常new→实际自然World→自然Loading false→HUD，重新取得真实Hero/Companion/AI引用。采样场景固定为 **初始自然图＋正常交流页＋固定视角**。未经过夜袭、营地经营、路线移动或战斗的正常玩家链，报告使用 `normal_new_initial_scene_dialogue_fixed_view`，不给完整序章或其他四场景验收信用。

公开 `Companion.CanCommunicate(actual Pawn)` 必须true；读取公开 Source/Camp，Source `GetItemCount(wood)`≥1。生产 NaturalCamp 默认wood16及370cm出生布局只提供源码依据，不能代替本轮实际返回。范围、Source、Loading或暂停前置失败即保留原HTTP/阶段并结束；不定位角色、给材料、改安全/地形或跳时。

公开正常 `Screen.ExecuteAction('page:dialogue')` 成功、GetPage=dialogue、IsGamePaused=false后，使用公开 ExecuteConsoleCommand设置已批准1080p/sg3、100%、TAA、Nanite/VSM、无动态分辨率/垂直同步且不限帧；每个CVar用公开GetConsoleVariableStringValue读回。启动明确DX12/ForceRes。正式硬件、驱动、供电/温度与实际RHI等仍需Root登记/核对。

Loading结束后另等10秒，作为显式 **capture开始前** 的场景稳定窗口，不按固定1800帧删除数据。随后只提交一次原话 **“采集1份木材送入营地仓库”**，公开参数名为 Speaker/Companion/Text，来自本轮实际引用。无二次推理、固定回复或改写raw；不会ConfirmCandidate。Sourcewood在终态需保持原数量。

每秒读取实际Busy/Ready/GenerationCalls/Status，并记录 `model-progress.jsonl`；同时核对Loading=false、dialogue页不变且未暂停。采样从实际CSV文件创建开始，至少经过61秒host时间；请求尚未到真实Busy=false终态时继续采样。最终还要求 **CSV全部FrameTime累计≥60秒**；host等待时间不当帧时。

完整终态保留原输入、raw、reason、applied_intent、candidate、token、latency、filtered context及模型PID。实际GenerationCalls必须1且曾观察Busy/Ready；Gen1＋Busyfalse仍可能是失败，所以还要求真实解析后的raw绑定同一原话。collect正确卡独立记录 `expected_collect_candidate`：HasCandidate=true、reason空、applied_intent=proposal、Candidate collect/wood/1/additional_acquired/S1。clarify/refuse或错误卡保留实际结果，不能计作正确理解；`MODEL_CSV_CAPTURE_COMPLETE`仅表示本次数据采集完成，指标与候选正确性另看字段，不表示072通过。

## CSV入口与数据口径

复用现有引擎 CSV Profiler，参考[官方 CSV Profiler](https://dev.epicgames.com/documentation/unreal-engine/csv-profiler?application_version=4.27)；命令和格式按本机UE5.8源码核对。

- 公开 `KismetSystemLibrary.GetProjectSavedDirectory()`（cpp298–300）实际返回ConvertRelativePathToFull。CSV实际路径为返回目录下 `Profiling/CSV/Task072-<uuid>.csv`，必须落在本次out/User隔离目录内。
- `CsvProfile STARTFILE=Task072-<uuid>` 设置GCsvFileName；随后 `CsvProfile START` 将start排入下一BeginFrame消费队列（CsvProfiler.cpp1057–1069/3889–3895）。BeginCaptureInternal3973–3978即创建文件；10秒文件等待验证真正捕获开始。命令参数采用源码比较的大写START/STOP。
- `CsvProfile STOP` 异步结束；等待最后HasHeaderRowAtEnd metadata，未完成的部分文件不计帧信用。不设置BlockOnCaptureEnd去人为停顿game thread。
- 复用旧 `run_performance.py:77–85` 的p99索引及最慢1%均值倒数方法；追加>50ms帧数量/比例、实际累计秒、最大帧时和完整raw保留。当前单场诊断目标为p99≤16.67ms、1%Low≥60、>50ms比例≤.001。
- 保留全部采样数字帧，零值保留；缺FrameTime、非数字、负值或NaN/Inf明确失败。只跳过空行、EVENTS＋初始header完整前缀的末尾summary header，以及末行固定HasHeaderRowAtEnd/EventTimestamps metadata。metadata末尾Commandline可能有未转义逗号，不能按整行偶数键值识别，也不能按Events中的方括号误删帧。

已用**准确候选parser AST**只读解析一份既有真实UE CSV：`Profile(20260922_165259).csv`，5537数字帧、68.7040385秒、初始308列／末header314列，PASS。它具体验证末header增长及metadata识别；不给当前模型、代码或性能信用。结果在 `normal-new-model-csv-existing-parser-check.json`。

## 计时和退出边界

`GetLastLatencySeconds` 在Subsystem.cpp514的HTTP解析成功时赋值，早于ApplyProposal/Stage/UI；它记录submission-to-response，不能当暖完整UE回复p95或decode速度。另记录本次首次Ready和Busyfalse终态的host观察上界；每秒轮询有粒度，未记录首次Paint时间。此次第一请求包含冷启动，无暖p95样本、无TTFT或≤.2秒UI反馈信用。

整体host timeout沿参数最多300秒；model opt-in平时deadline=overall_deadline−10，finally恢复overall_deadline，为CSVstop＋正常quit预留10秒，不扩总时限、不改runtime冷加载120秒或HTTP业务120秒。若请求超出host采样预算，保留失败/部分CSV，不重新提交或延迟退出去凑完成。

结束先尝试已有 `Screen.ExecuteAction('quit')` 的正常生产退出分支，再finally由同一UEClient停止自有PID。正常source链为QuitGame→Shutdown/CleanupWorld→Subsystem.Deinitialize→LocalAI.StopServer→Runtime.Stop；该实际退出/模型清理仍须Root确认。HTTP quit回包可能在退出时中断，报告保留该事实；stop_request_ok单独不能证明PID或模型已退出。

## 072能覆盖的范围

若实际完成，本次可提供正常new公共入口、真实单次模型负载与该固定视角交流场景至少60秒原始GPU渲染帧时分布，以及该一次冷Ready/完整终态的观察时间。CPU模型20～30秒或更长等待只记模型时间；帧指标来自真实CSV FrameTime。

正式072仍需营地昼夜、序章、三人战斗、路线流送、经营面板各CPU/Vulkan的真实活动样本；本次固定视角和单后端不足替代。暖p95、UI首次反馈、UE／模型进程RAM/VRAM/页文件/可用RAM、无OOM完整观察、正式Shipping／第二机器／安装路径兼容均 **NOT_RUN**。单场 `diagnostic_frame_target_pass` 不解除这些门槛。074 Cook仍 NOT_RUN，本提案未执行Cook。

现成 `run_performance.py` 可复用统计方法，但它启动即固定帧数采样、盲删1800帧且没有本轮public model请求联动，不直接运行作为本次联合样本。增量只加入当前host的局部诊断分支；无新依赖、生产helper或长期QA服务，未改冻结60/20数据、Source/Content/Git，未计算hash/checksum。
