# TASK-072 独立Game Development构建与正常运行诊断

2026-10-05，当前未提交批次。使用公开UEClient.build.package与runtime.launch_packaged；输出F:/uagent-task-temp/task072-game-development-20261005，未冻结RC、未发布。原同G卷既有模型和两后端文件64项硬链接到ignored Runtime目录供原Build.cs依赖；未复制GGUF到G、未改模型/runtime版本或依赖规则。完整成品内部模型来自本次F Archive/Hearthward/Runtime/LocalAI，运行没有使用原工程bundle。

首个Game编译真实Exit6：HearthwardWorldPresentation.cpp100/104调用GetComponent，UE5.8把该接口放在WITH_EDITORONLY_DATA。精准登记后仅两行改为运行时GetLightComponent，创建时保留Directional cast；引擎构造函数确认两接口指向同一组件。颜色/强度/时序保持原值。实际Game+Editor增量构建48.83秒Exit0、Cook97.52秒Exit0，Cook summary0error/0warning，Stage/IoStore/Pak/Archive均Exit0。内层真实Hearthward.exe338619904字节与GGUF2740937888字节、两后端server/DLL/OpenMP许可及Resources/Fonts/policy进入清单。顶层bootstrap不作为持有Game进程。

首个成品公共引用已通过但QA在GetPage返回Title时按字符串title误判；该函数以及Goal Intent/Item为FName、大小写不敏感。只修QA四个page及两候选FName比较casefold；FString SourceRef/QuantityMode/原话/raw保持精确，不改生产，不重Cook。原GetPage=Title/HTTP200和FAIL结果保存game-title-fname-case-red-*。

后续CPU/Vulkan均从正常Bootstrap title→public new→正式自然图/HUD→实际30米交流检查→Dialogue启动。使用真实Actor/World/PC/HUD/Subsystem引用（运行对象214748...由返回路径取得）、精确函数白名单、原生1080p/sg3/DX12/TAA/Nanite/VSM、100%渲染/无限帧/无VSync，经真实CVar读回。Loading false后settle10秒，真正创建CSV后各Submit一次“采集1份木材送入营地仓库”；未定位人物、授物或Confirm，Source木材16→16。

|实际成品条件|CPU|Vulkan|
|---|---:|---:|
|pool|99246739-0d4c-46d7-b4aa-8004fa5253bc|be88f3b0-7b14-42da-84f8-c8eea5cd64df|
|ready观察|8.889秒|7.659秒|
|完整UE终态观察|129.539秒，120秒HTTP超时|64.069秒，正确collect wood1 S1候选|
|输入/输出|3177/0|3177/69|
|帧数/总时长|13553/129.6783524秒|7012/64.1052673秒|
|p99|14.3746ms|14.0511ms|
|1%Low|58.16995FPS|61.43688FPS|
|>50ms帧|1/0.0073784%|0|
|帧target|false|true，仅本场|
|host退出|1，真实失败|0，MODEL_CSV_CAPTURE_COMPLETE|

CPU原生日志至少106.70秒prompt处理到2661token/progress.84，仍无完整prefill/decode终态；没有因减少Editor工作集而解决超时。CSV UE工作集2482.33–2748.29MiB，系统可用RAM最低612.95MiB、<1GiB累计121.0113秒；低余量仍真实存在，不证明换页/节流原因。一次中途进程查询为Game2612088832/model4474679296字节，仅单点；后续系统资源快照在终态清理后，标签已纠正，不算运行中峰值。与Editor诊断的4419.72–4859.18MiB相比Game工作集更低；其输入3177与Editor3173相差4token，own_bag的FName显示大小写也不同，不能称完全相同prompt。独立资源与阶段记录见game-cpu-completion-log-independent-review.*。

Vulkan日志完整prompt3177/30.80884秒，decode69/25.23680秒。一次冷请求完整64.069秒不作为暖p95；ACCEPTANCE冷门槛衡量model ready，不能拿完整回复时间替代ready门槛。该场景帧分布通过登记三项，但CPU帧仍失败，五场景、暖60样本、首次UI绘制、完整资源峰值/热条件、Shipping和第二机器尚未完成。

两次均public quit=true、UEClient stop_ok，Root按实际PID核实Game32572/42856、模型20820/49892已不存在；不称物理OS退出验收。所有原始CSV/HTTP/进展/launch/results/stop/session ini与原生日志保留game-development-normal-new-{cpu,vulkan}-*。原始结果中的NOT_VERIFIED_BY_UECLIENT不覆写，独立PID核实文件补充实际事实。当前任务仍Active。

构建、Cook与Stage路径/清单见[独立清单审阅](game-development-cook-stage-archive-independent-review.md)，[接口等价性](game-directional-light-runtime-interface-review.md)，[FName契约](game-rc-fname-case-comparison-review.md)。[Epic打包文档](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project)说明该构建流程；本报告的通过结论以实际返回值及原始日志为依据。
