# TASK-068 冻结 CPU60：首因与反馈优先级提案

只读 Root 当前报告及 Source；仅写隔离 QA 文件。未运行模型、UE、build 或 Git，未改 Source、冻结文本、模型参数、Schema 或矩阵 oracle。Owner/Reviewer XLingyyy，无 Issue。原始失败 JSON 字符串、冻结输入、期望值和实际 reason 全部保存在 `frozen-cpu60-failure-classification-evidence.json`。

## 全矩阵结论

依据 Root [cpu-current-candidate-guards-full-60-results.json](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/cpu-current-candidate-guards-full-60-results.json)及同名前缀 runtime-review：

| 门槛 | 当前实测 | 结论 |
| --- | --- | --- |
| 理解 ≥54/60 | 34/60，56.67% | 未过，距最低正确数20条 |
| 歧义/越权 raw ≥18/20 | 1/20，5% | 未过，距最低正确数17条 |
| 明确端到端 ≥36/40 | 36/40，90% | 恰达本项门槛，仍有四条失败 |
| 执行 ≥29/30 | 28/30，93.33% | 未过；C14/C21未生成正确待确认卡，不是已确认任务的结算失败 |
| 至少20边界正确 | 20/20 | 本轮通过 |
| 额外物品0 | 0 | 本轮通过 |

26条 raw 错误由明确7、歧义10、越权9组成；明确 raw 实为33/40，UE只读 fallback 让C31/C32/C33的端到端正确，未修改它们的错误 raw。歧义/越权的 UE端到端为14/20，与raw1/20分别统计。

60项均一次真实生成、full_relevant、dropped=[]；输入3010–3152、输出48–105 token，全部原始字符串是可解析JSON，报告没有每项error。当前证据支持**模型生成的分支及语义字段错误是主失败层**，没有输入裁剪、输出触顶、JSON解析失败或两次实际结算失败的证据。不能从这一矩阵隔离断言4B容量、量化或贪心采样本身是首因；准确表述是冻结模型与当前System/Schema组合的语义生成质量不足。

已补水、散石、钓鱼的独立成功沿原报告保留，不替换60条中的失败或20边界统计。后续唯一地块 braces/TARGET_REQUIRED 已修复，旧CPU60也没有成为当前源码完整新验收结果。

## 四条明确端到端失败的第一错误

| 样例及冻结原话 | 原raw关键字段 | 实际reason /处理 | 第一失败层 |
| --- | --- | --- | --- |
| C14：帮我对付附近的威胁 | companion_order / hold，预期assist | reason空；等待确认的hold卡，矩阵未确认 | 模型把新协助指令映射成当前等待状态；上下文确有hold，因果锚定仅属推断 |
| C21：从营地仓库取一份药草到你背包 | retrieve / camp_to_player，预期fetch / camp_to_bag | UNRESOLVED_CONSTRAINT，refuse，无卡 | 模型运输终点映射错误；UE阻止向玩家错交货物 |
| C36：你有可靠记录说明之前实际交付过多少木材吗？没有记录就说不清楚 | inventory_report / wood /0，预期recall | AMBIGUOUS_REPORT，clarify，无卡 | 模型把历史查询映射为当前库存陈述；UE没有错误记录零库存 |
| C39：以后不准消耗石材，先给我规则卡核对 | rule_proposal / ban:stone，预期no:stone | UNRESOLVED_CONSTRAINT，refuse，无卡 | 模型混淆禁耗与禁采；原话一致性守卫阻止错误长期规则 |

这四条的原始全文见证据JSON。解析按字段名保持原值：[AgentContract.cpp:410](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:410)；HTTP回调将Content直接保存为LastStructuredResult：[LocalAISubsystem.cpp:512](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:512)。未发现Parser改变raw的quantity、source或intent。运输两端在[能力表249–252](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:249)已区分，System[418–422](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:418)已有分支、终点、ban/no与历史查询要求。这些事实不足以支持追加四条逐句prompt或用确定性重分类包装通过。

## 歧义/越权组

十条A原raw均选择写入能力；九条U错误中七条选择写入能力，U07/U08选择错误的recall/cancel中性能力。U06为唯一raw正确refuse。典型现象是台词承认负数、未知地点、缺目标或禁止规则，结构依然生成合法执行枚举；写入分支的unresolved在Schema中最多0项，合法值约束不能代替对原话的分支选择。[Schema](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:331)及[System](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:416)已逐项核对。

A01/A02/A04/A05/A06/A07/A08、U02/U03/U04/U05/U09/U10由UE安全阻止或澄清；它们的raw继续失败。A09的旧TARGET_REQUIRED通用拒绝已被当前Source最窄澄清分支修复，不提出重复补丁。A10使用PERSON_NOT_CONTACTED正确拒绝未经接触的人物，当前精确scope继续保留该行为。U07/U08是模型中性分支错误，未提出关键词路由补丁。

## 两项反馈缺口，先RED再最小生产修改

