# TASK-039｜AI NPC 组件化收口与深模块重构

状态：Blocked（本地实现与验证完成；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

在不改变 TASK-027→038 玩法语义、存档 schema、LLM contract 与数值参数的前提下，对三个热点类做结构收口：

- `UHearthwardLocalAISubsystem`
- `AHearthwardCompanionFixture`
- `UHearthwardGameplayComponent`

采用 deep-module seam，而不是简单拆 cpp 文件。

## 新 seam

### 1. `UHearthwardCompanionNavigationComponent`

从 Fixture 中拥有并隐藏：

- AAIController movement
- actor / location navigation target state
- NavMesh projection
- workbench approach point
- retry cooldown
- stop / arrival logic

Fixture 保留兼容 facade，executor 与新的 behavior module 共享同一导航实现。

### 2. `FHearthwardNPCInitiativeQueue`

从 LocalAISubsystem 中拥有并隐藏：

- pending queue
- dedupe history
- active initiative
- communication-range delivery
- TTL
- cooldown

Subsystem 只负责提供当前 interaction busy / player / companion context。

### 3. `HearthwardCompanionBehavior`

从 GameplayComponent 中收口：

- typed-task navigation arbitration
- camp routine orchestration
- combat observation construction
- Assist / Protect / Regroup application
- navigation / LOS / attack commitment

GameplayComponent 只构造 context、接收 result，并负责最终玩家侧 damage settlement/cooldown。

### 4. `FHearthwardLocalAIRuntime`

从 LocalAISubsystem 中拥有并隐藏：

- llama-server process lifecycle
- local port allocation
- backend / gpu layer configuration
- API key / base URL
- Windows Job Object orphan protection
- health polling
- loading timeout

LocalAISubsystem 保留 NPC dialogue / candidate / memory / context / request orchestration。

## 非目标

- 不改 Goal / Plan / Action 语义。
- 不改 Recovery / Belief / Episode / Initiative / Coordination / Routine 规则。
- 不改 gameplay tuning。
- 不改 SaveGame schema。
- 不把 Fixture 在本任务直接替换成正式 CompanionCharacter。
- 不顺手扩大 prompt token budget；真实模型当前 vNext context 超过既有 `max_input_tokens` 的问题单独记录。

## 验收

1. Editor Development build PASS。
2. repository Python tests 31/31。
3. repo validator 0 errors。
4. 全量原生 `Hearthward.` 39/39。
5. TASK-028 Executor PIE 49/49。
6. TASK-034 Initiative PIE 16/16。
7. TASK-036 Tactical PIE 16/16。
8. TASK-038 Routine PIE 26/26。
9. Local AI runtime 使用主 checkout bundle 可实际启动并达到 ready；后续请求按现有 token budget 被 `CONTEXT_OVERFLOW` 拒绝，证明 runtime lifecycle seam 正常且没有绕过既有输入预算。
