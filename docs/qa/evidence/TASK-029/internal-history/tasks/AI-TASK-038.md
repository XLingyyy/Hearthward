# TASK-038｜营地自主生活 / Routine

状态：Blocked（本地功能与验证完成；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

让伙伴在没有任务、没有显式 Follow/Assist/Hold、玩家不在战斗/倒地状态时，在营地附近进行低权限自主活动，使 NPC 不再只是“等 prompt 的雕像”。

```text
no typed task
+ routine authorized
+ order = wait
+ safe player state
        ↓
world-time deterministic routine phase
        ↓
real UE navigation around camp
```

任何玩家显式指令或 typed task 都具有更高优先级。

## 第一版 Routine

按 ActivePlaySeconds 每 6 秒确定性切换：

- rest
- patrol
- check_camp
- patrol

若伙伴距离营地 >450cm，则优先 return_camp。

Routine 只产生移动与可见 activity，不生产物资、不修改任务、不调用 LLM。

## 授权语义

- 新进度默认 `CompanionRoutineEnabled=true`。
- 显式 Z/Wait、X/Follow、C/Assist 会关闭 Routine。
- 新增高层 `companion_order=routine`，玩家确认后重新授权营地自由活动。
- typed collect/craft/repair 执行时 Routine 保留授权但暂停，让 executor 独占导航；任务结束/取消后自动恢复。
- 战斗或玩家倒地时 Routine policy 不激活。

## 导航

新增 `AHearthwardCompanionFixture::NavigateToLocation`，复用现有 AAIController/NavMesh、重试与 StopNavigation 语义，不使用 teleport 假移动。

## 存档

Gameplay snapshot 新增可选 `companionRoutine`：

- 新存档明确保存授权位。
- 旧存档无该字段时默认 false，保持旧行为，不在加载后突然开始自主移动。
- routine phase 不单独存储，由世界时间确定性重建。

## 验收

1. CampRoutinePolicy pure test 覆盖 rest/patrol/return/suspend。
2. 新进度 Routine 默认启用并产生真实移动。
3. Wait 立即关闭 Routine 且伙伴停住。
4. confirmed routine directive 可重新启用。
5. typed task 抢占 Routine navigation，授权位保留。
6. 任务取消后 Routine 自动恢复。
7. Follow 覆盖并关闭 Routine。
8. Save/load 恢复 routine 授权与 activity。
9. 全量原生回归和 TASK-028 executor 回归保持通过。
