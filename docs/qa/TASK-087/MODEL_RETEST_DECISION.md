# TASK-087 模型复测与契约决策记录

## 2026-10-07 已批准的离线候选比较

用户已通过 DSGN-004 批准推荐方案7B：可比较其他离线模型及参数，保留单次生成、UE验证、原质量阈值和资源约束。以下历史“没有变更授权/待Owner决策”不再代表当前方向状态。

Qwen3-4B-Instruct-2507 Q4_K_M 完整60题结果：原始26/60、受限0/20、明确E2E26/40、行为22/30，边界20/20、白得物品0；语言质量 FAIL，未替换正式模型。证据：[原始报告](qwen3-2507-vulkan-20261007-02.json)、[逐题CSV](qwen3-2507-vulkan-20261007-02.csv)。测试使用隔离候选目录，硬链接别名兼容既有文件名，报告保留真实模型来源。同期编辑器/制作负载存在，耗时仅作诊断，不登记正式性能通过。首次准备缺少knowledge.json的失败记录保留，未混入本次完整结果。

Qwen3-8B Q4_K_M 下载曾读取超时，续传时遇G盘空间不足；本次生成的partial移到F:/HearthwardModelTrials/qwen3-8b后恢复工程盘约3.1GB空间，按固定官方revision、HTTP Content-Range分段续传，最终5,027,783,488字节已完成。候选位于独立目录。首轮32层Vulkan仅完成13/60：原始与明确E2E各11/13；多次暖请求约27—39秒。终端中断未留下UEClient关闭回执，两个精确PID已退出，按异常中断的部分诊断归档，不作完整质量结论。证据：[32层部分结果](qwen3-8b-32layer-partial-20261007.json)。启动器原本将显式GPU层数截到32；取消该上限后已编译，并通过相关7项原生测试（0 warning/error），CPU仍0、默认16及正式模型保持。实际模型进程参数确认为37层。37层首5题5/5正确，但四个暖请求为36.0、37.344、38.0、36.672秒；固定60暖样本p95最近秩为57，已有四个>10秒，故本窗口无法达标，按延迟门槛提前拒绝此配置，未伪造完整p95或60题质量成绩。通过显式停止文件由持有UEClient的启动器正常关闭成功。[37层部分结果与停止依据](qwen3-8b-37layer-partial-20261007.json)、[原生结果](offload-lifecycle-native-20261007.json)。两个8B配置未选用，正式模型不变。

日期：2026-10-07。当前源码版本 `0.2.0-preview.20261007.2`，共享分支 `codex/TASK-084-103-iteration`，基线 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加未提交改动。当前 087 Source.2 与下述已测 AI 实现相同；其他任务后续音频构建不迁移为新的模型或 087 成绩。未提交、未推送、未发布。

## 证据范围

