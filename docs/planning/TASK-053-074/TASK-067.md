# TASK-067｜实现永久夺回、双营地和通关后世界

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿069，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-043、TASK-046、TASK-049、TASK-058、TASK-059、TASK-066。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

043永久规则、046双营地字段、049控制和Victory事务已存在。目标是严格计数、实际夺旗、安全转换、营地2布局和通关后未完支线的玩家闭环；不增加反攻战争或夺回维护费。

## 控制与永久胜利

总进度P=70%×K／N＋30%×C／4，70／30是清敌与旗帜的权重。区域夺旗前必须清除归属本区的全部基础敌和已生成增援；偷袭、击晕、正面击杀等价。旗帜需5A秒交互，外区活敌进入本区中断动作；已控制旗不回退，不用进度条四舍五入改变资格。

最终N由80基础＋已生成增援组成，只能80／84／88；K=N、四旗已控、两增援触发器终结才Victory。K严格>95%才提示剩余敌大致位置，分别至少77／80／84；跨区、离屏和增援都按稳定ID查。不可达敌回合法原岗保留血／异常／奖代次，无合法岗报告真实阻塞，不代杀凑数。

永久Victory事务一起提交不再刷故乡敌、开放第二营地、故乡唯一装备、世界安全入口；任一账本失败则整笔失败。M07领奖只结主线经验，不再发刀／营地。野外普通敌四日刷新照旧，故乡永久状态不被时钟／读档覆盖。

## 双营地与后续生活

第二营地即时给仓储入口1、床2、篝火1，建材投入0，拆除0返料。共享同一仓储／阶级／人口／口粮／蓝图／一次属性；四设施和治疗区当地建设、各自等级／队列／布局。换营调普通人移动唯一关系，不模拟运输；弟弟劳动需实际到场。

故乡站点在胜利后激活，沿052零跳时旅行；未完成营救／取物／知识支线保持安全可完成。通关提示后可继续种养、建设、钓鱼、协作，不启动新敌占周期、仓库运输税或重复主线。

## 最小验证与出口

原生检查控制严格边界、跨区干扰、N三个合法值、最后敌／最后旗顺序、事务失败、重复Victory和回档；正常游戏完成一次真实夺回，分别在两营建／移动／生产、分配人、存取同库、领取公共饭及旅行，再存档继续。验证通关后4日不复活故乡敌而野外能刷新。全23任务兼容在066／073报告，本单不能只靠开关flag验第二营地。

## 建议施工范围

- `Source/Hearthward/Campaign/HearthwardCampaignState.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Camp/`
- `Source/Hearthward/Building/HearthwardBuildingEconomy.cpp`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/Tests/CampaignTests.cpp`
- `Source/Hearthward/Tests/CampEconomyTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-043-world-time-persistence](../../contracts/CT-TASK-043-world-time-persistence.md)、[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
