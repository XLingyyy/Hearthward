# TASK-054｜实现正式生存、药品、倒地和救援闭环

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿056，阶段B，P0；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-044、TASK-050、TASK-052。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

044已有Survival组件与死亡优先级，050有弟弟自动维持和互救，052统一时间。目标是在自然地图正常操作形成从受伤到恢复、倒地互救、死亡失败、选档恢复的完整体验；不再造一个生命状态机。

## 玩家流程与规则

玩家在背包／快捷栏使用真实药品，开始时校验物品、生命状态及当前动作，显示3A秒进度；完成只消费一次，沿044完整／半份药量及恢复公式，不把开始／取消当作成功。移动、正伤害、菜单语义冲突等按批准中断条件处理。完整速效药30%最大血，持续药15A秒共50%；伤害中断只损半份，半份可用且疗效减半，半份再被伤害中断则耗尽。非受伤取消不扣药，同帧受伤／取消只结算一次；持续药只刷新不叠加，满血或倒地不能普通用药。食物、自然恢复及其他数值沿DSGN-R04和运行表。

倒地120A秒；可操作的同伴在2m内保持5A秒救援，成功10%最大血。开始、持续和结束都核对距离、epoch、生命及遮挡；受击／离开／取消终止进度。两人倒地、120秒耗尽、主动放弃、三日饥饿终态及水／虚空死亡，采用已有唯一失败判定，先处理终态再处理同帧救援，禁止死亡后完成救援回血。

界面呈现倒地期限、救援进度、可救条件、中断原因和明确放弃确认。失败时暂停并进入存档恢复／回主菜单；关闭弹窗不能继续已终结世界。设置页暂停默认开启，弟弟对话继续运行；保存／加载／失败冻结语义沿051／052，不能开背包无限回血。

床休息及真实治疗区仅在合法位置／条件下提供对应恢复；睡眠或篝火跳时由协调器统一推进，只结算允许领域。普通族人无个体生存状态，不能因演示救援新增族人死亡。

## 持久化与最小验证

当前快照保存生命、饥饿、倒地剩余、动作等已有字段。新增UI提示从权威状态派生，不存第二套生命值。读档取消旧药品／救援，恢复同一时间线；危险保存仍按现有延后政策。

定向原生：119／120秒、救援与死亡同帧、两人倒地、药品中断、不重复扣药、饥饿终态优先；实际地图：玩家救弟弟、弟弟救玩家、无药恢复、治疗区、深水／虚空失败、暂停倒计时及失败后读档。界面实测剩余时间和原因是否可懂。本单不调药效、不新增永久伤残、饥饿罢工或普通族人死亡。

## 建议施工范围

- `Source/Hearthward/Survival/`
- `Source/Hearthward/Interaction/HearthwardFurnitureInteractionComponent.cpp`
- `Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardHUD.cpp`
- `Source/Hearthward/Save/（仅必要生存字段）`
- `Source/Hearthward/Tests/SurvivalTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-044-survival-transitions](../../contracts/CT-TASK-044-survival-transitions.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
