# TASK-068 真实语言失败根因审阅与单次 Schema 实验提案

状态：只读 Source / 真实报告审阅；仅新增本目录 QA 文档、证据摘录和未应用补丁。未启动 UE、build、Git、模型或 HTTP。父 agent 已确认其 advanced runner 任务与本审阅无冲突。当前 CPU60 保持现有 System / Schema；本提案尚无实际模型结果。

## 已由真实报告确认

[完整 Vulkan60 报告](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/vulkan-two-positive-integrated-full-60-results.json)的 raw 是 **36/60（60%）**，歧义/越权组 raw **1/20（5%）**；明确组 raw 可直接计得 **35/40**，明确组 E2E **38/40**，真实执行 **30/30**，边界 **20/20**，无额外物品。24 个 raw 失败分为明确组 5、歧义组 10、越权组 9。E2E 的 fallback 和守卫结果不能抵扣 raw 错误。

60 条均 generation_calls=1、tier=full_relevant、dropped=[]；实际输入 3010–3152 token，输出 50–105 token，JSON 可解析。当前证据没有输入裁剪或输出触顶造成失败的迹象。30/30 执行只证明这批有效卡的执行路径，无法替代语言分支判断。

| 真实失败 | raw 中的错误结构 | 同一 raw 台词 / 原话表现 | 直接证据所支持的问题 |
| --- | --- | --- | --- |
| U01 / U02 / U03 | collect，数量分别 3 / 1 / 32 | 台词保留“负三”“1.5 非整数”“三十三” | 异常量已进入台词，结构仍把原量取绝对值、取整或截上限 |
| A09 / U05 / U09 | harvest / known_target；collect / S1；give / refined_ore | 台词要求明确地块、承认未知北山、说明缺稀有披风 | 尚未满足目标或目录条件时，仍选择可执行枚举值 |
| A08 / A06 / A05 / U10 | collect；前三 limits=[]，U10 limits=[ban:wood] | 台词保留避开采点、不耗耐久、后续建工作台、禁止采木 | 限制与多目标没有转成对应 clarify / refuse 分支；台词与执行结构不一致 |
| A04 / A07 / A02 | 拿货→collect/S1；补到四份→新增四份；四支→四批 | 原话只确定搬运、目标存量或件数 | 动作来源、总量/新增量、件/批的语义角色投影错误 |
| C31 / C33 / C36 | 当前疑问或历史交付查询→inventory_report | 台词分别说库存未知、口述十份、缺可靠历史记录 | 查询/报告/回忆分支混淆；未知事实还进入 unresolved |
| C39 / U07 / U08 | 禁消耗→ban:stone；建工作台→recall；自由坐标攻击→cancel | 台词复述禁消耗或说明无法执行 | 规则操作语义与中性分支分类错误，不能靠格式合法性判断理解正确 |

[最新 A04 / A08 / C39 三条报告](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/vulkan-collection-rule-three-cases-results.json)仍是 **raw 0/3**。三条 candidate=false、no_unconfirmed_world_effect=true；对应 reason 为 UNRESOLVED_COLLECTION_SOURCE、UNRESOLVED_COLLECTION_LOCATION、UNRESOLVED_CONSTRAINT。这确认新守卫拦住错卡。C39 仍把“禁止消耗石材”生成 ban:stone；模型语义没有获得通过结论。A04 / A08 的安全 E2E 通过仍分别保留 raw=false。

全部 24 条失败 raw 原字符串和最新三条相关字段，见同目录 [language-quality-selected-field-evidence.json](G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-068/language-quality-selected-field-evidence.json)。摘录不包含修改后的 raw、期望值或世界状态补造。

## 源码事实与因果推断

[当前 System](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:416)已经明确要求先判 clarify / refuse，保留原量、未解限制、最终终点，区别库存查询/报告、ban 禁采与 no 禁耗。失败项覆盖这些已有规则；逐句补更多措辞缺乏当前收益证据。Contract 的四个中性能力在 [267–268 行](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:267)使用同一组合说明，具体分支边界由 System 给出。这是分类表达负担的源码事实，尚未证明其为唯一原因。

[Schema 338–367 行](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:338)目前按 intent → item → quantity → mode → source → limits → unresolved → npc_line 添加 properties。写分支及 rule_proposal 的 unresolved 被限制为 []，与现有业务 Validate 对齐。选择写分支后，缺项无法再写进该分支的 unresolved；后生成的台词也不能改写已输出的 intent / quantity。这一约束是静态事实。**早选分支加受限值域促成“台词承认问题、结构仍执行”是待验证的因果假设**。Schema 只保证允许的结构和值域，不能从玩家原话自动判定何时该拒绝；负数变正、33 截32等 raw 也可能源于模型先错误理解语义。

