# 固定目录压缩最小方案

根已登记并授权下列两个路径。生产改动已在隔离树实施，增量补丁为 `production-compression.patch`；未执行 UE / 构建 / Git。

独立源码/当前根 data 核对结果在 `source-contract-audit.json`：24 能力、158 唯一 item、507 intent/item 关系、16 分组，缺失/新增关系均为 0；注册表除 description 外逐字保持，Schema/Parse/Validate 和 Subsystem 的非 System 内容逐字保持。此次属于静态核对，真实 Native token 和双后端理解/执行仍待根执行。

## 真实 RED 和候选预算

根实际 UE C01：`CONTEXT_OVERFLOW`，`input_tokens=5691`，`tier=required_minimal`，`generation_calls=0`。原始记录位于根 `Saved/Task068/cpu/cases.jsonl`；一个样本的失败报告不代表完整 60 样本矩阵。

固定开销来自 `AI/HearthwardAgentContract.cpp::Describe()`：31 个普通运输物品的 id/中文名重复五次；77 个物品重复两次；58 制作配方、53 营地配方、38 修理项再次展开相同 id/中文名。`AI/HearthwardLocalAISubsystem.cpp` 的 System 又重复了目录中的权限、指令、认知和候选说明。

最终草案 `compact-scoped-system-proposal.txt` 通过真实锁定 CPU `/apply-template`→`/tokenize` 测得 **2570 tokens**，0 次生成，server 已退出。它包括全部 24 能力、全部 id/中文别名、完整 intent/item 允许关系，以及逐能力 quantity 上限、mode、source。world context 全部省略，输入使用相同四份木材诊断句。

按当前 RED 的 world-context 差值 229 推算，实际最小层约 2799，距 3328 约 529。这个差值是估算，真实 UE token/生成/结果仍为正式验收。不得把 sourceprobe 当 Native Token PASS。

## 只改两处生产路径

### `Source/Hearthward/AI/HearthwardAgentContract.cpp`

1. `Capabilities()` 仅缩短 description 字符串。同义说明集中在共同 System；每项特有的真实世界条件继续保留。`Items/Sources/QuantityMode/MaxQuantity/Constraints/Writes` 及全部能力 ID 不改，最后四个通用能力原循环保持。
2. `Describe()` 改为局部的完整目录序列化：从所有 `C.Items` 收集唯一 id；对每个 id 从同一注册表取得允许的全部 intent。按相同 intent 集合分组，每个 id=中文只打印一次。左侧明确列出该组的所有允许 intent，右侧为该组全部 item，不按玩家文本选择、删除或裁剪。
3. 每个能力行按共同表头输出 `能力|语义|quantity上限|mode|source`。所有数值、mode 和 source 直接取注册表；仅减少反复打印列名。
4. 不新增公共接口、业务抽象或依赖。分组只存在于原 Describe 方法内；Schema/Parse/Validate/确认/执行逻辑均不修改。

### `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp`

只替换现有 System 表达式：合并同义固定规则，再追加完整 Describe。旧 `CompanionOrderPrompt()` 保持可调用和原生测试覆盖，其完整语义和参数已出现在新目录/共同规则中，System 不再追加第二份。

原安全约束全部保留：低权限输入、单候选和明确确认、中文整数与新增量、负数/小数/超限、缺信息不猜测、全部原话/澄清历史/限制、未知地点和口述安全、坐标/逐帧战术/多目标/未注册能力、bag/camp授权、唯一自持维修实例、长期 ban/no/max/source、allow/once 适用范围、库存问句和未核实 report、episode coverage、插入查询/闲聊不执行旧目标、四种高层指令、角色称谓与回复长度、未知事实和候选未完成表达。

## 为何保留模型可见对应关系

[llama.cpp b10964 官方 grammar 文档](https://github.com/ggml-org/llama.cpp/blob/b10964/grammars/README.md)说明 JSON Schema 不进入模型提示词，只约束输出。因此，单独给全局别名表并声称“由 Schema 决定 item”不足以表达意图和物品的语义对应。最终方案打印全部对应关系；先前 2504 token 的非 scoped 草案仅为探索记录，不能用于生产。

## 必须执行的定向验证

1. 根构建后分别运行实际 CPU/Vulkan C01，确认真实预算≤3328、生成次数1、raw collect/wood/quantity1 等于原期望、确认前没有物资或任务写入，确认后真实采集/入库成立。
2. 两后端各运行固定 60 表达和 30 执行/20 边界，分别记录原始理解、候选纠正、实际库存/GUID/指令状态、延迟、tier/dropped；期望保持冻结，不能按输出调整。
3. 直接受影响原生能力/限制/投影测试验证完整目录、Schema 和原安全逻辑。完整世界故事、新自然能力模型执行、双营地连续改口/旧约束和 Owner 角色判断继续单列；此补丁解决固定预算 RED，不提前关闭全部 TASK-068。
