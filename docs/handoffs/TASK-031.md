# TASK-031 交接

## 基线

- 基线 commit：`e4bcaf7 feat: add deterministic companion combat policy`
- 任务规划 commit：`c1c0feb docs: plan TASK-031 user retest fixes`
- 实现 commit：`e1dab72 fix: preserve NPC task navigation after combat policy`
- 分支：`codex/TASK-031-user-retest-fixes`

## 用户复验反馈

用户在 UE 中观察到：

- 对话页右上动态控件压住“弟弟”标题；
- “帮我采集两份木材带回营地”确认后伙伴无移动。

## 根因

导航问题不是模型理解失败，也不是任务卡未确认。

`UHearthwardGameplayComponent::TickCompanion` 在 typed task active phase 分支调用了 `StopNavigation()`。该 Tick 每帧执行，于是 TASK-028 executor 刚发出的 MoveTo 会被 combat policy 立即取消。

## 修正

- active typed task 时 combat policy 只记录 `TASK_OWNS_COMPANION` 后 return，不再触碰导航。
- “刷新建议 / 记忆与约定”移入 y=240 工具栏，离开静态标题 y=290 区域。
- clarification 清理按钮上移到独立位置。
- manual task capability 过滤为 collect/craft/repair，避免 companion_order 被错误显示为采集。

## 当前验证

- HearthwardEditor Development build：PASS。
- full native `Hearthward.`：33/33 PASS。
- TASK-030 deterministic combat PIE：29/29 PASS。
- 用户反馈定向 PIE：11/11 PASS；标题区域无动态 action 覆盖，确认 collect 后产生实际移动并完成2/2交付。
- TASK-028 executor PIE：49/49 PASS。
- 用户原句真实 Qwen 复验：11/11 PASS；`帮我采集两份木材带回营地。` → `collect/wood/2/additional_acquired/S1`，确认后实际移动并完成2/2。
- repo validation：0 errors；TASK-031 scope validation against planning baseline `c1c0feb`：0 errors；Python repo tests 31/31 PASS。

运行证据同步记录在 TASK-030 validation，因为本单直接修复该功能链；TASK-031 保留独立任务范围、交接和提交。

## 统一 PR 整理

2026-09-22 用户要求把当前全部 AI NPC 修改整理后提交 PR，但不直接合并。

为避免与已进入 main 的 TASK-026 文档/地图分支混杂，先在本地历史分支上基于最新 `origin/main=4114556` 按序整理 TASK-027→031，再把最终树 squash 到 `codex/ai-npc-stack-pr`，因此远端 PR 只需要一个干净提交。详细来源历史仍保留在本地：

- `efb9cb0` — authoritative NPC perception/safety
- `5c8123e` — typed Goal→Plan→Action executor
- `6a07721` — executor compatibility / retained cargo
- `2bd20d1` — contextual suggestions
- `a9aa8d8` — deterministic companion combat policy
- `ea713fc` — TASK-031 planning snapshot
- `573da49` — user-retest navigation/UI fixes
- `b8b991e` — TASK-031 retest documentation

最终 squash PR 树重新验证：Editor build PASS、full native 33/33、用户反馈 PIE 11/11、combat PIE 29/29、真实 Qwen 原句采集 11/11。TASK-026 的 main 内容保持不变，PR diff 不包含任何 `Content/`、`.uasset`、`.umap`、`.blend` 或设计 DOCX 资产修改。

## 远端状态

用户已授权 push / 创建统一 PR，但未授权 Agent 合并。Issue 创建在前序集成中曾返回403；若 PR API/CLI 仍受认证限制，分支 push 后以 GitHub compare 页面交给用户创建/评审。
