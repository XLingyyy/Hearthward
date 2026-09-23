# TASK-029 AI NPC vNext 完整交付｜统一 PR 交接

日期：2026-09-23
工作分支：`codex/ai-npc-vnext-rework-01-fix`
目标：`main@73bb10e+`

> Owner 指定本次最终对外口径为 **TASK-029 AI NPC 完整交付**。仓库内部 TASK-027～040 继续作为设计、实现和证据分解，不重写历史编号。

## PR 建议标题

`feat: complete TASK-029 grounded adaptive AI NPC stack`

## 这次到底做了什么

从玩家自然语言到 UE 世界执行，形成完整的受控 AI NPC 链路：

```text
player text / explicit suggestions
        ↓
local Qwen structured proposal
        ↓
deterministic capability + guardrail validation
        ↓
candidate card + player confirmation
        ↓
authoritative UE Goal → Plan → Action
        ↓
navigation / combat / crafting / repair / settlement
        ↓
receipts + events
        ↓
belief / episode / coordination / initiative
        ↓
bounded context for the next interaction
```

### 内部 TASK-027～031：基础闭环

- authoritative perception / safety
- deterministic Goal → Plan → Action executor
- explicit contextual suggestions
- hold / follow / assist directives
- UI / navigation arbitration fixes

### TASK-032～039：Agent 能力扩展与组件化

- adaptive recovery / replanning
- typed Belief / Knowledge State
- event-driven Initiative
- grounded Episode Memory
- tactical cooperation: Assist / Protect / Regroup
- bounded Coordination Prior
- camp autonomous Routine
- Navigation / Initiative / Behavior / Local AI Runtime componentization

### TASK-040 内部返工：可信上下文与真实模型收口

- 修复 default Unity build helper collision
- single authoritative snapshot → full / compact / required-minimal projection
- real llama.cpp apply-template + tokenize budget checks
- one accepted body → at most one generation
- capability prompt registry alignment
- remove irrelevant belief / recipe quantity prompt pollution
- explicit Chinese quantity handling
- Belief semantic time vs `LastEvidenceAt`
- Episode Complete / Truncated / Unknown coverage
- Save Schema 2 → 3 real-file migration
- real Qwen M01～M16 clean/pressure validation

## 权限边界

AI/LLM **不直接决定**：

- world coordinates
- concrete enemy IDs
- pathfinding
- frame-by-frame combat
- hit/damage
- inventory settlement
- arbitrary world facts
- safety truth

这些继续由 UE authoritative systems 决定。

Belief / Episode / Coordination Prior / Suggestions 都是 cognition/read-model；不能成为第二套 world authority。

## 最终技术证据（main 同步前）

| 验证 | 结果 |
|---|---:|
| UE Editor Development default Unity build | PASS |
| full native `Hearthward.*` | **41/41 PASS** |
| real Schema 2 → 3 file migration | **1/1 PASS** |
| real Qwen M01～M16 clean + pressure | **32/32 safety PASS** |
| core M01～M10 raw model contract | **20/20 PASS** |
| total matrix generations | **32 / 32 cases** |
| normal matrix context overflow | **0** |
| CTX-03 degradation | PASS — compact 2832 tokens, one generation, restrictions preserved |
| CTX-04 mandatory overflow | PASS — minimal 4020 tokens, zero generation |
| TASK-028 Executor PIE | **49/49 PASS** |
| TASK-034 Initiative PIE | **16/16 PASS** |
| TASK-036 Tactical Cooperation PIE | **16/16 PASS** |
| TASK-038 Camp Routine PIE | **26/26 PASS** |
| repository Python tests | **31/31 PASS** before final main sync |
| repo validator | **0 errors** before final main sync |

Evidence: `docs/qa/evidence/TASK-040/`.

## Real Qwen result interpretation

M01～M10 core semantic raw contract is 20/20 across clean + pressure.

M11/M12/M14/M16 can still produce raw collect/repair proposals instead of an ideal immediate refusal/clarification. The deterministic validation layer correctly returns `AMBIGUOUS_TARGET / UNRESOLVED_CONSTRAINT / POLICY_CONFLICT` and prevents prohibited execution. Raw JSON, guardrail result and world result are all retained separately in `model-results.json[l]`; guardrail success is not reported as raw model success.

## Save compatibility

A new native automation writes a real Schema 2 `.hws` file, loads it through the production read/migration path, verifies conservative migration fields, writes Schema 3, reloads and strictly validates it.

## Release process

Before opening the PR:

1. sync current `origin/main@73bb10e+`
2. resolve README/docs conflict while retaining main asset changes
3. rerun final integrated validate/build/native smoke
4. update candidate SHA in this handoff

Then:

- push `codex/ai-npc-vnext-rework-01-fix`
- create PR to `main`
- request review
- **do not merge**

Independent Reviewer / Owner gameplay acceptance remains a separate final step.
