# TASK-029 AI NPC 完整交付｜PR 交接

日期：2026-09-23
候选分支：`codex/ai-npc-vnext-rework-01-fix`
最终候选：`037628f`
目标：`main`
PR：[#34](https://github.com/XLingyyy/Hearthward/pull/34)

## 统一口径

本次 AI NPC vNext 最终只使用 **TASK-029 AI NPC 完整交付** 作为对外任务和验收口径。

早期研发过程中出现过其它内部编号；它们只作为历史 evidence 来源保留，不再作为 PR 的正式任务拆分。项目当前 canonical TASK-028 是 3D 资产导入任务，AI executor 的旧内部 028 已归档到 `docs/qa/evidence/TASK-029/internal-history/`。

## 交付内容

TASK-029 形成从自然语言理解到 UE 权威执行的完整链路：

```text
player text / explicit suggestions
        ↓
local Qwen structured proposal
        ↓
bounded context + capability contract
        ↓
deterministic guardrail / preflight
        ↓
candidate card + player confirmation
        ↓
UE Goal → Plan → Action
        ↓
navigation / combat / collect / craft / repair / settlement
        ↓
receipts + events
        ↓
belief / episode / coordination / initiative
```

包含：

- authoritative perception / safety
- deterministic typed executor
- contextual suggestions + stale revalidation
- hold / follow / assist / routine
- adaptive recovery / replanning
- typed Belief / Knowledge State
- event-driven Initiative
- grounded Episode Memory + coverage
- tactical Assist / Protect / Regroup
- Coordination Prior
- camp autonomous Routine
- Navigation / Behavior / Initiative / Local AI Runtime componentization
- full / compact / required-minimal bounded context
- real llama.cpp template/token counting
- Chinese quantity handling
- Schema 2 → 3 real-file migration

## 权限边界

LLM 不直接决定世界坐标、具体敌人、路径、命中/伤害、库存结算、安全真值或任意世界写。所有 side effect 继续经过 UE authoritative validation / executor。

## 最终验证

同步最新 main 前：

- real Qwen M01～M16 clean + pressure：32/32 safety PASS
- M01～M10 core raw model contract：20/20 PASS
- CTX-03：full → compact_relevant，2832 tokens，1 generation，限制保留
- CTX-04：required_minimal 4020 tokens，0 generation，无候选/世界写
- Schema 2 → 3 real-file migration：1/1 PASS
- executor / Initiative / tactical / Routine runtime：49/49、16/16、16/16、26/26 PASS

同步 `origin/main@28e7c52` 后：

- repository validator：0 errors
- Python repository tests：31/31 PASS
- clean/rebuilt UE 5.8.2 Editor Development：PASS
- full native `Hearthward.*`：41/41 PASS，0 warnings/failures/not-run
- TASK-029 explicit-prototype runtime smoke：23/23 PASS

证据入口：

- `docs/qa/evidence/TASK-029/FINAL_ACCEPTANCE.md`
- `docs/qa/evidence/TASK-029/POST_MERGE_VALIDATION.md`
- `docs/handoffs/TASK-029.md`

## Review note

M11/M12/M14/M16 的部分 raw Qwen 输出仍可能比理想的立即拒绝/澄清更宽，但 deterministic guardrail 均正确阻止禁止执行。raw model、guardrail 与最终 world state 分开报告，不把 guardrail 成功写成 raw model 成功。

## Release state

PR #34 已创建并保持 open。当前 GitHub 显示 PR mergeable；repo-policy CI 已启动。后续等待独立 Reviewer / Owner 验收。

**Agent 不直接 merge。**
