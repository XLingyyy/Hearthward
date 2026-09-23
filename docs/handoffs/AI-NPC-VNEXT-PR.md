# AI NPC vNext 统一 PR 交接

日期：2026-09-23
分支：`codex/ai-npc-vnext-rework-01`
目标 main：`e729349`；共同基线：`4114556`
当前受测源码 HEAD：`6d1ca5e`

## PR 建议标题

`feat: integrate grounded adaptive AI NPC stack`

## 范围

本分支统一整理 TASK-027 → TASK-038，不修改 TASK-026 World Partition / Content 资产。

核心链路：

```text
Player language / explicit controls
        ↓
typed Goal / companion directive
        ↓
authoritative perception + safety
        ↓
deterministic plan / combat / routine policy
        ↓
typed actions + UE navigation + real settlement
        ↓
receipts / events
        ↓
beliefs + grounded episodes + coordination prior
        ↓
contextual suggestions + event-driven initiative
```

### TASK-027 → 031：现有 AI NPC 基线

- authoritative perception / safety
- typed Goal → Plan → Action executor
- explicit contextual suggestions
- hold / follow / assist deterministic combat policy
- UE 用户复测修正：typed task 导航仲裁、对话工具栏

### TASK-032：Adaptive Replanning / Recovery

- action failure 先经过 deterministic Recovery Policy
- Source 在采集中移动可自动 rewind 到 `MoveTo(Source)`
- transient route failure 有界 retry / rewind
- 真实 command cargo 始终优先返营保货
- hard block 不凭空生成替代资源或权限

### TASK-033：Belief / Knowledge State

- World Truth 与 NPC Belief 分离
- `firsthand / player_report / receipt` provenance
- 离营后不偷看真实仓库
- 回营后 firsthand 可纠正玩家报告
- World Truth 与 stale belief 可在同一 SaveGame 边界独立恢复

### TASK-034：Event-driven Initiative

- completed / replanned / blocked / belief correction 可主动提醒
- 无 LLM heartbeat
- 超出交流范围时排队，靠近后再显示
- candidate / clarification / active inference 不被主动台词抢占

### TASK-035：Grounded Episode Memory

- 从持久化 Events 即时派生 command-scoped Episode
- 聚合 acquired / delivered / craft / repair / replans / reasons / evidence
- past-action recall 只引用 episode evidence
- 不持久化第二份会漂移的摘要

### TASK-036：Tactical Cooperation

- healthy → Assist
- low health + one close threat → Protect
- low health + multiple close threats → Regroup
- player down → Regroup
- enemy target / LOS / navigation / cooldown / damage 始终 UE-authoritative

### TASK-037：Coordination Prior

- 从真实成功的 hold / follow / assist 指令事件学习 bounded rolling prior
- stable prior 只影响 suggestions / model context
- 不自动覆盖当前玩家命令
- save/load 从 events 重建

### TASK-038：Camp Routine

- 无 typed task / 显式控制 / combat / downed 时，低权限营地自由活动
- rest / patrol / check_camp / return_camp
- 使用真实 UE NavMesh / AAIController，不 teleport
- 不生产物资、不接任务、不调用 LLM
- Z/X/C 显式关闭；confirmed `routine` 可重新授权
- typed task 暂停 routine，但不撤销授权；任务结束后恢复

## 关键验证

以下各任务计数来自先前的任务工作树验证，不能替代本分支当前受测提交的 clean-tree 结果；当前结果见 [TASK-040 clean-tree review](../qa/evidence/TASK-040/CLEAN_TREE_REVIEW.md)。

| 验证 | 结果 |
|---|---:|
| repository Python tests | 31/31 PASS |
| repo validator | 0 errors |
| UE Editor Development build | PASS |
| full native `Hearthward.` | 39/39 PASS |
| TASK-027 Safety PIE | 27/27 PASS |
| TASK-028 Executor PIE | 49/49 PASS |
| TASK-029 Modern / Legacy Suggestions | 40/40 + 8/8 PASS |
| TASK-030 Combat PIE | 29/29 PASS |
| TASK-030 real Qwen directive | 14/14 PASS |
| user retest / exact collect phrase | 11/11 + 11/11 PASS |
| TASK-032 Adaptive Recovery PIE | 11/11 PASS |
| TASK-033 Belief PIE | 25/25 PASS |
| TASK-034 Initiative PIE | 16/16 PASS |
| TASK-035 Episode PIE | 21/21 PASS |
| TASK-036 Tactical Cooperation PIE | 16/16 PASS |
| TASK-037 Coordination Prior PIE | 28/28 PASS |
| TASK-038 Camp Routine PIE | 26/26 PASS |

TASK-038 完成后再次重跑 TASK-028 executor：49/49 PASS。

## 重要边界

- LLM 不选择世界坐标、具体敌人、路径、逐帧动作、命中、伤害或库存结算。
- 玩家/模型文本不能制造世界事实、安全事实、资源、权限或任务完成。
- Belief / Coordination Prior / Episode 都是 cognition/read-model，不是 world authority。
- Routine 不生产资源。
- 本 PR 不修改 TASK-026 Content 资产。
- 不在 Agent 侧直接 merge。

## 远端状态

当前远端分支 `codex/ai-npc-vnext-rework-01` 已存在。本机 GitHub CLI 返回 HTTP 401，无法查询 PR 和 Reviewer 实时状态；TASK-040 记录显示当前未登记 Issue/Reviewer，也未创建 PR。候选分支当前构建未通过，见上方 clean-tree review。

修复当前 Unity 编译错误、同步最新 main 并完成独立评审后，可按 Owner 授权创建 PR：

```bash
git push -u origin codex/ai-npc-vnext-rework-01
gh pr create --base main --head codex/ai-npc-vnext-rework-01 \
  --title "feat: integrate grounded adaptive AI NPC stack" \
  --body-file docs/handoffs/AI-NPC-VNEXT-PR.md
```

PR 与合并需按项目流程另行授权；当前分支构建失败，暂不创建 PR。
