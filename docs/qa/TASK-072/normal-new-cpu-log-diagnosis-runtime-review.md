# TASK-072 CPU completion 阶段日志诊断

2026-10-05，当前未提交工程，正常 Bootstrap new→正式自然图，Editor Development -game，独立池1356d3d4-f148-41f6-b343-150ee67de779。画质、输入、预算、模型、4线程、CPU0层及单并发沿前一次诊断；只增加本次 host 进程环境 LLAMA_ARG_LOG_FILE，让其自有模型进程保存原生日志。未修改生产源码或模型参数。

一次实际 Submit“采集1份木材送入营地仓库”，gen1、3173输入token，ready观察9.985秒，完整UE终态观察130.082秒。completion既有120秒HTTP超时后返回MODEL_UNAVAILABLE，输出0、无候选，未确认，Source木材16→16。host退出1，证据如实保留。

原生local-ai.log显示prompt processing：2048token/76.88秒、2617/100.85秒、2657/102.85秒，最后progress0.84、累计25.83token/s。因此已确认至少102.85秒在提示词处理阶段。日志未记录完整prefill、decode或最终timings；剩余约17秒的进度不可证明，不能断言解码从未开始。前一次同3173输入的Vulkan提示词处理14.786秒；这是局部冷请求对照，不能替代暖p95或普遍吞吐。默认日志输出条件及文件队列见llama b10964源代码；强制Stop可能丢失末尾队列，不把最后一行当完整终态。

实际引擎CSV保存11697帧/130.124902秒，未排除数字帧：p99 15.8573ms、1%Low55.829269FPS、>50ms仅1帧/0.0085492%、最大57.7484ms。1%Low仍未满足60FPS，diagnostic_frame_target_pass=false。MemoryFreeMB最低6.9727MiB、低于1024MiB累计85.557679秒，最长56.970068–129.336799秒/72.366731秒；这些是实际系统可用物理内存列，不能证明换页、硬缺页、温度或CPU慢的因果关系。UE工作集4419.72–4859.18MiB，RHI有效GPU使用2345.39–2409.39MiB，均不等于模型进程或整机峰值。具体阶段对齐由独立只读数据审阅记录。

公共quit返回true、UEClient stop_ok；Root另外按实际PID核实UE30100与模型46992均已不存在。launch/results/stop/HTTP/model-progress/原生server与engine日志/原CSV/frame-report归档normal-new-cpu-log-diagnosis-*，原结果中NOT_VERIFIED_BY_UECLIENT保持历史原文，独立核实见process-exit-review.json。

本次只定位初始正常场景的一次CPU请求，不覆盖五场景、暖60样本、首次UI绘制、Shipping或第二机器。下一步评估独立Game Development工程诊断以分离Editor开销，不冻结正式RC、不发布。072继续Active。

原生日志实现：[参数环境变量](https://github.com/ggml-org/llama.cpp/blob/b10964/common/arg.cpp)、[server进度输出](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-context.cpp)、[日志队列](https://github.com/ggml-org/llama.cpp/blob/b10964/common/log.cpp)。
