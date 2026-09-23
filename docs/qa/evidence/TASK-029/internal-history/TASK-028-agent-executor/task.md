# Internal AI NPC TASK-028｜Goal → Plan → Action Executor

> Archived during the 2026-09-23 merge because main later reused canonical `TASK-028` for the 3D asset integration task. This file preserves the original AI NPC internal TASK-028 history required by the TASK-029 umbrella delivery.

状态：Blocked（本地研发已由用户授权；GitHub Issue/独立Reviewer待远端通路恢复）。

## 为什么现在做

TASK-025 已把自然语言收敛成 typed Goal，TASK-027 已把 UE 世界事实与 Safety 收敛成统一观察层。当前剩余的结构性瓶颈是 `AHearthwardCompanionFixture::Tick` 仍主要围绕 `EHearthwardCompanionPhase` 分派：每增加一种能力，都有继续增加 `GoingToX / DoingX / ReturningX` 枚举的趋势。

本单把执行核心改成：

```text
Natural language
      ↓
Canonical Goal
      ↓
Deterministic Planner
      ↓
Typed Action[]
      ↓
Action Executor
      ↓
Perception / Safety / Preconditions
      ↓
UE world effects + receipts
```

LLM只负责理解高层目标，不生成逐步执行脚本。

## typed actions

第一版只覆盖已真实存在的能力：

- `MoveTo(Source/Camp/Workshop)`
- `Gather`
- `TakeMaterials`
- `CommitWorkshop`
- `Deposit`

示例：

```text
collect wood
  MoveTo(Source) → Gather → MoveTo(Camp) → Deposit
                       ↑                         │
                       └──── remaining goal ────┘

craft arrows from bag
  MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit

craft arrows from camp
  MoveTo(Camp) → TakeMaterials → MoveTo(Workshop) → CommitWorkshop → MoveTo(Camp) → Deposit

repair own axe
  MoveTo(Workshop) → CommitWorkshop
```

## 迁移原则

`EHearthwardCompanionPhase` 暂不删除，因为HUD、对话状态和旧存档仍依赖它。但从本单开始它是**兼容投影**：

- executor/action cursor 才是运行时执行主状态；
- Phase 由当前 action/recovery 状态映射得到；
- 旧存档恢复时根据 Goal + Phase + acquired/carried/delivered 重建 action cursor；
- 不增加存档 schema 字段，不要求破坏已有存档。

## 安全与结算

TASK-027 的安全层继续处于 action 前置：

- MoveTo source / Gather / TakeMaterials / MoveTo workshop / CommitWorkshop：执行前重验安全；
- MoveTo camp / Deposit：属于安全返营和已获得物资结算，不因战斗状态阻断；
- 所有物资写入继续经过现有 epoch + receipt + settlement guard；
- action retry 不得导致重复获取、重复制作或重复入库。

## 验收

1. planner纯测试覆盖 collect/craft/repair 的plan shape。
2. runtime暴露当前action字符串用于调试和PIE证据。
3. collect多趟、craft bag/camp、repair、材料消失、取消、受阻返营、存档恢复全部保持原语义。
4. 中途存档恢复后plan cursor可从旧字段重建。
5. 全量 `Hearthward` 原生自动化和新增 executor PIE PASS。
