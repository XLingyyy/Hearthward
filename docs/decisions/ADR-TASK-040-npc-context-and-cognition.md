# ADR-TASK-040｜AI NPC vNext 上下文预算、认知时效与 Episode 完整性

状态：Accepted for local implementation / remote review blocked

日期：2026-09-23

基线：`codex/ai-npc-vnext-pr@04239f542ee99c1a735a380a66faa6ec99dc614b`（包含 TASK-032～039）

## 背景

TASK-039 组件化后，AI NPC 已把导航、主动消息队列、伙伴行为编排与本地推理 Runtime 从热点类中拆出，但真实模型链仍暴露出四类剩余问题：

1. filtered context、system prompt、schema 与历史澄清共同进入固定 3328 input-token 上限，现有链在真实 llama.cpp template/tokenize 后可能直接 `CONTEXT_OVERFLOW`。
2. `companion_order` capability registry 已包含 `routine`，system prompt 仍维护独立的 hold/follow/assist 列表，出现契约漂移。
3. Belief 的 `RecordedAt` 同时承担“语义变化时间”和“最近证据时间”；同值、同来源的新证据被当作完全无变化丢弃。
4. Episode 只从当前 128 条 Event ring 派生；事件被淘汰后无法区分“完整历史”与“只剩一部分历史”。

这些问题不能通过提高 token 上限、无限增大事件缓冲、重复模型生成或直接把 UE 世界真值暴露给模型解决。

## 决策

### 1. 固定预算不变，增加三档纯 ContextProjection

保留：

- `max_input_tokens = 3328`
- `max_tokens = 256`
- 每次玩家交流最多一次 `/v1/chat/completions`

新增 `HearthwardNPCContextProjection` 深模块。UE 先一次性捕获可信快照，再由纯函数从同一快照投影：

1. `full_relevant`
2. `compact_relevant`
3. `required_minimal`

每一档都先用同一个 llama.cpp Runtime 的 `/apply-template` 和 `/tokenize` 计算真实 template token 数。只有通过预算的那一个 **同一 request Body** 才进入 generation。若三档都超过预算，返回 `CONTEXT_OVERFLOW`，generation 次数为 0。

降档只能删除低优先级、无关或可压缩内容。已确认硬规则、当前玩家原话、尚未解决限制以及执行所需能力边界不可静默删除。

### 2. Capability registry 是能力枚举的唯一事实来源

`HearthwardAgent::Capabilities()` 继续负责能力 ID、item、quantity mode、source 与 constraints。

新增由 registry 派生的 `CompanionOrderPrompt()`。system prompt 不再单独维护 hold/follow/assist/routine 枚举。Schema 仍从同一 registry 生成；测试逐分支比较 registry 与 schema，并对所有已注册 companion directive 走同一 Validate/preflight。

`inventory` 与 `inventory_report` 保持两个不同契约：

- `inventory`：只读已有 belief。
- `inventory_report`：玩家明确报告精确数量，只更新 NPC belief，不写真实仓库。

### 3. Belief 分离语义变化时间与最近证据时间

`RecordedAt` 定义为最后一次 value/source 语义变化时间。

新增 `LastEvidenceAt`：当前 value/source 最近一次有效证据时间。

同 value、同 source、同 campaign 的新证据：

- 只刷新 `LastEvidenceAt`
- 不增加 `Memory.Revision`
- 不改变 `RecordedAt`
- 因而不会仅因为“重新看到了相同事实”让候选任务卡失效

来源或值发生变化仍是语义变化，继续推进 revision。

### 4. Episode coverage 独立于任务终态

新增：

- `Unknown`
- `Complete`
- `Truncated`

coverage 与 `Completed/Cancelled` 分离。

新接受的命令从 authoritative `SubmitGoal -> Accepted` 路径登记为 `Complete`。如果属于该 command 的事件被 128 条 ring 淘汰，coverage 永久降为 `Truncated`；后续收到 `completed` 也不能恢复为 `Complete`。

活动 command 即使所有早期事件都被挤出，coverage metadata 仍保留。非活动 command 在不再拥有 buffered event 后可被清理，因此 metadata 仍是有界的。

旧格式没有 coverage 证据，迁移为 `Unknown`，不能推断成 `Complete`。

### 5. Save schema 显式升级

当前 Save schema / NPC cognition state 升级到 3。

Schema 2 是明确的 pre-TASK-040 vNext 格式：

- `LastEvidenceAt <- RecordedAt`
- episode coverage 迁移为 `Unknown`
- 活动 command 以 `Unknown + Active` 迁移

当前 Schema 3 中损坏的新字段必须直接拒绝，不能因为“字段看起来像旧档默认值”而自动迁移。迁移和严格验证都发生在 Restore 修改世界状态之前。

## 权限与安全边界

本 ADR 不改变：

- World Truth、库存结算、安全、路径、目标选择、攻击、伤害的 UE 权威边界
- executor / Recovery / NavigationComponent / InitiativeQueue / LocalAIRuntime 的职责
- 模型、GGUF、llama.cpp 版本
- input/output token 上限
- Content、地图、输入映射或正式 CompanionCharacter

LLM 仍不能写世界真值、生成坐标、指定具体敌人或直接执行任务。

## 验证

当前源码验证：

- repository validator：PASS，0 errors
- repository Python：31/31 PASS
- UE Editor Development build：PASS（本机实际 UE 5.8.2；仓库锁定版本仍是 5.8.1）
- native `Hearthward.*`：40/40 PASS
- 关键新回归：
  - `Hearthward.NPCAgent.CapabilitiesAndLimits`
  - `Hearthward.NPCAgent.BeliefStateProvenance`
  - `Hearthward.NPCAgent.GroundedEpisodeProjection`
  - `Hearthward.NPCAgent.BoundedContextProjection`
  - `Hearthward.Save.FileIntegrityAndSnapshot`
  - `Hearthward.Save.NPCMemoryCompatibility`

当前源码真实 Qwen end-to-end：NOT_RUN / BLOCKED。模型 bundle 与 GGUF hash 已确认可用，但当前工具环境没有项目文档要求的 GameFactory UEClient；命令行 game instance 的 remote Python 能发现进程，却无法取得可用 Game WorldContext 来触发项目的 world-scoped debug command。因此 TASK-039 的历史 `CONTEXT_OVERFLOW` 证据不计作 TASK-040 当前源码结果。

## 后果

优点：

- token budget 从“超限即失败”变成确定性、可诊断、最多三档的 bounded projection。
- capability prompt/schema/preflight 不再维护彼此独立的枚举。
- Belief freshness 不再制造 semantic revision churn。
- Episode 对“完成”和“历史是否完整”有独立语义。
- 旧档迁移不会伪造新认知字段。

代价：

- Save schema 增加一次显式迁移。
- Local AI 请求前多进行最多三轮 template/tokenize 计数，但仍只允许一次 generation。
- 当前仍需要可操控 Game WorldContext 的 UEClient/PIE 验证入口，才能完成真实模型 CTX-01～CTX-04 证据。
