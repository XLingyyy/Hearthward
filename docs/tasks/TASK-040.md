# TASK-040｜AI NPC vNext 上下文、认知与契约返工

状态：Blocked（本地实现已完成；Issue / 独立 Reviewer、锁定工具链与真实模型当前源码 e2e 尚未闭合）。

基线：`codex/ai-npc-vnext-pr@04239f542ee99c1a735a380a66faa6ec99dc614b`。该基线已包含 TASK-032～039，本任务不从 TASK-029 返工。

## 目标

实施 AI-NPC-VNEXT-REWORK-01 的 R0～R6，同时保持既有 Agent 权限边界与 executor / Recovery / NavigationComponent / InitiativeQueue / LocalAIRuntime。

## 已完成实现

### R1｜Bounded ContextProjection

- 新增纯 `HearthwardNPCContextProjection`。
- 单次 UE snapshot，最多三档 `full_relevant / compact_relevant / required_minimal`。
- 每档实际调用 llama.cpp `/apply-template` 与 `/tokenize`。
- 通过预算的同一 Body 才能 generation；三档均超限则零 generation。
- `max_input_tokens=3328` 与输出 256 不变。
- 硬规则、当前玩家原话与 unresolved 约束不会作为普通 Top-K 被删除。

### R2｜Capability Contract

- `Capabilities()` 是能力枚举唯一事实来源。
- companion-order prompt 从 registry 派生。
- routine 不再与 prompt 漂移。
- schema / prompt / Validate 由测试结构化对齐。
- inventory 与 inventory_report 继续严格区分。

### R3｜Belief Freshness

- `RecordedAt` = semantic change time。
- `LastEvidenceAt` = latest evidence time。
- 同值同来源证据刷新不推进 semantic revision。
- 值/来源变化仍推进 revision。

### R4｜Episode Coverage

- 新增 `Complete / Truncated / Unknown`。
- coverage 与 completed/cancelled 分离。
- 新接受 command 才能从 Complete 开始。
- event eviction 会永久降级为 Truncated。
- legacy history 迁移为 Unknown。
- metadata 仍按活动/当前 buffered command 有界。

### R5｜Save / Restore

- Save schema = 3。
- NPC cognition state version = 3。
- Schema 2 显式迁移新字段。
- current-format 损坏字段拒绝，不静默 legacy-repair。
- Restore 前先 migrate + validate。

## 不可退让边界

- 不提高 token 上限。
- 不增加第二次 generation fallback。
- 不扩大事件缓冲绕过完整性问题。
- 不让 LLM 获得坐标、路径、具体敌人、伤害、库存结算或任意世界写权限。
- 不修改 `Content/`、模型、GGUF、输入映射或地图。
- 不重写 executor / Recovery / Navigation / Initiative / Runtime。
- 不授权 commit / push / PR / merge。

## 验证

PASS：

- repo validator：0 errors
- Python：31/31
- UE Editor Development build：PASS（实际机器 UE 5.8.2）
- native `Hearthward.*`：40/40

BLOCKED / NOT_RUN：

- 仓库锁定 UE 5.8.1 当前机器复验
- 当前源码真实 Qwen CTX-01～CTX-04 end-to-end
- TASK-040 baseline task-scope approval check（本任务不在基线 SHA 中，不能把当前 JSON 冒充基线审批）
- GitHub Issue / independent Reviewer
- commit / push / PR / merge

完整证据见 `docs/qa/evidence/TASK-040/VALIDATION.md`。

## 权限

Owner：XLingyyy。

Reviewer：未分配。

远端写入与合并均未授权。
