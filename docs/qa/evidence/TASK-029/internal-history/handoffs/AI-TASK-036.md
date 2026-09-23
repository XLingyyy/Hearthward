# TASK-036 交接

## 基线

- TASK-035：`7b80aff`（其后文档修正提交 `9f9f93a`）
- 分支：`codex/TASK-036-tactical-cooperation`

## 实现

扩展 `HearthwardCompanionCombatPolicy`：

- `Assist`
- `Protect`
- `Regroup`

新增 observation 参数：

- player health ratio
- protect health threshold
- protect radius
- regroup threat count

运行时规则：

- health > threshold：保持原 Assist。
- low health + 1 个近身威胁：Protect，拦截真实威胁。
- low health + >=2 个近身威胁：Regroup，不再追敌。
- player down：PLAYER_DOWN_REGROUP。
- wait/follow 显式指令仍保持原优先级。

修复一个运行时断路：此前 `TickComponent` 在 Health<=0 时先 return，使 policy 的 PLAYER_DOWN 分支永远不可达。现在先执行 `TickCompanion`，再停止玩家自身 gameplay tick。

## 验证

- repository tests：31/31 PASS。
- repo validator：0 errors。
- Editor Development build：PASS。
- Native `Hearthward.NPCAgent`：12/12 PASS。
- TASK-036 runtime PIE：16/16 PASS。
  - healthy Assist 命中真实 guard，Opponent health 实际下降。
  - player health≈36% 时，对单个近身 guard 切 `protect / PROTECT_LOW_HEALTH` 并继续真实攻击。
  - 双近身威胁切 `regroup / LOW_HEALTH_OVERWHELMED`。
  - Regroup 距离约 500cm → 89.5cm。
  - player health=0 后切 `regroup / PLAYER_DOWN_REGROUP`。
  - 倒地回援距离约 500cm → 142cm。

## 边界

“Protect”当前承担近身拦截/保护职责；尚无世界级掩体数据，因此没有伪造 cover mechanic。玩家倒地也没有复活结算，本任务只保证伙伴停止追击并回援。

## 远端

未 push / 未 merge。