|受测集合|实际状态|证据及边界|
|---|---|---|
|087 生命周期与投影原生|7/7 Success，0警告/错误|[原生子集](native-green-20261007-01.json)：生命周期3项、投影4项；投影此前4/4 Fail的RED保留。|
|独立兼容原生|3/3 Success，0警告/错误|[兼容子集](native-compatibility-green-20261007-01.json)：079两项、BoundedContextProjection一项；不加进087的7分母。|
|该次联合原生|16项中13 Success、3 Fail|`.agent-local/qa/TASK-087-099/native-green-20261007-01/index.json`；3项Fail属于当时099新增用例。目录名green不表示联合全通过，后续099修复另绑定自己的证据。|
|受限输入修复前本轮Vulkan原60|原始33/60，受限2/20，明确E2E34/40，行为26/30；语言门槛FAIL|[完整RED](vulkan-full-red-20261007.json)；边界20/20、白得物品0，原暖59不足。该完整结果不等于Source.2最终全量成绩。|
|Source.2新投影Vulkan定向12|原始2/12，明确E2E2/6，行为2/6，受限0/6；FAIL|[定向原报告](vulkan-projection-subset-20261007-01.json)；`diagnostic_subset=true`，12条由6明确与6歧义/越权组成。只C01、C03原始正确；不替代原60/40/20/30分母。|
|CPU原C01单条诊断|空回复，MODEL_UNAVAILABLE；FAIL|[CPU单条](cpu-c01-diagnostic-20261007.json)，generation1/input3206，提交到最终status129.797秒；不作为完整CPU成绩。|
|Source.2最终Vulkan原60及固定暖辅助|原始32/60，受限2/20，明确E2E33/40，行为24/30；语言FAIL。组件窗口60暖p95=8.672秒PASS|[最终原JSON](vulkan-final-20261007-02.json)、[CSV](vulkan-final-20261007-02.csv)、[独立辅助JSONL](vulkan-final-performance-auxiliary-20261007-02.jsonl)；run_id=final-vulkan-20261007-02。边界20/20/白得0；原暖59不足保留；UIpaint/joint未验。|
|Source.2最终CPU原60及固定暖辅助|原始33/60，受限3/20，明确E2E33/40，行为25/30；语言FAIL。60暖p95=22.656秒≤30秒组件窗口PASS|[最终JSON](cpu-final-20261007-01.json)、[CSV](cpu-final-20261007-01.csv)、[独立aux](cpu-final-performance-auxiliary-20261007-01.jsonl)；边界20/20、白得0、原59暖不足保留、无cold_restart；UIpaint/joint未验。|
|CPU Development/API Standalone单场景|CAPTURE_COMPLETE_MODEL_FAILED，HTTP120.007秒失败；采集129.8656161秒/11603帧|[TASK-102独立结果](../TASK-102/settled-dialogue-cpu-20261007-01/results.json)；与旧PIE C01、087新完整CPU不同场景，不迁移超时/帧时/质量，没有Shipping/OS信用。|
|正常OS中文IME、真人角色语气、二机与TASK-102联合性能|NOT_RUN|原生或PIE模型结果不能替代这些验收。|

定向12条实际输入3169—3312 tokens，全部不超过3328；每条仅1次生成，输出55—71 tokens，均为完整八字段JSON。当前请求预算及结构能真实通过，原始语义仍失败。不得将2/12外推为原60的比例，也不得将原生GREEN写成模型理解通过。

## 实际请求与官方实现核对

