# TASK-033 交接

## 基线

- TASK-032 adaptive recovery：`33763df`
- AI NPC PR 祖先：`27c4aaa`
- main：`4114556`
- 分支：`codex/TASK-033-belief-state`

## 实现

新增深模块 `HearthwardNPCBelief`，当前只管理 typed `camp_stock` belief。

每条 belief 记录：

- stable id
- item / exact value
- source: `firsthand | player_report | receipt`
- game-time recorded_at
- memory revision
- campaign id

现有 `FHearthwardNPCMemory` 持有 Beliefs，同时保留 legacy CampInventory / CampObservedAt。旧快照在 `Migrate` 时转成 firsthand belief。

新增 cognition-only `inventory_report` capability 和 deterministic `ReportCampInventory` 入口。玩家报告只更新 belief；实际库存不变。模型上下文增加 `camp_beliefs` 和 source / confirmed 字段。

弟弟实际入库时写 receipt belief；在营地观察时写 firsthand belief。离营后 ObserveCamp 不读取仓库，所以真实库存变化不会偷偷同步。

## 验证

- Editor Development build：PASS；`HearthwardNPCBelief.cpp` 及相关 AI/Companion 文件实际编译、DLL 重新链接。
- Python repo tests：31/31 PASS。
- Native `Hearthward.NPCAgent`：10/10 PASS，新增 `BeliefStateProvenance`。
- TASK-033 runtime PIE：25/25 PASS。
  - 初始 firsthand wood=0。
  - 离营后玩家报告 wood=99；belief=99/player_report，世界仍为0。
  - 玩家实际向仓库存1木材；NPC 离营仍 belief=99。
  - NPC 回营后 belief 自动改为1/firsthand。
  - 再次离营，仓库实际变2；belief 仍为1/firsthand，并明确不能保证仍是1。
  - 保存此状态后临时把 belief 改成77/player_report，再读档恢复为 World=2 / Belief=1 / firsthand。

## 设计边界

- Belief 只影响 cognition / dialogue / future high-level reasoning。
- Executor、inventory transfer、safety、combat、settlement 不读取 player report 作为 world truth。
- 当前只覆盖营地库存；source / enemy / location 等 knowledge domains 后续需要真实感知来源后再扩展。

## 远端

未 push / 未 merge。后续可和 TASK-032 作为 AI NPC vNext 增量 PR 交付，但不直接合并。