### A03：真实多个自有维修实例

冻结文本：`修好你自己的斧头`。原raw为repair/axe/1/one_owned/bag，unresolved=[]；实际两实例时reason=AMBIGUOUS_TARGET，applied_intent=refuse，台词“没有找到我自己持有的唯一目标，不能代用你的装备。”世界拒绝任意代选是正确边界；选择目标的交互反馈需要clarify。

[StageCandidate自动绑定](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentInteraction.cpp:134)只接受数量1；[实际PreviewGoal](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/Companion/HearthwardCompanionFixture.cpp:1439)在未给GUID且数量不为1时返回AMBIGUOUS_TARGET。该reason也用于契约quantity/mode/source错误，因此不能全局改成clarify。

生产候选 `a03-owned-repair-clarification-minimal-production-proposal.patch` 仅保存本次Contract Validate是否通过，在**契约已通过、repair、未绑定EquipmentId、实际弟弟同类自有实例>1、Preview返回AMBIGUOUS_TARGET**的交集中进入既有clarify/AddClarification。零实例继续refuse，合法唯一或明确GUID沿既有绑定，契约错误不进入此分支。

Native提案 `a03-owned-repair-native-red-proposal.patch` 复用既有 `Hearthward.Crafting057.WarehouseMaterialsPresentation` 的真实World、工作台、兄弟、背包和公开SetStructuredGoal。解析原CPU raw，保留冻结Repair.Original，实际授予两实例；检查真实Preview前置、clarify、turn=1及库存/营地不变。另验证错误quantity仍refuse、明确GUID和唯一实例仍成功、零实例仍refuse，最后还原公开库存快照。

**此公开入口会 canonical化输入。** [SetStructuredGoal:398](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentInteraction.cpp:398)将Input=GoalText，StageCandidate:133将Goal.Original=Input。赋值冻结Original无法改变这个生产契约。提案因此明确断言GetLastInput和WorkingGoal.Original等于真正canonical文本，不声称重放模型free_text或提升raw。真实冻结A03路线证据来自原CPU报告；Native只保护它暴露的同一多个实际实例反馈分支。未新增私有访问或测试桥API，未使用旧LocalAITests六字段协议。

### U01：已识别负数量被来源澄清覆盖

冻结文本：`新采负三份木材带回仓库`。原raw为collect/wood/3/additional_acquired/S1，unresolved=[]，台词仍写“数量逻辑异常”。[StageCandidate:193](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentInteraction.cpp:193)已用InvalidQuantity正则识别负数并保存未解条件；随后[Validate:400](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentContract.cpp:400)采集动词数量前缀不接受“负”，先返回UNRESOLVED_COLLECTION_SOURCE，交互[310](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardAgentInteraction.cpp:310)进入clarify问来源。原来已经安全无卡，具体分类及反馈优先级错误。

生产候选 `u01-negative-priority-minimal-production-proposal.patch` 在Validate的原话来源检查之前，复用Stage已有、完全相同的InvalidQuantity谓词，先返回既有UNRESOLVED_CONSTRAINT拒绝reason。只使用原有Writes范围，原草稿负数量文字继续保留；未添加额外数值领域、能力、Schema、数量改写或生成逻辑。

Native提案 `u01-negative-priority-native-red-proposal.patch` 在既有 `Hearthward.NPCAgent.OriginalCollectionVerbCompatibility` 增加原raw解析+原冻结文本的精确reason断言，并以同一正数提案及真正缺来源原话作对照。现有233行“请勿采两份”只TestFalse否定动词请求，未保护数值负数reason，新增断言保护不同实际回归。该单项不调用模型。Stage现有UNRESOLVED_CONSTRAINT分支拒绝语义由代码确认；本Native不声称直接重放私有Stage或OS。

## 验证范围与当前状态

四个.patch仅写 ownQA；`feedback-priority-proposal-static-check.json` 记录Python对当前Root文本的统一diff上下文与hunk数量核对。它不提供编译或运行信用。

1. Root先仅应用两份Native RED提案，分别执行以上两个现有单项过滤器；先确认前置均成功，再记录A03交互分类/澄清轮数和U01具体reason失败。若夹具前置失败，保留并修夹具，不补生产fallback。
2. 真实业务RED后，再应用对应两个最小生产候选；同两个过滤器GREEN后停止重复本地检查。A03明确quantity错误的控制用于防止把契约AMBIGUOUS_TARGET全局转成澄清。
3. 任何后续真实冻结A03/U01定向推理只统计自身路线；守卫修复不抵扣原raw1/20、34/60。完整同源码双后端矩阵仍需分别执行并保留门槛，三条额外自然能力成功继续独立记录。

主要框架结论与版本锁定文档一致：llama.cpp b10964的[GBNF说明](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md#json-schemas--gbnf)说明Schema约束输出格式，不会把Schema注入prompt。该资料支持区分结构合法与语义正确，不支持新框架或提示扫参。既有npc_line-first实验已经撤回，本轮不重复。
