# TASK-068 施工准备与真实验收门槛

2026-10-04。仅研究和 QA；任务尚未激活，未修改生产实现，未执行 UE / 构建 / Git。

## 已确认事实

- 根隔离树没有忽略的模型/可执行文件。原目录 `G:/GameFactory/Hearthward/Runtime/LocalAI` 有锁定 GGUF（2,740,937,888 字节）及 CPU/Vulkan server/bench。现有非 Shipping 参数 `HearthwardAIBundlePath=` 能直接复用，根已授权；无需复制或下载模型。原目录 knowledge/license 与根跟踪文本相同。
- 实际 CPU `llama-server --version` 返回 build 10964，commit `b29c606e2`。已有生产配置是 context 4096、输入 3328、输出 256、parallel 1、CPU layers 0、Vulkan layers 16、thinking false、no-cache-prompt、仅 loopback。
- 24 个能力目录包含 050 的自然采集/护理、狩猎、捕鱼、捕获、设施有限生产和护送。`HearthwardAgent::Describe()` 对五种运输重复打印普通物品目录，并对 inventory / inventory_report 重复打印全部 77 物品。
- 正式输入预算已通过实际 `/apply-template` 和 `/tokenize` 检查；Full→Compact→Minimal 三层都保留固定 System。世界上下文降级不能削减 System。
- 历史 TASK-040 32 样本报告是 raw 24/32、core 20/20、safety 32/32。旧 `ok=true` 无法证明本任务 raw ≥90%。
- 062 实际 PIE 已确认 `SaveSubsystem.InitialWorld` 无 Python wrapper。`FHearthwardWorldSave` 非 BlueprintType，private UPROPERTY 也不能据此推断可被 Python 写入。Nature.State/Restore、Clock.AdvanceCalendar 同样无对应公开 Python 方法。068 QA 不调用这些字段/方法。

## 本轮实际诊断结果

`probe_prompt_budget.py` 启动原目录 CPU server，threads 1，仅调用 `/apply-template`→`/tokenize`，不推理；进程已退出。

1. 七个固定目录行（五种运输 + inventory / inventory_report）和 System 字面规则、无其他能力/世界上下文：2932 tokens，ready 7.361 秒。
2. 按当前源码/JSON 重建 24 能力目录、Describe、CompanionOrderPrompt 和全部 System 固定规则，省略全部世界上下文：5462 tokens，ready 3.689 秒，0 generation。

第二项输出保存在 `prompt-budget-probe.json`。这属于真实 tokenizer 对源码重建提示词的诊断，尚不能代替正式 UE 请求的实际最小层 token 数。它说明固定目录超预算有直接证据，根应先运行一个真实 `SubmitPlayerText`，记录 `CONTEXT_OVERFLOW`、input_tokens、tier、generation_calls，随后按真实 RED 决定最小生产修复。不得提前把结构化拒绝计入模型理解。

## 最小施工建议与工程缺口

1. 先复现实际 UE 最小输入预算失败。若复现，优先消除目录中重复展开的物品表，并保留完整能力、来源授权、数量语义、硬规则和未知事实边界；Schema 的真实能力枚举仍由现有注册表生成。无须模型升级、扩窗、新 RAG 依赖或静默截断玩家原话。
2. 固定 60 表达的预期、分别跑 CPU/Vulkan。原始 JSON、UE 候选、确认和实际结果各自评分。任何 UE 后置纠正只能计入执行指标，不能修正 raw 指标。
3. 当前矩阵准备覆盖 30 个真实确认执行：采集、制作、GUID 修理、伙伴指令及五向物资运输。其余 10 清晰表达覆盖认知、规则确认和取消；模糊/越权各 10。实际 world fixture 通过公开 `TryAdd/TryRemove/StorageAccess.Transfer` 和公开 SavePoint/LoadPoint 生成。
4. 新自然能力、hunt/fish/capture/camp_batch/escort 仍需附加真实模型执行覆盖，双营地连续剧情/改口/旧约束、角色措辞与 Owner 评价也尚未完成。不能仅用基础 60/30/20 或已有原生测试关闭这些门槛。
5. API 预算、真实模型不可用/负载/中断的玩家反馈和 072 联合性能须单列。provider readiness 与 bench 的 token/s 无法替代游戏内完整延迟和帧时间。

## QA 使用

只由根串行运行，默认引用既有本地 bundle。首个实际诊断使用：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-068/run_pie.py --backend cpu --cases 1 --timeout 300
```

`--cases 1` 会输出真实单样本结果，完整门槛保持未通过，进程正常停止；退出码 1 属于未完成矩阵。完整 CPU/Vulkan 各运行 `--cases 60`，各自生成 `Saved/Task068/<backend>/results.json`、cases.jsonl 和 boundaries.jsonl。不得同时运行两个 UE 实例。

最低门槛：raw 54/60、模糊/越权 raw 18/20、clear E2E 36/40、真实执行 29/30、边界 20/20。边界使用确定性公共 API，单独标注，不获得语言理解分数。每项失败记录继续保留；任何 fixture 前置或反射失败记录为失败，不发明 setter。

静态验证：Python AST 通过；JSON 60 条（40/10/10）、30 条要求真实确认执行。UE 尚未执行，因此不能报告矩阵/执行/边界通过。

## 采用的主要官方与开源工程经验

- [llama.cpp b10964 server](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/README.md)：通过真实 chat template 和 tokenizer 计算完整输入；当前项目已使用相同路径，继续复用。
- [llama.cpp b10964 grammar](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md)：JSON Schema 的结构约束与语义、权限、世界状态正确性分别验证。
- [Qwen3.5-4B 官方模型卡](https://huggingface.co/Qwen/Qwen3.5-4B/blob/main/README.md)：使用 `chat_template_kwargs.enable_thinking=false`；项目已配置此项。模型卡不提供本项目游戏语义正确率保证。
- [τ-bench 论文](https://arxiv.org/abs/2406.12045)：工具调用评估检查最终环境状态和规则遵守。此处用真实库存差额、GUID 耐久、交付量和取消边界核对执行，避免只判断回复文本。
- [EleutherAI evaluation harness API](https://github.com/EleutherAI/lm-evaluation-harness/blob/main/docs/python-api.md)：保留固定数据版本、模型输入/输出和逐样本指标。此处复用 JSONL 记录方法，不引入训练/评测框架依赖。
- [llama-bench b10964](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/llama-bench/README.md)：模型吞吐基准独立于游戏完整请求延迟，本轮不执行长性能 bench。
