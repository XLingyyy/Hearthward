# TASK-032 交接

## 基线

- AI NPC 统一基线：`27c4aaa feat: integrate AI NPC perception executor suggestions and combat`
- 基于最新 main `4114556`（含 TASK-026 PR #25）。
- 本任务在隔离 worktree 中实现，未合并 main，未修改 Content。

## 实现

新增深模块 `HearthwardAgentRecovery`，纯输入/输出决定 executor 失败后的恢复模式：

- `RetryCurrent`
- `RewindToMove`
- `ReturnToCamp`
- `Hold`

`AHearthwardCompanionFixture` 将 action failure 统一送入该策略。runtime execution state 新增 bounded adaptive recovery 状态、重试时间、次数和最后原因。

核心规则：

- Source 在 Gather 中移动 → rewind `MoveTo(Source)`。
- transient route failure → bounded retry / rewind。
- command cargo > 0 → 优先返营保货。
- hard block 不虚构替代目标。
- 成功 world effect 后恢复预算归零。

## 验证

- Python repo tests：31/31 PASS。
- repo validator：0 errors。
- UE Editor Development：新 `HearthwardAgentRecovery.cpp.obj`、更新 `HearthwardCompanionFixture.cpp.obj` 实际生成；`UnrealEditor-Hearthward.dll` 于本轮重新链接。
- `Hearthward.NPCAgent`：9/9 PASS，包含新增 `AdaptiveRecoveryPolicy`。
- TASK-032 runtime PIE：11/11 PASS。
  - Gather 中将 Source 从约 (400,100) 移到 (850,220)。
  - 观察到 `Recovery:Replan->MoveTo:Source` 与 `REPLANNING：SOURCE_POSITION_CHANGED`。
  - adaptive delay 后重新导航到新 Source。
  - 最终 acquired=2、delivered=2、carried=0。
  - Source 实际 -2，Camp 实际 +2。
  - 无手动 `ResumeBlocked`。

## 边界

当前只有已知 Source S1，因此本任务是“对已知目标与路径的可靠重规划”，不是开放世界多目标搜索。资源耗尽、Source 销毁、材料不足、安全失败仍按硬阻塞处理。

## 远端

未 push / 未 merge。本任务应在后续 AI NPC 增量 PR 中单独保留可审查提交或明确 squash 来源。
