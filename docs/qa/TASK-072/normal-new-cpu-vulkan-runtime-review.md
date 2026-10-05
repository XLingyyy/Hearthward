# TASK-072 初始正常场景的模型与帧时诊断

2026-10-05，UE5.8.2 Editor Development -game，当前未提交批次，基线67fb0784ca8c6d488173e587e7f95c4be0d9092a。正常Bootstrap new→正式自然图，实际30米交流检查及Source木材16、Dialogue/不暂停通过；没有Actor定位、授物或世界夹具。CPU/Vulkan各只提交一次“采集1份木材送入营地仓库”，无Confirm；输出不同独立UUID池及UserDir。既有模型Qwen3.5-4B-Q4_K_M/llama b10964、c4096/3328输入预算/256输出、单并发/4线程、CPU0/Vulkan16层沿原配置。

启动原生1920×1080、DX12，全部sg3、100%渲染、原生TAA、Nanite/VSM、无动态分辨率/VSync/限帧均经实际CVar读回。正常Loading结束后额外settle10秒再START；公共GetProjectSavedDirectory绑定本次输出，真正创建CSV才Submit。STOP后等待末尾完整metadata/header，所有数字FrameTime保留，没有截掉1800帧或排除卡顿。请求未到终态时继续采样；普通HTTP/坏数据立即留证失败，总timeout300秒中预留10秒正常清理。opt-in临时session ini另启用引擎独立console执行开关用于本次画质/CSV，精确函数名单仍保留；永久Config不改。

|实际条件|CPU|Vulkan|
|---|---:|---:|
|UUID|7ae656be-42d4-4667-9573-33c5799cdbcb|29dd5b02-c8e5-44d6-a84d-385afca4ee94|
|模型ready观察|11.013秒|13.685秒|
|完整UE终态观察|131.197秒，MODEL_UNAVAILABLE|40.905秒，正确collect/wood/1/S1候选|
|实际请求/生成|1/1|1/1|
|输入/输出token|3173/0|3173/69|
|实际帧数/时长|11727 / 131.2467241秒|5890 / 61.601223秒|
|p99|15.9546ms|16.5024ms|
|1% Low|53.2438327FPS|52.3017127FPS|
|>50ms帧|4，0.0341093%|0|
|最大帧|75.5412ms|44.9840ms|
|采集点库存|16→16|16→16|

CPU模型ready及tokenize成功后，实际completion HTTP等待触发120秒既有超时；随后生产StopServer与MODEL_UNAVAILABLE，终态PID0不能用于判断模型先崩溃。未生成JSON/候选，host1。Vulkan成功回复原始结构和候选，host0仅表示MODEL_CSV_CAPTURE_COMPLETE；其帧指标中1%Low未到60，两次diagnostic_frame_target_pass均false。没有执行或交付采集任务。

Vulkantimings明确cache_n0、prompt3173/14.786473秒、predicted69/12.575846秒；GetLastLatencySeconds40.034秒早于完整UE终态，不能替代完整回复p95。两次都是首次冷请求，不计暖p95、TTFT或首次UI绘制反馈。CSV有效MemoryFreeMB在CPU最低149.3516MiB、低于1024MiB累计68.9441秒；Vulkan最低189.7695MiB、累计7.8990秒。具体列语义/区间/GPU列见cpu-vulkan-normal-new-existing-csv-resource-review.json；GPU CSV列只代表其实际引擎统计，不等价总显存。

一次Vulkan结束前资源快照为物理可用1147817984字节、内存负载93%、全局GPU5505/8188MiB、86℃/83.53W；只绑定该时点，不代表峰值。psutil未安装，没有安装新依赖，进程RSS查询错过退出，CPU温度未取得；没有把这些标成已测。内存余量风险已经实测，CPU超时的具体prefill/decode/换页原因仍需下一次局部日志证据，未依据最终错误归因。

两次均正常public quit及公共stop_ok，Root按实际PID确认UE33024/48548、模型16864/36688已消失。原frames.csv、frame-report/results/http-events/model-progress/launch/stop/session ini分别保留normal-new-cpu-*和normal-new-vulkan-*，原引擎日志与输出仍在Saved/Task072唯一目录。初次未启console开关的真实400停在画质命令，无Submit/CSV，单独存model-csv-console-policy-red-*。

这是一场固定初始自然图交流视角的工程诊断；场景表、移动/战斗/经营、暖至少60样本、完整内存峰值/热状态、Shipping和第二机器未验收。本项没有正式性能通过，任务仍Active。

后续仅CPU日志诊断已完成：至少102.85秒处理提示词至2657token/progress0.84，随后120秒HTTP超时；MemoryFreeMB最低6.973MiB。原两次数据与结论保留，新增证据见[CPU日志诊断](normal-new-cpu-log-diagnosis-runtime-review.md)。
