# TASK-028 交接

## 基线与分支

- 父提交：`25e8f13 feat: add authoritative NPC perception safety layer`（TASK-027）。
- 主实现已提交：`d186dc1 feat: add typed NPC action executor`。
- 当前后续分支已进入 `codex/TASK-029-contextual-suggestions`；TASK-028 的 retained-cargo / legacy-fixture 兼容修正作为独立 follow-up commit 收尾，然后继续保持 029 独立 commit。

## 已完成实现

- 新增 `HearthwardAgentPlan.h/.cpp`：Canonical Goal → deterministic typed actions。
- collect/craft/repair 不再由不断扩张的 Phase 枚举决定主执行分派。
- `FHearthwardAgentExecutionState` 持有运行时 plan/cursor/recovery 状态。
- `EHearthwardCompanionPhase` 保留为旧 HUD / Save / UI 的兼容投影。
- 保存期间不新增 schema 字段；加载后用 Goal + Phase + Acquired/Carried/Delivered 重建 cursor。
- 当前 action 通过 `GetExecutionAction()` 暴露，并进入 NPC observation / LLM filtered context。
- TASK-027 Safety seam 继续位于 side-effect action 前；Return/Deposit 作为安全恢复通道。
- 有限资源、存档恢复、制作、维修、旧 receipts/epoch 逻辑已覆盖。

## 验证

见 [TASK-028 VALIDATION](../qa/evidence/TASK-028/VALIDATION.md)。

当前真实结果：

- HearthwardEditor C++ build：PASS。
- repository validation：PASS / 0 errors。
- Python repository tests：31/31 PASS。
- native Hearthward automation：31/31 PASS（029新增 suggestion test 后当前总量 32/32 PASS）。
- TASK-027 safety PIE：27/27 PASS。
- TASK-028 executor PIE：49/49 PASS，新增 retained-cargo replacement / surplus preservation 覆盖。
- TASK-025 workshop deterministic regression：129/129 PASS。
- TASK-012 旧伙伴完整 PIE：45/45 PASS；retained cargo、去程阻塞、危险中断、返营阻塞、epoch、第二次PIE清理和 manual-console 均恢复。bootstrap 暂停时仅 development-only console command 显式恢复 simulation。

## 关于旧 TASK-012 retained cargo

TASK-012 的 retained-cargo 断言被保留下来，但实现方式现在更明确：只有**来自已取消/被替换命令、真实存在于 NPC 背包里的同类 cargo**可以在新 collect 接受时被 adopt，且最多 adopt 到新任务 requested 数量；多出的物资继续留在背包，不会被无条件入库或虚构成新采集。这与普通 pre-existing cargo 的 `additional_acquired` 语义区分开。

## 下一步

1. 完成 retained-cargo / legacy-fixture follow-up commit。
2. 继续 TASK-029 deterministic contextual suggestions：显式 refresh、未选择不入认知、点击后仍走正常 LLM/candidate/confirm 边界。
3. GitHub 通路恢复后，把 TASK-027、TASK-028 主提交 + 028兼容修正 + TASK-029 分成清晰 commits 统一推送为一个可审查 PR；不自动 merge。
