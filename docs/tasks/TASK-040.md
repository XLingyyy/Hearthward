# TASK-040｜AI NPC vNext 上下文、认知与契约返工

状态：Blocked（实现与技术验证已完成；最终 main 同步、独立 Reviewer / Owner 验收和 main 合并尚未完成）。

> **Owner 指定的对外口径**：最终 AI NPC vNext 总验收、PR 与项目状态统一称为 **TASK-029 AI NPC 完整交付**。TASK-040 仅保留为内部返工编号；TASK-027～040 均继续保留用于历史追踪，不重编号。

基线：`codex/ai-npc-vnext-pr@04239f542ee99c1a735a380a66faa6ec99dc614b`；本轮修复从 `origin/codex/ai-npc-vnext-rework-01@97f8818` 继续。

## 目标

实施 AI-NPC-VNEXT-REWORK-01 的 R0～R6，同时保持既有 Agent 权限边界与 executor / Recovery / NavigationComponent / InitiativeQueue / LocalAIRuntime。

## 最终实现

### R0｜Unity build collision 与 helper 审计

- 修复 Unity 编译单元中重复匿名 helper `Json`。
- AgentInteraction / ContextProjection 的匿名 helper 改为模块前缀。
- 审计其它通用 helper，Workshop / Save 中的重复 `Counts` 也改为模块前缀。
- 默认 `HearthwardEditor Win64 Development` Unity build：PASS。

### R1｜Bounded ContextProjection

- 单次 UE authoritative snapshot。
- 最多三档：`full_relevant / compact_relevant / required_minimal`。
- 每档真实调用 llama.cpp `/apply-template` → `/tokenize`。
- 只有已计数且不超 3328 tokens 的同一 Body 才进入唯一一次 generation。
- 三档均超限：明确 `CONTEXT_OVERFLOW`，generation=0。
- full 不再无界倾倒所有 Belief；collect 请求不再因为“带回仓库”注入无关仓库库存。
- 必保原话、未解决限制与硬规则不作为普通 Top-K 被静默裁剪。

### R2｜Capability Contract / Prompt

- `Capabilities()` 保持能力枚举唯一来源。
- routine 与 hold/follow/assist 在 registry/schema/prompt/preflight 中一致。
- 通用 prompt 不再永久注入全部配方数量，避免把配方数量和玩家任务数量混淆。
- 中文数词是明确数量；“四份”“十份”不会被当作缺失数量。
- inventory report guardrail 支持显式中文数量，同时继续只写 cognition、不改真实仓库。
- 未知地点、玩家口述安全、多目标、负数/小数、规则冲突仍不能变成可成功确认的世界写任务。

### R3｜Belief Freshness

- `RecordedAt` = semantic change time。
- `LastEvidenceAt` = latest evidence time。
- freshness-only 更新不推进 semantic revision。
- 值/来源变化仍推进 revision。

### R4｜Episode Coverage

- `Complete / Truncated / Unknown`。
- coverage 与 completed/cancelled/in-progress 分离。
- event eviction 会永久降级为 Truncated。
- legacy history 迁移为 Unknown。
- metadata 仍有界。

### R5｜Save / Restore

- Save schema = 3。
- NPC cognition state version = 3。
- 新增真实 Schema 2 `.hws` 文件级 migration automation。
- Schema 2 → 3 保守补 `LastEvidenceAt = RecordedAt`，旧 episode coverage = Unknown。
- current-format 损坏字段继续严格拒绝。

## 最终技术验证（main 同步前）

- Editor Development Unity build：PASS
- full native `Hearthward.*`：**41/41 PASS**
- Schema 2 → 3 real-file migration：**1/1 PASS**
- real Qwen M01～M16 clean + pressure：**32 cases / 32 generation calls / 32/32 safety PASS**
- M01～M10 core raw model contract：**20/20 PASS**
- CTX-03：full 超预算后降到 `compact_relevant` 2832 tokens，generation=1，限制保留、无候选
- CTX-04：`required_minimal` 4020 tokens，`CONTEXT_OVERFLOW`，generation=0、无候选、无世界写、原话保留
- TASK-028 Executor PIE：**49/49 PASS**
- TASK-034 Initiative PIE：**16/16 PASS**
- TASK-036 Tactical Cooperation PIE：**16/16 PASS**
- TASK-038 Camp Routine PIE：**26/26 PASS**
- repo validator：0 errors（最终 main 同步前）
- Python tests：31/31（最终 main 同步前）
- Content/：零返工修改

完整证据见 `docs/qa/evidence/TASK-040/`。

## 对外 TASK-029 总交付包含的内部工作

TASK-027～040 作为 TASK-029 总交付的内部实现轨迹：

- authoritative perception / safety
- deterministic Goal → Plan → Action executor
- contextual suggestions
- hold / follow / assist / routine
- adaptive recovery / replanning
- typed Belief / Knowledge State
- event-driven Initiative
- grounded Episode Memory
- tactical cooperation
- coordination prior
- camp routine
- componentization
- bounded context/token budget
- real Qwen contract + guardrail
- Schema 2 → 3 migration

## 不可退让边界

- 不提高 token 上限。
- 不增加第二次 generation fallback。
- 不扩大事件缓冲绕过完整性问题。
- 不让 LLM 获得坐标、路径、具体敌人、伤害、库存结算或任意世界写权限。
- 不修改模型/GGUF。
- 不在 Agent 侧直接 merge。

## 剩余流程

1. 同步 `origin/main@73bb10e+`。
2. 解决 README / 文档冲突并保留 main 的资产更新。
3. 对最终集成候选再跑 validate/build/native smoke。
4. 提交、推送新修复分支。
5. 创建 PR；**不直接 merge**。
6. 独立 Reviewer / Owner 最终体验验收。
