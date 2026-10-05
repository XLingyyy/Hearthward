# TASK068 数量边界与知识来源调查

本次仅写子树QA及未应用补丁草稿。没有修改Source、Runtime实际bundle、冻结60输入/expected、预算3328/256、模型/采样或任何私人存档；没有UE/build/Git/model/HTTP运行。

## 先处理已证实的数量确认缺口

根真实B诊断 `Saved/Task068/vulkan-validator-aligned-diagnostic-10/cases.jsonl` 的U03原话为“新采三十三份木材带回仓库”。实际raw输出collect/wood/32/additional_acquired/S1，limits[]、unresolved[]，台词同时承认请求三十三。实际candidate=true、status等待确认、e2e_pass=false。尚未确认物品没有增益，不能据此把错误候选视为正确。

首个失守位置是AgentInteraction.cpp248–255：只查Number.FindNext()，没有核对捕获的真实数量。其后AgentContract.cpp224仅检查模型quantity在注册上限内；33已被模型截为32便通过。B仅对齐现Validate的writes unresolved max0及rule限1，raw仍5/10，clear5/7、执行4/6、extra0；U03确认门退化是真实生产缺口。

LocalAISubsystem.cpp44–63存在LocalAIChineseQuantity(0–99)和LocalAIContainsExplicitQuantity，后者只用于inventory_report607。它不适合直接作为新写入guard：ASCII任意Contains会把32匹配到132；中文Contains二份也会命中十二份末尾；未区分目标数量与max消耗数量。没有现成的反向中文整数parser。

最小候选仅加强既有数量槽核对，不能回写模型数量或宣称raw理解变好：

- 先使用现已选Goal.Intent/Item；不新增自然语言意图分类器。原话保留，现负数/小数guard保留。
- 捕获完整数字token及单位，完整数字字符边界须含零/〇/两/百/千/万，防止从一百三十二中仅取三十二。从完整ASCIIcapture核对整数值；中文对受支持的小整数复用现0–99规范格式化、两→二即可，大数或不支持写法保留并澄清，不取后缀/截断。
- craft只把“批”作为目标单位。采集/搬运优先用Normalize(ItemText)相邻的目标数量短语，兼容“二份物品”及现GoalText“物品 × 2 份”；不将原话任意一个数字视为任务量。显式数量补充沿现WorkingGoal/Original流程核对，不凭最新库存、示例或max限制补量。
- 原话目标量缺失、多义或与Goal.Quantity不等时，按现unresolved流程阻止候选。明确33/模型32必须无候选，raw仍记失败；超上限不能改成合法量。
- 目标数量与消耗预算按单位/目标物区分。“做些绳索，最多耗三份木材”不含批数；“制作二批箭矢，最多耗五份木材”的目标为2，消费5不能让quantity5通过。若无法可靠归属，澄清，禁止猜选。
- 正常SetStructuredGoal公开路径使用真实GoalText“物品×N单位”，须保持可用。AdjustCandidate是玩家明确UI加减，370–375绕过StageCandidate，保留其原有行为。

准确潜在生产窗口：AgentInteraction.cpp248数量检查；若抽取现ChineseQuantity供跨文件复用，需明确登记AgentContract.h/.cpp或LocalAIContext.h/.cpp真实业务接口，不能新增仅测试入口。当前未授权写生产，不提供未审查的语义parser。代表验收应包括完整ASCII33→32、中文三十三→32、十二→2、132→32、真实合法量正控制、craft不同目标/消费数量、只有预算无目标、现结构化GoalText和明确澄清。现Native契约测试只覆盖Goal量域，未覆盖Stage原话一致性，不能代替真实U03确认门回归。

## 陈旧safety知识命中事实

实际SendInference401从Runtime.GetBundlePath/knowledge.json读资料，运行bundle override指向原仓库G:/GameFactory/Hearthward/Runtime/LocalAI。root工作树同路径也保留旧safety句。RetrieveKnowledge只处理initial_known，topic同hint加3，每个原话terms子串加1，稳定降序取前三；ClassifyHint没有safety分支。

