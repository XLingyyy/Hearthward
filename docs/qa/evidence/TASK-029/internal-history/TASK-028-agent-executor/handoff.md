# Internal AI NPC TASK-028 交接

> Archived during the 2026-09-23 merge because main later reused canonical `TASK-028` for 3D asset integration.

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

历史证据原路径：`docs/qa/evidence/TASK-028/`。

当时真实结果：

- HearthwardEditor C++ build：PASS。
- repository validation：PASS / 0 errors。
- Python repository tests：31/31 PASS。
- native Hearthward automation：31/31 PASS（029新增 suggestion test 后当前总量 32/32 PASS）。
- TASK-027 safety PIE：27/27 PASS。
- TASK-028 executor PIE：49/49 PASS，新增 retained-cargo replacement / surplus preservation 覆盖。
- TASK-025 workshop deterministic regression：129/129 PASS。
- TASK-012 旧伙伴完整 PIE：45/45 PASS。

## retained cargo

只有来自已取消/被替换命令、真实存在于 NPC 背包里的同类 cargo 可以在新 collect 接受时被 adopt，且最多 adopt 到新任务 requested 数量；多出的物资继续留在背包，不会被无条件入库或虚构成新采集。

## 后续历史

该 executor 后续继续被 TASK-029～040 AI NPC 栈使用，并在 2026-09-23 最终返工中再次跑出 49/49 PASS。最终对外交付口径统一为 TASK-029 AI NPC complete delivery。
