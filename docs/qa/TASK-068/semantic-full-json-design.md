# TASK068 单一完整JSON提示候选

本次只写子树QA设计与未应用patch草稿。最终候选仅两个完整8字段JSON，已覆盖同一候选文件，未保留另一分叉。没有修改任何Source、actual runtime bundle、冻结输入/expected、Schema、原始输出或采样；没有UE/build/Git/model/HTTP运行。

## 选择依据

真实B诊断仍raw5/10、clear5/7、exec4/6，extra0。C09已选camp，却把请求制作一批写成max:rope:1；C18把最终入仓的一次搬运拆成receive并遗失终点；A01/U01/U03选了collect后，在台词才描述缺量/负量/超限。不能把quantity门阻挡错误候选计为模型raw正确。

当前System的旧例仅列intent/item/quantity/mode/source斜线串，未展示完整JSON对象；唯一明确制作例又恰有max预算，缺少“无约束制作→limits[]”的完整正例。缺量旧例有中性槽，但Schema itself不会给模型显示字段含义。[llama.cpp b10964官方说明](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md)明确response_format Schema用于约束输出，不注入提示，因此完整可见JSON比“其余字段按Schema”更有明确工程依据；这不证明模型会因示例自动通过语义门。

[Qwen3.5-4B官方Best Practices](https://huggingface.co/Qwen/Qwen3.5-4B)建议在提示中明确标准输出格式，历史仅保留最终输出。本候选直接使用完整紧凑JSON和现有非Thinking最终输出历史；未改Qwen模式、sampling或输出长度。官方不同任务的采样与长输出建议不属于本次固定合同，不能据其扩大3328/256预算。

## 最终改动

semantic-full-json-candidate.patch只替换LocalAISubsystem.cpp SendInference的System字符串。仍先调用完整HearthwardAgent::Describe，然后身份、通用槽位规则和两个完整例；后续projection、RAG、澄清历史、原玩家Input、计数/API/response_format/cache全部逐字不变。AgentContract、Registry/Describe、Schema/Parse/Validate未改，因此现24能力、158个ID、507个关系完整保留。

规则先决定能否表达：缺项/多目标/不明限制用clarify并保存unresolved原文；负数/小数/超限/目录外用refuse。二者item=none、quantity=0、mode/source=none、limits=[]，明确不改量；合法请求再从完整物名和能力行填槽。保留批数/件数、总量/新增量、材料授权、规则来源、typed limits格式、未知库存、belief/episode coverage、维修实例和UE复核权威。quantity仅为目标量，绝不自动产生limits；低权限资料不补本次缺参。

两个独立参数例：

- “拿仓库材料冶炼七批金属锭”→craft/metal_ingot/7/batches/camp；limits[]、unresolved[]。展示明确仓库材料与批数不构成消耗预算。
- “我包里的十一份矿石先给你，再存进营地仓库”→store/ore/11/held_to_camp/player_bag；limits[]、unresolved[]。展示弟弟接手是中转，整行取最终仓库终点。

完整对象见semantic-full-json-examples.json，每个严格8字段，npc_line分别33/31字，只提出确认卡，不声称已交接、已批准或已执行，不含内部字段。示例输入不等于任何冻结60原句，正例物品/数字组合不取冻结表达；没有case id或输入关键词路由。

本机Resources/Data/gameplay.json确认metal_ingot为真实craftingRecipes ID，材料ore2+wood1、smelter1；ore是common堆叠物，具备store货物资格。规则/完整目录仍要求真实设施、库存、同营地和交付距离由UE复核，示例不证明当前fixture有材料或设施。

## 预算、静态证据和最窄验证

原System suffix969字符，最终1002字符，净+33；规则含例头643、两个完整例358（余1为Describe后的换行）。这是字符数，不能换算为真实token，也未宣称减少token。semantic-full-json-static-proof.json记录未运行计数；根必须使用当前完整Describe+actual projection/RAG/history/Input，经真实/apply-template和/tokenize确认3328门与actual tier。基线诊断最大3188，剩140token；若实际超预算，停止该候选的模型验收并基于实际计数收缩重复文案，不能删目录/改投影必需字段/加预算。

静态diff只有System +9/-11，其前后所有文本一致；两个对象可标准JSON解析、字段数/名称正确、writes limits[]及unresolved[]、台词长度在现普通范围内。没有C++编译或新的真实模型PASS。

根最小真实验证保持同一固定10诊断。单一新变量为System；现Schema B和数量guard保持。记录真实input_tokens/tier/raw/e2e/candidate与确认前物品变化，分别核对无预算制作limits[]、玩家→最终仓库、缺量clarify、负数/超限refuse。若仅UE guard提高E2E而raw仍错，保留失败。子集不能替代60正式门或p95；不改变4B、16layers/t4、3328/256、冻结expected、采样、raw或门槛。
