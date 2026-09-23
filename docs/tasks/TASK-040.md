# TASK-040｜AI NPC vNext 上下文、认知与契约返工

状态：Blocked（UE 5.8.2 默认 Editor 构建失败；真实模型 e2e、相关 PIE、Issue / 独立 Reviewer 与 Owner 验收尚未闭合）。

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
- PR 创建与 main 合并需 Owner 明确授权；commit / push 按 Owner 的显式授权执行。

## 验证

PASS：

- repo validator：0 errors
- Python：31/31

FAIL：

- UE Editor Development build：UE 5.8.2 CL 56702186；Unity 中 `Json` 重定义，C2084/C2264。

NOT_RUN：

- native `Hearthward.*`：本轮构建失败，无法启动原生自动化测试。

BLOCKED / NOT_RUN：

- 当前源码真实 Qwen CTX-01～CTX-04 end-to-end
- TASK-040 baseline task-scope approval check（本任务不在基线 SHA 中，不能把当前 JSON 冒充基线审批）
- GitHub Issue / independent Reviewer
- current-source native/PIE/model retest after build repair
- PR / merge

完整证据见 [当前 clean-tree review](../qa/evidence/TASK-040/CLEAN_TREE_REVIEW.md) 与 [历史工作树验证记录](../qa/evidence/TASK-040/VALIDATION.md)。

## 权限

Owner：XLingyyy。

Reviewer：未分配。

用户已授权提交、推送当前任务分支；PR 创建与 main 合并仍未授权。
