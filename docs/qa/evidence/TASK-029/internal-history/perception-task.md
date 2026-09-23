# TASK-027｜AI NPC世界感知、安全判定与权威观察快照

状态：Blocked（用户于2026-09-22授权本地研发；正式Issue/独立Reviewer待GitHub通路核验）。本单与TASK-026地图制作并行，不修改Content、地图或角色资产。

## 目标

把TASK-025已经可靠的“自然语言 → 规范目标 → UE真实执行”继续推进为真正的Agent闭环第一层：

```text
World → Perception Snapshot → Safety Policy → Goal/LLM Context → Executor → World
```

本单只做 **Perception + Safety**。Planner、通用Action Executor、主动交流、战斗协作与生活调度后续拆单。

## 核心约束

1. **UE事实高于语言。** 玩家说“那里安全”、模型说“可以去”都不能改变安全结论。
2. **同一快照服务推理与执行。** LLM看到的世界状态与执行层复核来自同一观察模块，避免两套事实口径。
3. **确认时重验，执行中继续重验。** 候选形成后世界变化，旧候选不能凭旧安全状态开始新的采集/取料/制作/维修结算；已取得物资继续沿用既有安全返营或原地保留语义。
4. **按能力判断所需事实。** 采集需要采集点+安全来源+营地；bag制作/维修不应因为采集点失效而被错误拒绝。
5. **不假装完成开放世界感知。** 现有安全点仍是PROTOTYPE_ONLY来源之一；本单建立可替换的深模块，不把夹具升级成正式玩法规则。

## 计划接口

新增 `HearthwardNPCPerception` 深模块：

- `Capture(companion)`：从UE真实世界读取观察快照。
- `Evaluate(observation, goal)`：纯函数安全判定，返回 Allowed / Unsafe / Unavailable + reason。
- 调用者无需理解战斗、暂停、来源、营地和目标类型的组合逻辑。

观察包含：世界/暂停、战斗状态、采集点存在与已知安全证据、营地存在、导航是否重建、与采集点/营地距离、当前伙伴阶段。

## 验收

- 采集：安全点和营地存在时允许；战斗、暂停、来源丢失或安全证据撤销时拒绝/阻塞。
- 制作/维修：使用bag材料时不依赖采集点；当前 craft 产物按既有执行语义需要返营入库，因此仍要求营地有效；repair 仅在明确使用 camp 材料时要求营地。
- LLM filtered context输出权威观察与当前安全判断。
- `NPCAgentTests`覆盖纯安全决策，不依赖模型。
- 仓库自检、脚本测试及能运行的UE自动化均记录真实结果。

## 非目标

不接EQS/AI Perception视觉系统，不实现敌人逐个威胁评分，不做Planner/Behavior Tree重构，不改地图与角色资产，不关闭R18/R20/R21。
