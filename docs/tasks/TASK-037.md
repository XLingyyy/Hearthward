# TASK-037｜长期协作习惯 / Coordination Prior

状态：Blocked（本地功能与验证完成；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

让 NPC 从真实、已确认的伙伴协作指令中学习“近期协作习惯”，用于建议与对话上下文，而不是自动替玩家执行。

```text
confirmed hold/follow/assist
        ↓ directive events
recent rolling window
        ↓
CoordinationProfile
        ↓
suggestion ranking / model context
        ✕
no automatic world action
```

## 学习规则

- 最近最多 8 条 directive event。
- 至少 3 个样本。
- 主导指令至少出现 3 次。
- 主导比例 >= 67%。
- 领先第二名至少 2 次。
- 不满足则 profile 为 unstable，不输出 preferred directive。

## 数据来源

- Z/X/C 直接控制成功后：`reason=direct_control`
- 自然语言/结构化任务卡确认后：`reason=dialogue_confirm`

仅成功执行的显式指令计入样本。

## 行为边界

- profile 是 `derived_recent_behavior_not_explicit_player_preference`。
- stable prior 只改变 contextual suggestion 的第三项以及 filtered context。
- 刷新建议不会执行指令。
- 一次相反指令不会立刻清空已有习惯。
- 持续的新行为会在 rolling window 中让 prior 失稳并翻转。
- profile 不单独持久化；由随 SaveGame 保存的 directive events 重建。

## 验收

1. <3 样本不形成 prior。
2. 3 次一致 follow 形成 stable follow。
3. 单次 hold 后 follow 仍 stable，但实际 world order 保持 hold。
4. contextual suggestion 出现“按最近协作习惯：跟随”，刷新不改变 world。
5. 连续 assist 行为使 prior 自适应切换为 assist。
6. Save/load 后恢复保存时的 directive events 与 follow prior。
7. direct_control 与 dialogue_confirm 两条来源均被记录。
