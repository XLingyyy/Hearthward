# TASK-030｜AI NPC 高层战斗指令与确定性战术策略

状态：Blocked（用户已授权本地研发；GitHub Issue 写入当前返回 403，独立 Reviewer 未指定）。

## 核心目标

TASK-030 把现有 Z/X/C 的 `wait / follow / attack` 从“硬编码按钮行为”提升成一条可被自然语言复用、但仍由 UE 掌握战术权威的 AI NPC 战斗边界：

```text
玩家自然语言 / Z X C
        ↓
High-level directive
 hold / follow / assist
        ↓
玩家确认（自然语言路径）
        ↓
Deterministic Combat Policy
        ↓
target / leash / movement intent
        ↓
UE navigation + LOS + cooldown + damage
```

原则：**LLM = strategy；UE = tactics**。

## 能力契约

新增写能力 `companion_order`：

- `item=hold`：原地等待；
- `item=follow`：跟随玩家；
- `item=assist`：协助玩家处理允许范围内的威胁；
- `quantity=1`
- `mode=directive`
- `source=player`

自然语言只允许生成这个高层 directive，不允许生成目标坐标、逐帧动作、攻击时机或伤害值。模型输出仍只是候选卡；**确认前不改变当前委托或 CompanionOrder**。

## Tactical policy

纯函数输入 requested order、玩家健康/战斗状态、玩家和伙伴位置、存活威胁稳定ID/位置/生命、现有 command leash。输出只包含 `Hold / Follow / Assist(target)` 与 reason code。

assist 目标只能来自玩家 leash 内的真实有效威胁。选择顺序：

1. 距玩家更近；
2. 相同时距弟弟更近；
3. 再相同按稳定 enemy ID。

没有合法威胁时 assist 自动退回 Follow，不追击未知/超范围目标。

## 与现有系统关系

- 继续使用 `UHearthwardGameplayComponent::TickCompanion`、现有导航、LOS、attack cooldown 和真实 enemy health。
- 不增加伙伴独立HP，因为当前游戏没有这份权威状态。
- `CompanionOrder` 继续保存 legacy `wait/follow/attack`，避免破坏旧存档。
- natural-language confirm 映射：hold → wait，follow → follow，assist → attack。
- 新增 tactical intent / target / reason 只作为运行时可观测状态，不增加第二套存档事实。

## 验收重点

1. pure policy 覆盖 hold/follow/assist、leash、死亡敌人、稳定 tie-break。
2. structured / natural-language `companion_order` 先生成候选卡，确认后才改变 order。
3. 条件在候选形成后变化时，确认失败而不是执行旧卡。
4. assist 无目标时跟随；有目标时由 UE 选目标并使用既有真实战斗结算。
5. filtered context 只暴露 UE 当前真实 requested order / tactical intent / target / reason。
6. 旧 Z/X/C 与 collect/craft/repair executor 不回归。
