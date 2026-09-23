# TASK-035｜行动结果驱动对话与 Episode Memory

状态：Blocked（用户已授权本地继续研发；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

把真实执行事件聚合成 command-scoped episode，让 NPC 回答“上次做了什么、为什么慢、有没有重规划、实际交付多少”时只引用真实 evidence。

```text
immutable-ish NPC Events
       ↓ derived projection
Command Episode
       ↓
recent_episodes in model context
       ↓
grounded recall / explanation
```

Episode 不是新的持久化事实源；保存的仍是 Events，Episode 每次即时重建。

## Episode 字段

- command id / item
- acquired / delivered
- crafted / repaired
- replan count
- completed / cancelled
- reasons
- last time
- evidence event ids

## 行为约束

- own_bag 不能反推出“做过什么”。
- past-action claims 只能来自 Episode evidence 或玩家明确记录。
- recall 会优先给出 command-scoped episode，而不是散列最后3条原子事件。
- filtered context 只暴露最近3个 grounded episodes。
- Save/load 不保存第二份摘要；读档后从持久化 Events 重建相同 episode。

## 验收

1. pure projection 聚合多个事件并按 command 分组。
2. Describe 输出实际取得/入库/重规划/原因/evidence。
3. Editor build PASS。
4. Native NPCAgent 全绿。
5. PIE 制造 source relocation → replan → collect2 complete。
6. recent episode 显示 acquired=2/delivered=2/replans=1/completed=true。
7. QueryRecentHistory 输出 SOURCE_POSITION_CHANGED 与 evidence。
8. Save/load 后 command/evidence/projection/grounded line 保持一致。
