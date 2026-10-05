# b10964 Qwen3.5 GDN CPU prefill 有界工程资料审阅

没有找到官方已确认的b10964／Windows CPU／Qwen3.5-4B／4线程prefill特定缺陷。本次实际Game最后可见2661 tokens / 106.70秒 / 24.94 tokens/s；源码与历史记录只支持下列三项结论，不能确定本次超时的算子主因。仅阅读锁文件、已有文件目录和上游primary PR/source，没有运行UE、模型、bench或修改参数、依赖。

当前[local-ai.lock.json](G:/GameFactory/Hearthward/.agent-local/task051/Config/local-ai.lock.json)登记b10964官方win-cpu-x64 artifact、Qwen3.5-4B Q4_K_M。CPU目录包含alderlake及其他ISA DLL；本次verbosity3日志没有实际加载DLL记录，不能根据存在某个文件就判定选择了该实现。

1. [GDN fused op PR #19504](https://github.com/ggml-org/llama.cpp/pull/19504)于b8233合入，最初CPU实现保留逐token递推。当前[b10964 CPU kernel](https://github.com/ggml-org/llama.cpp/blob/b10964/ggml/src/ggml-cpu/ops.cpp#L10967)按head×sequence分片，第10991行在各片内串行遍历tokens，第11049–11088行在工作线程间领取分片；`nth*4`的4是每线程分块倍率，没有`nth==4`专用分支或4线程上限。当前[delta-net-base.cpp435–446](https://github.com/ggml-org/llama.cpp/blob/b10964/src/models/delta-net-base.cpp#L435)在多token且fused_gdn_ch时选择相同fused op。该源码说明CPU内核并行边界，未证明它占据本次全部prefill时间；旧PR的其他模型/线程数吞吐表不用于预测本机4B结果。

2. [state布局与SIMD优化 PR #20443](https://github.com/ggml-org/llama.cpp/pull/20443)于2026-03-13合入，消除冗余state转置并调整CPU内层访问/向量运算。b10964源码第10999–11000行仍是连续状态行，第11018/11024/11030行分别使用vector dot/mad/dot，确认已包含这些改变。该旧修复不能直接解释当前版本的退化。上游公开回归幅度来自Metal/M4/Qwen3.5-9B，不能转换成本次Windows CPU收益。[当前内核](https://github.com/ggml-org/llama.cpp/blob/b10964/ggml/src/ggml-cpu/ops.cpp#L10999)

3. [CPU/OpenMP server线程回归 PR #27133](https://github.com/ggml-org/llama.cpp/pull/27133)于b10447合入，官方回归分析限定b10429–b10446，将ggml计算保留在当前线程。b10964的[server_queue::yield_to_queue222–238](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-queue.cpp#L222)仍直接在当前线程调用work()，已经包含该修复。[官方分析](https://github.com/ggml-org/llama.cpp/discussions/27111)没有本次WinCPU/4B/4线程prefill的实测，无法将约25 tokens/s归为同一旧问题。

当前[llama-context.cpp232–234](https://github.com/ggml-org/llama.cpp/blob/b10964/src/llama-context.cpp#L232)默认fused_gdn_ar/ch=true、auto_fgdn=false，560–564的自动支持探测受后者控制。只能按这些准确条件描述默认路径，不能声称默认总会自动探测backend支持。

上述关键源码经官方raw URL直接只读取得并核对；本次b10964 ops.cpp实际12164行、delta-net-base.cpp606行。搜索工具返回的raw缓存曾分别只有11254与546行、对应位置不同，因此最终使用直接取得的tag源码行号，未作hash或修改源文件。

现有证据支持保持当前参数与锁版本，把CPU单请求门槛记录为失败。实际选中的AVX库、GDN占全部prefill的比例、硬缺页/CPU频率及线程调整收益均未知；没有资料支持直接认定升级运行库、改线程或禁用fused可以修复本次120秒失败。后续若要定向诊断，应由Root另行选择最小实际测量，不增加完整矩阵。
