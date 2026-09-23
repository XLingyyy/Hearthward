# TASK-035 交接

## 基线

- TASK-034：`4bbdac5`
- 分支：`codex/TASK-035-grounded-episodes`

## 实现

新增 `HearthwardNPCEpisode` 派生模块。它从 `FHearthwardNPCEvent` 即时按 command 聚合，不新增持久化摘要。

Episode 记录实际取得、实际入库、制作/维修、重规划次数、原因、完成/取消状态和全部 evidence event IDs。

`BuildFilteredContext` 新增最近3个 `recent_episodes`，并明确规定 past-action claims 只能来自 episode evidence。

`recall` 对“做过/完成/经历/上次/为什么/任务/委托”等历史问题使用 command-scoped episode。新增 `QueryRecentHistory` 作为无需模型的确定性测试/产品 seam。

## 验证

- Editor Development build：PASS，新 Episode 模块实际编译和重链接。
- Python repo tests：31/31 PASS。
- Native `Hearthward.NPCAgent`：12/12 PASS，新增 `GroundedEpisodeProjection`。
- TASK-035 runtime PIE：21/21 PASS。
  - collect2 过程中移动 Source，触发一次 `SOURCE_POSITION_CHANGED` replan。
  - 最终 episode：acquired=2、delivered=2、replans=1、completed=true。
  - episode 保留4个真实 event evidence IDs。
  - grounded history 输出“实际取得2、实际入库2、重规划1次、SOURCE_POSITION_CHANGED、证据”。
  - 保存/读档后 command ID、evidence IDs、projection 和 grounded history 完全一致。

## 边界

Episode 是 read model，不是 write authority。未来自然语言润色只能改措辞，不能增加 episode 中不存在的原因、数量或行为。

## 远端

未 push / 未 merge。
