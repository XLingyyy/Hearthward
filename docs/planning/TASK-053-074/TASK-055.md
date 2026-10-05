# TASK-055｜接入正式装备语义和玩家战斗

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿057，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-045、TASK-047、TASK-051。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

045战斗、047独立装备实例和051输入已有代码；兄弟已有动作，敌人模型也已导入。未闭合的是各正式武器持握、打击窗口、局部防护、转交／维修全过程及动作质量。本单沿用当前60等级、29技能和现行装备表，不新增第二套攻击数值。

## 装备与操作设计

每件武器、盾、护甲、工具沿InstanceId保持身份，耐久、装备槽、当前位置和来源随转交／仓储／放地不变。双手／盾兼容、承重和耐久失效按047；不能同一实例同时被兄弟穿戴。维修沿实际制造基准20%×修复比例逐材料向上取整，缺料或条件不符完全不结算。

正常键鼠完成轻／重击、锁定／切换、翻滚、盾／武器格挡、弓蓄力／射箭、弩装填和处决／击晕；输入优先复用051，不加绕过语义层的硬编码键。取消和读档清理连击缓冲、锁定、投射物旧epoch；耐力和耐久只在批准时点付一次。

用现有攻击定义作唯一时间基准，将正式动画的准备、有效、恢复段与Sweep／投射物时点对齐；若新增AnimNotify只发当前ActionId事件，不能另算伤害。近战按真实武器轨迹，远程按真实弹体和命中骨骼，头／胸／腿／脚护甲选实际部位。打击、格挡、失衡、受伤、倒地和无耐力都给可辨视觉与声音提示。

## 资产与保存

逐件做武器／盾／护甲外形与socket清单，复用已有兄弟骨架、敌人重定向和Tripo源。新增动作先列用途、时长、打击点、可取消点、骨架／来源；TEMP动作不记正式动画PASS。美术方向未定时仅对齐动作和功能，材质风格由070统一。

装备快照和动作安全边界复用当前schema9，不补发、修满或换新同名实例。唯一护符／故乡刀的来源账本不能靠转交、修理、读档再领奖。

## 最小验证与出口

原生只覆盖本次改动的部位／实例／伤害去重／取消；实际地图完成一套近战和一套远程、格挡到耐力耗尽、装备破损、转交和付费维修，再保存恢复。真人1v3门槛沿ACCEPTANCE分阶段记录；阶段II若尚未具备实际场景，保留未执行，后续066／073补齐。不得用站桩假动作或直接扣血当正式打击表现通过；不重做技能树、改掉落或新增武器类型。

## 建议施工范围

- `Source/Hearthward/Combat/`
- `Source/Hearthward/Inventory/`
- `Source/Hearthward/HearthwardCharacter.cpp及Source/Hearthward/Animation/`
- `Source/Hearthward/Building/HearthwardRepair.cpp`
- `Source/Hearthward/UI/HearthwardScreenEquipment.cpp`
- `Source/Hearthward/UI/HearthwardScreenRepair.cpp`
- `Resources/Data/gameplay.json（仅已批准装备／动作绑定）`
- `Content/Characters/及武器／护甲动画资产（先登记实际包）`
- `Source/Hearthward/Tests/CombatTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-045-combat-alert](../../contracts/CT-TASK-045-combat-alert.md)、[CT-TASK-047-progression-inventory](../../contracts/CT-TASK-047-progression-inventory.md)、[CT-TASK-051-input-traversal](../../contracts/CT-TASK-051-input-traversal.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