逐条按现代码重建冻结60检索，safety-001命中为0。旧句“当前可执行接口只有安全采集和取消”确与完整24能力冲突，但本次60及10语义失败均不能归因于它。完整排名证据见knowledge-retrieval-static-evidence.json；全60真实tier均full_relevant，因此有分数的RAG按现代码会进入发送消息。GetLastFilteredContext118仅保存Projection.Json，未保存Retrieved，不能凭报告filtered_context直接证明实际检索文本。

| 诊断输入ID | 当前静态检索 |
|---|---|
| C01/C19/U01/U03 | gather-001、inventory-001 |
| C08/A01 | gather-001 |
| C09/C18 | inventory-001 |
| C27 | 无 |
| C40 | cancel-001 |

实际knowledge的gather不含制作默认bag/限制协议，inventory只描述认知可信度，cancel只描述取消和已结算物品；没有sourcecamp、最终运输终点或草药→药膏的错误指令。仍无法将这些语义错误归因于检索；当前数据支持“模型原始槽位翻译错误”，无法由外部结果证明其内部唯一原因或模型容量不足。

## 项目知识与外部运行包的最小对齐草稿

project-knowledge-candidate.patch为未应用两文件候选：

1. LocalAISubsystem.cpp401改从ProjectDir/Runtime/LocalAI/knowledge.json读取游戏知识；Runtime bundle override仍选择实际exe/model，不扩写fallback，不读取或写原tree。
2. root自己的Runtime/LocalAI/knowledge.json只更新version及safety文本，去掉固定能力白名单，保留独立安全限制，明确同行提案按当前目录、目标安全由UE现场复核。完整能力仍由当前注册Describe生成，知识不复制24行目录。

Build.cs26–44当前已把项目Runtime的json作为NonUFS收集；prepare_bundle.py仅下载exe/gguf，不生成knowledge。此候选不新增依赖、文件、Build规则或下载；开发共享模型包时，游戏知识跟当前项目分支走，发行仍是原路径。需要根先登记Runtime/LocalAI/knowledge.json准确写窗口。冻结60 safety命中为0，修正该句不能宣称提高当前raw，也不值得单独为它再跑60。

## 最小下一提示候选和证据边界

llama.cpp b10964明确说明response_format Schema只约束输出，不注入提示；模型不会看到Schema本身。因此现“只输出Schema的8字段”不足以让未显示的Schema成为语义知识。现Describe已显示intent/item关系和quantity/mode/source，但limits/unresolved/npc_line依靠短规则和斜线示例。可替换一个现有斜线例为完整8字段紧凑JSON示例，展示可执行/澄清分支的真实空值规则；保持非冻结物品/数值，并先做真实token计数，不能追加多组例超过预算。[llama.cpp b10964 grammar说明](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md)

另一个更窄、字数可减少的候选是替换现“先核对动作…”规则，先表达通用分支选择：缺项/不可表达限制先选clarify，负数/小数/超上限先选refuse；这两类item=none、quantity=0、mode/source=none，限制原文留unresolved或npc_line；只有明确合法请求才选择能力整行。它不能恢复被错误Grammar分支遮掉的语义，也不能修复运输终点，须保存真实raw作判断。建议先完成U03 UE数量门，再决定单变量提示试验，避免同时改知识/Schema/提示后猜测因果。

Qwen3.5官方卡片给出chat_template_kwargs.enable_thinking=false的Non-Thinking用法，与当前请求一致；不能用/think或/nothink字符串代替。官方Agent示例依靠明确工具定义，支持保持真实目录为权威，未支持通过低权限旧知识重定义能力。此处不改变模式、采样、输出预算，不据该卡片推断项目错误的唯一根因。[Qwen3.5-4B官方卡片](https://huggingface.co/Qwen/Qwen3.5-4B)

以上均为工程候选或静态事实，不构成新增真实模型/Native PASS，也不改原60正式门槛。