推断按证据强度排序：首先是分支与约束投影错误；其次是先提交枚举、再补自然说明的生成顺序可能放大错误。小模型容量、量化、贪心采样或示例锚定尚无受控对照，不能写成已证根因。现有报告不支持归因缓存、网络或上下文降级。

## 已有实验反证

| 既有固定10实测 | Raw | 明确 E2E | 执行 | 歧义/越权 raw | 实际影响 |
| --- | --- | --- | --- | --- | --- |
| 两正例 original-budget-guard | 7/10 | 7/7 | 6/6 | 0/3 | 当前优先保留的正向行为基线 |
| unresolved-first | 0/10 | 1/7 | 0/6 | 0/3 | 十条实际首键均 unresolved，明确任务也臆造缺项；该候选已撤回 |
| 加第三个负数拒绝例 neutral-refuse | 7/10 | 6/7 | 5/6 | 1/3 | U01 改正，同时 C08 漏掉木材预算，台词仍复述预算；该负例已撤回 |
| semantic-branch-budget | 8/10 | 6/7 | 5/6 | 2/3 | A01 / U01 改善，同时 C18 store 退化成 receive |

unresolved-first 的原始前驱 raw 是 5/10，不能把它写成从两正例7/10直接退化的相邻 A/B。实验时的 Schema / System 版本也不同，0/10只能作为明确的字段顺序风险证据。详见 [原始审阅](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/schema-unresolved-first-runtime-review.md)、[负例审阅](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/semantic-neutral-refuse-runtime-review.md)、[四次提示对照](G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-068/best-clear-two-positive-independent-review.md)。上述结果支持控制单个变量，并阻止不断堆叠逐句补丁。

## 主来源与适用边界

