# 缺少自然目标：原生 RED → GREEN

2026-10-05；集成树 `.agent-local/task051`，基线 `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加本地未提交生产补丁。Owner／Reviewer XLingyyy，无 Issue。

CPU 完整矩阵 A09 的实际模型错误生成 `nature_care / harvest / known_target`。世界前置以 `TARGET_REQUIRED` 阻止执行，交互层却给通用设施拒绝，没有记录补充信息。这是独立的交互分支缺陷；本次不改变模型 raw、Schema、参数、冻结输入或评分。

现有 `Hearthward.Crafting057.WarehouseMaterialsPresentation` 内新增16行回归，使用真实 Standalone World、公开 `Companion.PreviewGoal` 和 `AI.SetStructuredGoal`。前置确认没有绑定目标、世界未暂停、原因确为 `TARGET_REQUIRED`；检查无候选、库存不变、WorkingGoal 保留，以及 clarify／澄清轮数。公开结构入口生成 canonical GoalText，因此该原生测试不宣称重放 A09 原话。

- RED：`run_native.py --filter Hearthward.Crafting057.WarehouseMaterialsPresentation --label missing-nature-target068-red --render`。1项失败，恰好2个业务错误：applied intent 实为 refuse；澄清轮数实为0。其余前置及材料真实结算检查通过。原始报告为 `missing-nature-target-native-red.json`。
- 修复：`HearthwardAgentInteraction.cpp` 中增加6行，仅对 nature_care／nature_collect 的 TARGET_REQUIRED 进入既有等待补充信息、clarify、AddClarification 分支。保留原因码及草稿；有目标的 NOT_READY／ALREADY_DONE／TARGET_UNAVAILABLE 与 escort 的 PERSON_NOT_CONTACTED 路径不变。
- Editor Development 构建通过。
- GREEN：同过滤器、`--label missing-nature-target068-green --render`。1/1通过，0错误，1项既有 EnhancedInput LocalPlayer 缺 PlayerInput 的夹具初始化警告。报告为 `missing-nature-target-native-green.json`。原生夹具不提供 OS 输入信用。

该 GREEN 证明缺少目标时的补充信息流程及无未确认物品变化；原始理解和 TASK-068 正式门槛仍未通过。
