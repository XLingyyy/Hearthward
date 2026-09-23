# TASK-039 交接

## 基线

- AI NPC vNext integration：`e3960f6`
- 分支：`codex/TASK-039-ai-npc-componentization`

## 收口结果

这次不是按文件大小机械拆分，而是增加四个 deep-module seam：

1. `UHearthwardCompanionNavigationComponent`
   - 拥有 controller/NavMesh/target/retry/workbench approach。
   - Fixture 不再持有 NavigationTarget / NavigationLocation / RetryAt 等内部状态。
   - Executor、Routine、Combat 复用同一移动实现。

2. `FHearthwardNPCInitiativeQueue`
   - 拥有 queue/history/active/TTL/cooldown/range-delivery。
   - LocalAISubsystem 不再自己编排这些运行时字段。

3. `HearthwardCompanionBehavior`
   - 统一应用 typed-task arbitration、Routine 与 Combat policy。
   - GameplayComponent 的 TickCompanion 收敛为 context → module → result → final damage settlement。

4. `FHearthwardLocalAIRuntime`
   - 拥有 llama.cpp process、backend、port、API key、Windows Job Object、health polling、load timeout。
   - LocalAISubsystem 重新聚焦 NPC dialogue / context / model request orchestration。

## 热点变化

重构前：

- LocalAISubsystem.cpp：789 行
- CompanionFixture.cpp：872 行
- GameplayComponent.cpp：725 行

重构后：

- LocalAISubsystem.cpp：679 行
- CompanionFixture.cpp：815 行
- GameplayComponent.cpp：625 行

热点类合计减少 267 行职责代码；新增逻辑转移到可单独理解和复用的深模块，而不是删除行为。

## 验证

- Python repository tests：31/31 PASS。
- repo validator：0 errors。
- Editor Development build：PASS。
- 全量 native `Hearthward.`：39/39 PASS。
- TASK-028 Executor PIE：49/49 PASS。
- TASK-034 Initiative PIE：16/16 PASS。
- TASK-036 Tactical Cooperation PIE：16/16 PASS。
- TASK-038 Camp Routine PIE：26/26 PASS。

真实 Qwen runtime 定向验证：

- 使用 `D:\Dev\Hearthward\Runtime\LocalAI` 作为 worktree bundle override。
- Runtime 实际启动 llama-server，并达到 ready。
- 随后现有 prompt/context 被既有 `max_input_tokens` 判定为 `CONTEXT_OVERFLOW`。
- TASK-039 没有修改 BuildFilteredContext、system prompt 内容或 token budget；该问题不在本任务中擅自通过放宽预算解决，应作为后续 prompt/context budget 优化单独处理。

## 后续 seam

下一轮如果继续 productionization，优先：

- Fixture → 正式 CompanionCharacter / TaskExecutor module。
- LocalAISubsystem 的 filtered-context construction 再抽成 ContextProjection module。
- Agent Inspector 直接读取这些新 seam，而不是读取三个大类的内部字段。

## 远端

未 push / 未 merge。
