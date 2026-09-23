# TASK-034 交接

## 基线

- TASK-033 belief state：`330c628`
- 分支：`codex/TASK-034-event-driven-initiative`

## 实现

新增 `HearthwardNPCInitiative` 纯策略模块，将以下真实信号转换为 grounded initiative：

- completed → task_completed
- replanned → task_replanned
- blocked → task_blocked
- player_report 与回营 firsthand 不一致 → belief_corrected

`UHearthwardLocalAISubsystem` 新增 runtime-only bounded queue、dedupe history、active initiative、显示 TTL 与 cooldown。

事件发生时不要求玩家在旁边。若玩家不在伙伴 30m communication range，消息保留在队列；重新靠近且当前没有模型回复、candidate 或 clarification 时才展示。

显示复用现有 `GetStatus/GetNPCLine/CanDisplay` 与 HUD，不新增第二套 UI。Active initiative 显示状态为“弟弟主动提醒”。

## 验证

- Editor Development build：PASS，新 Initiative 模块实际编译并重链接。
- Python repo tests：31/31 PASS。
- Native `Hearthward.NPCAgent`：11/11 PASS，新增 `EventDrivenInitiativePolicy`。
- TASK-034 runtime PIE：16/16 PASS。
  - collect 2 完成后，无新 prompt，主动显示“木材已经处理好了，实际完成 2。”
  - player_report=99 被 firsthand=2 纠正后主动说明来源冲突。
  - replan 时玩家被移到 >30m，initiative 不显示。
  - 玩家重新靠近后 queued `task_replanned` 才显示。
  - 上述主动提醒期间 `is_busy=false`，没有启动模型请求。

## 边界

当前主动台词是 deterministic grounded copy，不使用 LLM 润色。后续如果需要自然语言润色，应把 LLM 限制为“已有事实的措辞层”，不能决定是否触发、不能新增事实。

## 远端

未 push / 未 merge。
