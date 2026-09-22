# TASK-037 交接

## 基线

- TASK-036：`3206089`
- 分支：`codex/TASK-037-coordination-prior`

## 实现

新增 `HearthwardNPCCoordination` 纯投影模块，从 NPCMemory 中的 `directive` events 计算最近协作 prior。

Profile 包含：

- sample count
- hold / follow / assist counts
- preferred directive
- confidence
- stable flag
- last observed time

阈值：最近8条；至少3样本；主导>=3；confidence>=0.67；领先 runner-up >=2。

`OrderCompanion` 成功后记录 `direct_control`；确认过的 `companion_order` 记录 `dialogue_confirm`。directive event 进入原有 NPCMemory / SaveGame，但被 grounded Episode projection 排除，避免把“按键习惯”混成任务经历。

stable prior 接入：

- `RefreshSuggestions`：第三条建议变为近期协作习惯；
- `BuildFilteredContext`：加入 coordination_profile，并明确语义为行为观察，不是显式偏好。

## 验证

- Editor Development build：PASS。
- Python repository tests：31/31 PASS。
- Native `Hearthward.NPCAgent`：13/13 PASS，新增 `CoordinationPrior`。
- TASK-037 runtime PIE：28/28 PASS。
  - 2次 direct follow：samples=2，unstable。
  - +1次 confirmed follow：3/3 follow，stable confidence=1.0。
  - +1次 hold：3 follow / 1 hold，stable follow confidence=.75；world order 实际保持 wait。
  - refresh suggestions 出现 coordination / “跟着我。”，不自动执行。
  - +6次 assist：rolling window 变为 6 assist / 1 follow / 1 hold，stable assist confidence=.75。
  - 读档恢复为保存时的 3 follow / 1 hold stable follow。
  - event sources 同时存在 direct_control 与 dialogue_confirm。

## 边界

这是可解释的“近期协作 prior”，不是心理状态或永久人格。它不会自行执行动作，也不会覆盖玩家刚下达的显式命令。

## 远端

未 push / 未 merge。
