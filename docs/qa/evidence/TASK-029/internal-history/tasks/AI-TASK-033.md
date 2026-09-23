# TASK-033｜AI NPC Belief / Knowledge State 与认知来源

状态：Blocked（用户已授权本地继续研发；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

把现有“最后亲见营地库存”从裸快照升级成 provenance-aware typed belief，使：

```text
UE World Truth
   │
   ├─ firsthand observation ─┐
   ├─ execution receipt ─────┼─> NPC Belief Store ─> dialogue / reasoning
   └─ player report ─────────┘
                                  │
                                  └─ never replaces execution authority
```

LLM 与玩家文本可以影响“弟弟相信什么”，不能直接改变世界真实库存、安全事实或结算。

## 第一版能力

- typed `camp_stock` belief：item / value / source / recorded_at / revision / campaign。
- 来源：
  - `firsthand`：弟弟实际位于营地并亲自观察。
  - `player_report`：玩家明确报告的数量；永久标注为未核实，直到更强证据覆盖。
  - `receipt`：弟弟本人实际入库后的结算证据。
- `inventory_report` cognition-only capability：quantity 为玩家报告的精确营地库存，只更新 belief，不改实际仓库。
- 离开营地后，真实仓库变化不会自动写入 NPC belief。
- 返回营地后，firsthand 自动覆盖旧 player report / stale belief。
- belief 与世界在同一 SaveGame 快照中保存和恢复，但允许两者值不同。
- legacy `CampInventory / CampObservedAt` 继续保留并迁移成 typed firsthand beliefs。

## 明确不做

- 不把 player report 当成执行权限或世界真值。
- 不允许模型通过 belief 伪造资源、工作台、安全地点、敌人或任务完成。
- 不实现通用知识图谱；第一版只覆盖最有价值的 camp stock。
- 不实现多人之间的知识传播。
- 不实现主动交流；后续独立任务继续。

## 验收

1. BeliefStore 纯测试覆盖 firsthand / player_report / correction / idempotence / legacy migration。
2. `inventory_report` 是有效 cognition-only capability，`WritesWorld()==false`。
3. Editor C++ 实际编译新增 `HearthwardNPCBelief.cpp`。
4. PIE：玩家离营报告 99，实际仓库不变；NPC 查询必须明确“玩家报告、未亲见”。
5. NPC 离营时真实仓库变化，belief 不同步。
6. 回营后 firsthand 自动纠正报告。
7. 再次离营后真实仓库再次变化，belief 保持最后亲见值并明确可能过期。
8. Save/load 保留 belief provenance，并允许 World Truth 与 stale belief 同时恢复。