llama.cpp **b10964** 官方 [GBNF 文档](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md#json-schemas--gbnf)明确说明 response_format 的 Schema 只约束输出，不注入 prompt。本项目用 response_format，而未把 Schema 文本加入 messages；仅新增 Schema description 无法据此教会模型语义。

b10964 [json-schema.cpp 139–140 行](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json-schema.cpp#L139)按 properties.items() 构造有序 properties；[json-schema-to-grammar.cpp 656–702 行](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json-schema-to-grammar.cpp#L656)按该序收集并拼接 required_props，required 使用集合。因此改变 properties 顺序会改变此 GBNF 路径的输出键顺序；无需同时改 required 数组。相同文件 [936–943 行](https://github.com/ggml-org/llama.cpp/blob/b10964/common/json-schema-to-grammar.cpp#L936)另有 LLAMA_USE_LLGUIDANCE 路径，本审阅没有独立确认发布二进制编译选项。已有 unresolved-first 十条实测顺序变化支持本地存在顺序效应；新候选仍须检查实际首键，不能仅凭源码声称运行时已生效。

原论文 [Let Me Speak Freely?，EMNLP 2024 §4](https://aclanthology.org/2024.emnlp-industry.91/)讨论答案字段先出与任务成绩的关系，也报告结构化格式对分类任务可以有利。它未测试 Qwen3.5-4B 或本项目八字段动作契约。作者后续 [公开消融与更新](https://github.com/appier-research/structure-gen/blob/main/updates.md)指出提示差异影响显著，且先生成自然说明字段仍可能只得到概述。这些资料支持做本地隔离实验，无法证明 npc_line-first 必然改善，也不支持撤掉 Schema 或添加第二次生成。

[Qwen3.5-4B 官方模型卡的 non-thinking 用法](https://huggingface.co/Qwen/Qwen3.5-4B/blob/main/README.md#instruct-or-non-thinking-mode)使用 chat_template_kwargs.enable_thinking=False；当前 [454–455 行](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:454)已经设置该参数。官方不支持 /think、/nothink 软切换，追加此类提示无依据。其通用采样示例与本项目冻结设置不同，不能据此断定错误来源或改变冻结参数。

## 唯一具体候选：现有 npc_line 提前

同目录 [schema-npc-line-first-diagnostic-proposal.patch](G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-068/schema-npc-line-first-diagnostic-proposal.patch)只移动 Schema() 原366行的同一条完整语句，放在原338行创建 P 后、原339行 intent 前。新 properties 序为 **npc_line → intent → item → quantity → mode → source → limits → unresolved**。新 npc_line 行号339，intent340，required 行仍367。没有新字段、解释步骤、内部推理文本、调用、token预算或台词内容要求。

试验假设：先输出项目已有的面向玩家说明，在选择枚举之前保留表述缺项、禁止或未知事实的空间；所有24分支共享完全相同的 npc_line 类型与长度约束，这个共同前缀不会根据文本内容提前筛掉中性分支。npc_line 本身没有语义约束，可以为空或错误，也可能提前承诺执行，进而加强错误；这些属于候选风险。C39 的 ban/no 细粒度映射及查询分支错误未必受此改动改善。

| 内容 | 当前值 / 提案值 |
| --- | --- |
| 分支数及能力 | Capabilities 原20项加4个中性项，共24；不动函数及 oneOf |
| 字段集合及 required | 同一8字段；required 数组的原文本和顺序也保持不变 |
| intent / item / mode / source | 同一 C.Id / C.Items / C.QuantityMode / C.Sources 枚举 |
| quantity | integer；minimum=C.Writes?1:0，maximum=C.MaxQuantity，原语句不变 |
| limits | 同一最多4项、每项120字、能力约束 pattern；rule_proposal 恰1项 |
| unresolved | 同一最多4项、每项120字；写分支及 rule_proposal 最多0项 |
| npc_line | 同一 string、maxLength=150，无新增 minLength 或内容条件 |
| 对象和解析 | additionalProperties=false、8字段；Parse 412–418 按字段名读取，不依赖键顺序 |
| System、参数、数据 | 原两正例 System、temperature0、max_tokens256、thinking=false、投影、目录、原话和所有冻结判定均不变 |

仅 JSON 对象键的生成次序改变，按字段名解释的合法值集合保持不变。GBNF 的序列语言会有意改变；这与“值域/分支保持不变”可同时成立。上述不变性基于补丁仅移动同一条语句以及源码检查，未声称已运行新 Schema 或通过原生兼容测试。

### 两个 intent-first 完整示例的影响

[System 424–425 行](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:424)的 craft/camp 七批示例及 store/player_bag 十一份示例均是 intent-first。提案保留其全部字节，避免同时改 Schema 和提示。JSON 对象语义允许顺序不同，Parse 同样按字段名取值，因此旧示例仍可作为完整格式和能力参数示例。

生成层面存在明确的不一致：模型可见的示例先写 intent，候选 GBNF 路径则先允许 npc_line。Schema 不注入 messages，模型无法提前阅读这个新顺序；生成器可能需压制示例诱导的 intent 前缀。成功生成 npc_line-first 只证明顺序改变，不证明台词具有检查效果。实际10条需同时记录首键、有效JSON及 raw 是否提高；若失败，应把这个不一致列为可能原因，保留原 System 的对照结果。本次不调整两个示例以隔离变量，也不把 System 顺序一致化预先包成第二个候选。

## 单次固定10诊断与停止条件

Root 完成当前未改配置的 CPU60 后，若开展本候选，只复用既有固定10：**C01、C08、C09、C18、C19、C27、C40、A01、U01、U03**。明确组7项为前7条，真实执行6项为 C01 / C08 / C09 / C18 / C19 / C27，C40 是取消；歧义/越权3项为 A01 / U01 / U03。

沿用当前两正例 checkpoint、原 runner、原冻结 input / expected / 阈值、同后端及固定模型配置；单次一生成，不重试、改量或修饰 raw。记录原始8字段 JSON、实际首键、generation_calls、tier、dropped、现有 raw/E2E/execution 判定和物品副作用。首先确认十条均实际 npc_line-first 且按原 Parser 有效，确认实验变量实际生效。

本地候选进入进一步评估的条件：保持全部7条明确任务 raw / E2E 与6条执行正确，且3条歧义/越权 raw 至少有1条按原 oracle 改善，无新增副作用。该“至少1条”只是单次实验筛选标准，不改变任何正式门槛。若顺序未生效、明确任务退化或歧义 raw 无改善，撤回候选，停止该条试验，不继续添加针对句子的提示。固定10的收益只能作为诊断信号，不能外推60、CPU、Owner口吻或正式性能通过。

补丁已在内存重放并确认只移动一条完整语句，未改 Root 文件。精确UTF-8字节位置、原语句、原/新顺序和静态检查结果见 [schema-npc-line-first-proposal-static-check.json](G:/GameFactory/Hearthward/.agent-local/task062/docs/qa/TASK-068/schema-npc-line-first-proposal-static-check.json)。字节偏移针对该次读取的 Root 文件快照，以补丁上下文为应用定位依据；未计算 checksum。
