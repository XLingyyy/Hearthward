# TASK-057｜接通正式采集、工具、蓝图与加工链

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿059，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-043、TASK-047、TASK-048、TASK-052。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

当前有58即时配方、38修理基准、工具独立实例、真实资源与蓝图事实。目标是正常新游戏从初级工具到木石／绳、锭／精炼锭、熟食／药物、设施／装备的可操作供给链。沿048点位和047门槛，不用Debug给予材料或学全图纸。

## 玩家流程

在首营附近通过实际E取得木、石、食物／草药，提示当前工具、一次产量、剩余量与耐久；缺工具、工具等级不足、超重、遮挡或已耗尽应明确失败，不扣耐久／时间外的未授权费用。普通资源2日、精矿4日到期；部分采集不延期，到期受阻保留原Due，按052稳定点ID恢复。

用实际库存建工作台I，学习初级绳／箭／斧／药等已批准配方；冶炼转矿与燃料为锭，精炼锭再进锻造／维修。加工面板显示输入来源、整批材料、成品重量、设施种类／等级、图纸条件和失败原因。只调用既有Workshop／Inventory事务，手工即时与后台有限生产不建立两套配方。

稀有知识从049支线／061钓鱼来源获得，事实与物品分别处理；已会同一图纸时任务继续但不能重复知识奖。拾取源、装备掉落和制作结果保持稳定来源／实例；已完成产物满包则按既有待领取保留，不用再执行配方生成第二份。

## 切片经济与状态

已批准工作台I木72，S2木48石24，合计木120石24，另计工具、床、食物和行路。首营周边点位数量和木／石产量沿048，先核对合法初始行装与仓储，制定实际可采路线；不能新增隐性免费包压缩时间。中间投入、队列、工具耐久、资源剩余、已知配方与产物同档恢复，无离线补料。

## 最小验证与出口

定向测缺工具／等级、容量变化、采集取消、不重复磨损、刷新受阻与回档；实际键鼠从真实来源采集→加工→制作装备／药／熟食→维修，并完成工作台I＋S2材料路径。写清每条链的设施和图纸入口；计时交065，不提前调配方。禁止新造制作系统、免费采集测试当交付、自动赠满蓝图或刷料。

## 建议施工范围

- `Source/Hearthward/Interaction/HearthwardHarvestSubsystem.cpp`
- `Source/Hearthward/Nature/HearthwardNatureActions.cpp`
- `Source/Hearthward/Nature/HearthwardNatureSubsystem.cpp`
- `Source/Hearthward/Building/HearthwardCrafting.cpp`
- `Source/Hearthward/Building/HearthwardWorkshopService.cpp`
- `Source/Hearthward/UI/HearthwardScreenCrafting.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Resources/Data/gameplay.json（点位／设施绑定）`
- `Source/Hearthward/Tests/HarvestRefreshTests.cpp`
- `Source/Hearthward/Tests/NatureTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-047-progression-inventory](../../contracts/CT-TASK-047-progression-inventory.md)、[CT-TASK-048-nature](../../contracts/CT-TASK-048-nature.md)、[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
