# TASK-036｜战斗协作 Agent：Protect / Regroup

状态：Blocked（本地功能与验证完成；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

在 TASK-030 的 hold / follow / assist 高层指令之上，加入 UE 权威的动态战术决策：

```text
assist order
   ↓
authoritative combat observation
   ↓
healthy + legal threat -> Assist
low health + one close threat -> Protect
low health + multiple close threats -> Regroup
player down -> Regroup
```

模型仍只选择高层 `companion_order=assist`；具体敌人、是否保护、是否回撤、移动、LOS、攻击、伤害均由 UE 决定。

## 第一版能力

- `Protect`：玩家血量低且只有可拦截近身威胁时，优先拦截最相关威胁。
- `Regroup`：玩家低血且近身威胁数量超过阈值时，不继续追敌，撤回玩家附近。
- `PLAYER_DOWN_REGROUP`：玩家倒地时，伙伴停止追击并回到玩家附近。
- 健康状态仍保持原 TASK-030 Assist 逻辑。
- explicit wait / follow 仍保留原语义。
- active collect/craft/repair 仍拥有伙伴导航，不被 combat Tick 抢占。

## 配置

`Resources/Data/gameplay.json` 新增：

- `companionProtectHealthRatio=0.5`
- `companionProtectRadius=300`
- `companionRegroupThreatCount=2`
- `companionRegroupDistance=90`

## 明确不做

- 不让 LLM 选敌人、坐标或逐帧动作。
- 不伪造尚不存在的掩体 / cover-point 系统。
- 不实现玩家倒地复活；当前产品语义仍是倒地后读取存档，伙伴这里只做回援/停止追击。
- 不改变 typed task 对伙伴导航的优先级。

## 验收

1. pure policy 覆盖 assist / protect / regroup / player-down。
2. Editor build PASS。
3. `Hearthward.NPCAgent` 全绿。
4. PIE：Assist 对真实 guard 造成实际伤害。
5. 低血+单威胁切 Protect，并继续走真实 DamageOpponent。
6. 低血+双近身威胁切 Regroup，不带攻击目标。
7. Regroup 从约500cm移动到玩家附近。
8. 玩家倒地后仍执行 companion Tick，切 PLAYER_DOWN_REGROUP 并向玩家移动。