1. `HearthwardAgentInteraction.cpp::CountRequest` 将选中 `Bodies[TierIndex]` 发送到 `/apply-template`，模板计数通过后把同一Body传到 `Generate`；未发现旧Body或旧投影替换路径。12条原报告均包含新角色/代词/容器字段；A03保留真实axe=2，A09相关自然候选为0，A10人物视图available=false/ids为空。
2. 本次使用 `HearthwardAgent::Schema()` 的八字段Schema，旧五字段 `HearthwardLocalAI::ResponseSchema()` 未进入该调用链。System中的Describe显式列意图、物品关系、mode/source/上限。官方说明Schema只限制输出，不自动注入Prompt；这不能被当作自动理解输入语义的机制。[b10964 grammar说明](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md#json-schemas--gbnf)
3. `response_format={type:json_object,schema:...}` 与布尔 `chat_template_kwargs.enable_thinking=false` 都是b10964支持的路径；源码分别进入 `inputs.json_schema` 和 `inputs.enable_thinking=false`。[b10964请求处理](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-common.cpp#L1108)、[布尔参数处理](https://github.com/ggml-org/llama.cpp/blob/b10964/tools/server/server-common.cpp#L1241)
4. Qwen官方4B模型卡给出同样非思考API参数。只读检查当前GGUF元数据（未加载模型、未重算指纹）：architecture=qwen35，实际模板仅enable_thinking=true时打开思考段，false走已闭合的空思考段。[Qwen3.5-4B非思考说明](https://huggingface.co/Qwen/Qwen3.5-4B#instruct-or-non-thinking-mode)
5. 查到的官方问题与本轮条件不符：[#20345](https://github.com/ggml-org/llama.cpp/issues/20345)是thinking=true时grammar失效；[#21600](https://github.com/ggml-org/llama.cpp/issues/21600)是旧b8685模板引发HTTP400；[#20196](https://github.com/ggml-org/llama.cpp/issues/20196)主复现为旧b8233/MiniMax。当前12条均返回完整结构、覆盖多个合法分支。未找到匹配本轮症状且可确认的工程缺陷，也没有据此修改引擎、模型、Prompt、Schema或参数。

当前结论限定为固定部署条件下真实语义质量未达标；未通过对照实验证明硬件、缓存、量化或模型固有能力中的哪一项造成错误，不作这种归因。

## 全量与暖辅助的执行口径

原60保持40明确、10歧义、10越权/不支持；原始理解至少54/60、受限至少18/20、明确E2E至少36/40、行为至少29/30、边界20/20、白得物品0。最终CPU/Vulkan各独立报告全部原始结果，不能选成功表达、拼接不同实现成绩或把安全拦截计成理解正确。

当前runner默认完整60且未指定CaseIds时，实际启用固定一次 `WARM-C01-01`，在原60后、20边界前执行；无需额外开关。子集不启用。辅助通过公开基线恢复复用原C01前置，不确认候选，不重试，不进入 `cases` 或语言/执行分母，另写 `performance_auxiliary.jsonl` 及报告字段。

原 `warm_component_latency` 保留原暖统计与不足状态；`warm_component_latency_with_auxiliary` 另列原/辅助暖数量、辅助失败数量及样本来源。下限60、nearest-rank p95和Vulkan10秒/CPU30秒不变。空回复、异常、提交前未ready均保留，不能靠排除坏样本获得PASS。提交前未ready时记录计划辅助失败，不额外冷启动冒充暖。endpoint是完整UE proposal/status读取，Slate paint、确认执行及joint仍不计入本指标；UI paint/joint状态均NOT_RUN。

CPU的HTTP失败路径会StopServer→Runtime.Stop→终止进程并清空ready；其内存prompt缓存不能跨进程终止保留。完整CPU后续请求若重启，记录cold_restart；只有实际仍ready的提交才记warm_followup。若始终超时，保留暖样本不足/辅助失败，不能由预计暖机代填。

主代理已完成final-vulkan-20261007-02，原60/20边界及唯一辅助均原样归档，受测Source.2对应run根tracked.patch/untracked-files.txt；原暖统计59/不足/p95=null，合并统计59+1=60、无辅助失败、p95原值8.672000000020489秒≤10秒，仅组件窗口PASS。辅助提交前ready=true、warm_followup、generation1、无确认/无未确认世界效果，单条UE观察8.327999999979511秒。原60各项32/60、2/20、33/40、24/30不受辅助影响；语言FAIL和原暖不足保持。UIpaint/joint仍NOT_RUN。随后final-cpu-20261007-01已实际完成原60/20边界及唯一辅助：原始33/60、受限3/20、明确E2E33/40、行为25/30、边界20/20、白得0，语言FAIL。原C01 cold_first实际107.67200000002049秒完成，原始/E2E/行为true，没有cold_restart；原59暖不足/p95=null保持。唯一WARM-C01-01提交前ready=true、generation1、warm_followup、无确认/无世界效果，20.32800000003772秒；合并60暖p95=22.65600000001723秒≤CPU30秒PASS，辅助失败0，UIpaint/joint仍NOT_RUN。原JSON/CSV/aux按字节归档，snapshot/runtime/slots私有路径见REPORT；不复制Vulkan成绩，不覆盖历史RED或12条定位FAIL。

## 待Owner决定

当前固定Qwen3.5-4B-Q4_K_M、b10964、3328输入/256输出、4096上下文、温度0、单并发、默认Vulkan16层、HTTP120秒及非思考部署下，修复前完整矩阵、新投影定向及Source.2最终CPU/Vulkan各原60语义均失败；最终CPU33/60、Vulkan32/60分别保留。本单继续保持Active，不能以原生工程修复完成标Done。

Source.2最终双后端固定条件复测均未达到语义门槛，Owner需决定后续模型、预算或部署策略的修订方向，决策仍PENDING。改变模型、输入/输出预算、并发/生成链或部署策略会改变已冻结契约，应先记录具体拟改差异、资源/离线及重新验收影响，再取得明确决定。当前没有这种变更授权，继续保留FAIL；不硬编码原60应答、不补造缺量/对象、不放宽阈值，也不把手动回退成功替代模型语言门槛。

仅在现有固定条件下执行获准最终复测，其他已授权工程任务继续推进。正式Owner决策、模型契约变更和最终验收结论未产生，不伪填为已批准。
