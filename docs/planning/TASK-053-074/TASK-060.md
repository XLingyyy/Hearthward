# TASK-060｜导入动物行为并接通狩猎供给

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿062，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-048、TASK-053、TASK-055、TASK-057。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与已确认缺口

8野生、3家畜、4鱼和狩猎掉落已在Nature；14套配对模型303动作已归档并接入NatureActor。当前未捕获动物由Motion接管移动，Nature的狼主动攻击／羊猪熊反击路径被提前返回绕过。目标是保持现有资产成果，恢复正式生态行为与动画的一致性。

## 行为仲裁与玩家流程

按D03，Nature成为正式地图行为与移动的唯一权威；Motion消费真实速度、警觉、攻击、受击、死亡、牵引等状态。独立动物演示继续其既有状态循环。避免两组件同时SetActorLocation，捕捉／照料／委托时停自主移动，死亡一次停止全部攻击和移动。

鹿／兔／雉／狐逃离，羊／猪／熊受威胁反击，狼主动危险；警觉要求视线，受击确认来源，活动时段只影响活动频率。逃跑6.3m/s基础并随疾跑技能5%优势，回到栖息区后正常减速；危险动物攻击行为沿048固定基伤和间隔，不跟玩家等级动态提高，不受人类难度倍率影响。安全区不放危险兽，狼／熊距首营≥300m，不能把追击带入安全岗位。

真实武器／箭命中骨骼头部要害，弓弩头×3，其他×1；死亡／击晕给一次30经验和肉皮，尸体未拾物保留。出生槽两日新代次只在两兄弟均>80m及安全合法时出现，到期受阻保留Due；不突然在玩家眼前重生。

## 资产与持久化

复用当前14套源／骨架／动作、实际包及绑定表；逐物种列自然活动、逃跑、受击、倒地和攻击片段缺项。野猪仍是猪外形TEMP_VISUAL，专用外形归070；公羊作野羊的外形需Owner视觉确认，不能改名称假装已有独立资产。鱼是钓获表现，首版不新建水下群游模拟。

AnimalId、SlotId、Generation、生命、位置、警觉、尸体掉落和刷新同档；动画相位可重建，但不得恢复生物生命或发重复物资。

## 最小验证与出口

新增仲裁回归必须覆盖正常地图狼主动伤害、熊受击反击、鹿实际逃离、捕获跟随不逃跑、死体保持及拾取一次；同时检查既有演示仍保持速度和边界。正常狩猎→拾肉皮→熟食／加工／捐口粮，用真实库存完成。只对改动物种做定向动作／碰撞检查；正式专用外形在070，全图性能在072。不得宣称现有303动作已覆盖尚无攻击片段。

## 建议施工范围

- `Source/Hearthward/Nature/HearthwardNatureActor.cpp`
- `Source/Hearthward/Nature/HearthwardNatureSubsystem.cpp`
- `Source/Hearthward/Nature/HearthwardNatureState.h`
- `Source/Hearthward/Animals/HearthwardAnimalMotionComponent.h`
- `Source/Hearthward/Animals/HearthwardAnimalMotionComponent.cpp`
- `Source/Hearthward/Animals/HearthwardAnimalAnimInstance.cpp`
- `Resources/Data/animal_motion.json`
- `Resources/Data/gameplay.json（既有动物栖息地绑定）`
- `Content/Hearthward/Nature/及动物动作资产（按实际包登记）`
- `Source/Hearthward/Tests/NatureTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-048-nature](../../contracts/CT-TASK-048-nature.md)、[CT-TASK-051-animal-motion](../../contracts/CT-TASK-051-animal-motion.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
