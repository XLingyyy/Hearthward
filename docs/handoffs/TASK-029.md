# TASK-029 交接｜AI NPC 完整交付

## 当前口径

AI NPC vNext 最终统一使用 **TASK-029**。

早期研发的 027～040 编号只作为内部历史来源；其中历史 “AI TASK-028 executor” 已归档到 `docs/qa/evidence/TASK-029/internal-history/TASK-028-agent-executor/`，因为项目 main 当前 canonical TASK-028 已被用于 3D 资产任务。

当前候选分支：`codex/ai-npc-vnext-rework-01-fix`。

## 架构

```text
Player / Suggestions
        ↓
Local Qwen
        ↓
ContextProjection + Capability Contract
        ↓
Deterministic Guardrail
        ↓
Candidate + Confirm
        ↓
Goal → Plan → Action
        ↓
UE authoritative world systems
        ↓
Receipts / Events
        ↓
Belief / Episode / Coordination / Initiative
```

核心原则：LLM 做理解与表达；UE 做世界事实、安全、路径、战斗、结算和持久化权威。

## 已完成内容

- authoritative perception / safety
- typed Goal → Plan → Action executor
- contextual suggestion cache + stale revalidation
- hold / follow / assist / routine
- deterministic combat policy and tactical Assist/Protect/Regroup
- adaptive recovery
- typed Belief with provenance + freshness
- event-driven Initiative
- grounded Episode + coverage
- Coordination Prior
- camp autonomous Routine
- Navigation / Behavior / Initiative / Local AI Runtime componentization
- bounded ContextProjection with real template token count
- capability registry/prompt alignment
- Chinese numeric quantity handling
- Schema 2 → 3 real-file migration

## 本轮返工修复

1. 修复 Unity build 中匿名 helper `Json` collision，并审计/前缀化其它通用 helper。
2. 修复 CTX-02 压力下无关 Belief 与配方数量污染 prompt，导致“新采四份木材”被误判缺数量的问题。
3. 修复 `inventory_report` 只识别阿拉伯数字、无法接受“十份木材”的 guardrail 缺陷。
4. 验证 full → compact 降档时 unresolved 安全限制不会丢失。
5. 验证 required-minimal 真正超预算时明确失败且 generation=0。
6. 新增真实 Schema 2 `.hws` → Schema 3 disk migration automation。
7. 最终重跑 executor / Initiative / tactical / routine runtime PIE。

## 同步 main 前证据

- Editor Development default Unity build：PASS
- native：41/41 PASS
- Schema migration：1/1 PASS
- real Qwen matrix：32 cases / 32 generations / 32/32 safety PASS
- core raw M01～M10：20/20 PASS
- CTX-03：compact 2832 tokens / generation=1 / restriction preserved
- CTX-04：required-minimal 4020 tokens / generation=0 / no candidate / no world write
- executor PIE：49/49
- Initiative PIE：16/16
- Tactical PIE：16/16
- Routine PIE：26/26
- repo validator：0 errors
- Python：31/31

## 真实模型说明

M11/M12/M14/M16 的 raw JSON 在部分 clean/pressure run 中仍可能给出偏宽的 collect/repair intent；deterministic guardrail 均正确拒绝/澄清并阻止世界写。raw model 与 guardrail 成功分开记录，不混为一谈。

## Evidence

统一入口：[FINAL_ACCEPTANCE](../qa/evidence/TASK-029/FINAL_ACCEPTANCE.md)。

历史建议层证据仍在 `docs/qa/evidence/TASK-029/VALIDATION.md`。更早内部任务证据由 `internal-history/` 索引；上下文/Qwen/Schema 的原始大文件保留原来源路径并由 FINAL_ACCEPTANCE 链接。

## Release

当前正在同步最新 main（已从用户指定的 `73bb10e+` 前进到 `28e7c52`），保留 main 的自然地图/资产更新并合入 TASK-029 AI NPC 栈。

完成冲突解决后：

1. repo validator + Python
2. Editor build
3. full native
4. 必要 runtime smoke
5. commit merge result
6. push branch
7. create PR
8. **do not merge**
