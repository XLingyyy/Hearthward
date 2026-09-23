# TASK-038 交接

## 基线

- TASK-037：`b8f1aaf`
- 分支：`codex/TASK-038-camp-routine`

## 实现

新增 `HearthwardNPCRoutine` 纯策略，以世界 ActivePlaySeconds 确定性选择营地 routine phase；不持久化重复 schedule state。

新增 `NavigateToLocation`，让 routine 使用实际 AAIController / NavMesh 移动，并与现有 actor-target navigation 共用 StopNavigation / retry 边界。

Gameplay 新增：

- `CompanionRoutineEnabled`
- `CompanionRoutineActivity`
- `companion_order=routine`

优先级：

```text
typed task > explicit hold/follow/assist > routine
```

新进度默认 routine enabled。显式 Z/X/C 会关闭。confirmed routine directive 可重新开启。战斗/倒地时 routine suspend。

HUD 会显示“自由活动 · 巡营/查看营地/休息”；filtered AI context 只读暴露 routine enabled/activity，不暴露 waypoint 控制权。

存档新增可选 `companionRoutine`。旧档无字段时默认 false，避免行为突变。

## 验证

- Python repo tests：31/31 PASS。
- repo validator：0 errors。
- Editor Development build：PASS。
- Native `Hearthward.NPCAgent`：14/14 PASS。
- 全量原生 `Hearthward.`：39/39 PASS。
- TASK-038 runtime PIE：26/26 PASS。
  - 新进度 routine=true；tactical intent=routine。
  - routine 使用真实导航，初始观测移动 >26cm。
  - explicit wait -> routine=false / hold，0.8s 内保持静止。
  - confirmed routine -> routine=true。
  - collect MoveTo:Source 时 `TASK_OWNS_COMPANION`，routine activity 为空但授权保留。
  - cancel 后 routine 自动恢复。
  - follow -> routine=false / follow。
  - save/load 恢复 routine=true / wait / active activity。
- TASK-028 executor PIE 重跑：49/49 PASS，采集/缺料/retained cargo/craft/repair/存档重建无回归。

## 边界

当前 Routine 是“低权限生活感”，不是生产系统。它只在营地附近移动和展示状态，不会自动收集、制作、消费或接受任务。正式动画、坐/睡、营火交互与设施工作可在后续有对应 gameplay authority 后扩展。

## 远端

未 push / 未 merge。
