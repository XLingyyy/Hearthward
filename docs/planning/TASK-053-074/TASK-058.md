# TASK-058｜实现营地升阶、设施等级和建筑管理

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿060，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-046、TASK-047、TASK-052、TASK-057。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

046已有1—8阶、半径50—120m、四生产设施三等级、投入账本、搬移／拆除和治疗区。目标是玩家实际选择、支付、定位和管理每一实例，界面与地图范围一致；现有升级规则不重新设计。

## 建造与管理

建造预览显示营地／生产区范围、地面支撑、坡度、障碍、胶囊及占位；合法后5A秒施工，完成才扣真实预留材料。移动／跳跃／受伤／取消按既有规则不产生未完成建筑、不扣建材。已有场景陈设与付费建筑分开，不能把装饰认作可拆设施。

营地升阶核对既有建筑、实际救回人口、捐献和材料，显示下一阶半径／全局属性变化；升级一次提交，人口和全局增益不按两地叠加。设施按各自InstanceId升I／II／III，速度100／125／150%；界面同时呈现设施等级、工作区、队列、工人、已开始批次和暂离原因。

移动免费，保留等级、建材累计账本、队列和产出，停旧工作区后复核新区域，不能复制工人。显式取消已开始批次先显示投入不退、未完成产出0；未开始队列释放预留。拆有活动批次的建筑必须同样确认。拆建材逐项floor(4×累计实付／5)，赠送零投入建筑返0；仓储和已完成产物仍按真实库存保留。

## 保存和双营地接口

设施ID／变换／地区／等级／账本／队列和分配一起保存；移动或升级不变ID。重复拆除回调只结算一次。共享阶级与两地各自建筑沿046，第二营地实际开放由067；本单先覆盖合法局部数据，不用Debug开营地替代剧情验收。

## 最小验证与出口

原生定向查S2前置、不足材料、80%逐项向下、0投入赠品、活动批次取消、同实例移动／升级／恢复。正常UI建工作台、升设施、升营地、移动、拆除并实查仓储；展示第一阶到第八阶表与实际解锁一致，高阶正常供给路线在073验证。不增加住房／床位人口上限、维修税或营地独立等级；美术精修归070。

## 建议施工范围

- `Source/Hearthward/Building/HearthwardBuildingComponent.cpp`
- `Source/Hearthward/Building/HearthwardBuildingEconomy.cpp`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.cpp`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/Save/（仅设施增量）`
- `Resources/Data/gameplay.json（既有营地表接入）`
- `Source/Hearthward/Tests/CampEconomyTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-047-progression-inventory](../../contracts/CT-TASK-047-progression-inventory.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
