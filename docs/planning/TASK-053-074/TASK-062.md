# TASK-062｜实现种植、照料和个体养殖

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿064，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-048、TASK-052、TASK-058、TASK-059。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

048已实现三作物、栏舍、稳定个体、饲料／繁殖，050追加蛋奶和弟弟照料。目标是在真实营地可选耕地／个体、完成照料和产物供给；原稿无蛋奶口径已失效。

## 种植流程

绿叶／谷物／草药各1种子，成长2／3／4日，基产4／8／12并返1种子。该轮浇水、施肥各一次＋25%产量；界面分开显示成长、已照料、可收获、预计真实产物和容量。重复照料不重加收益，成熟不自动收割或播种。实际收割成功才改作物并发成品，满包不消失。

耕地需要真实支撑和营地合法范围，模型不同成长阶段，交互目标用稳定PlotId，视觉隐藏不重置成长；睡眠／等待通过052推进W，无离线收益。弟弟只照料已知且授权点，本单不扩模型权限。

## 个体养殖流程

山羊、家猪、家鸡可从现有家畜出生点捕捉，牵回预留栏位后才入栏；战斗、满栏、牵引失败先保留个体与原有占位关系，不隔空完成。栏舍I／II／III在营地2／4／6阶，容量6／10／14，费用沿048。

每个AnimalId保存成年／幼年、配对、饲料、繁殖／成长和所属栏。成年日料2／3／1，幼年1／2／1，足料同物种成体按稳定ID两两配对，累计2日生1只，幼年2日成年；满栏停止繁殖并清零未完成进度，空出栏位后重新累计2日，无积欠幼体。缺料暂停批准的生长／产出，不引入性别、饿死、病害或遗传。

2026-10-03施工核对纠正：初版整理误写“性别／异性”和满栏待结算。本批BASELINE优先CURRENT及已批准043—052，DECISIONS未提出这两项新增规则，因此沿DSGN-R15 D5／CT-TASK-048原有配对与满栏契约；不为该文案错误增加字段或迁移。

成年已喂山羊／鸡每1440W分别产奶／蛋1份；每栏最多24份，无积压补发，每份20食物。产品领取与饲料消耗、动物成长、栏容量同笔权威结算，可进入现有烹饪／公共口粮，不加新配方福利。原稿仅肉收益不再适用。

## 最小验证与出口

原生测两次照料、跨2／3／4日、容量满、配对停止／恢复、幼年成长、蛋奶满24停止与不补发、同档恢复；实际UI种植→照料→睡眠→收割，并捕捉→牵回→喂养→产蛋奶／繁殖，做一次存档恢复。核对动物动作组件不能让栏内个体逃离或重复入栏。正常等待时间沿现行W／A，验证可使用真实床／篝火；不改倍速缩短设计门槛。

## 建议施工范围

- `Source/Hearthward/Nature/`
- `Source/Hearthward/UI/HearthwardScreenNature.cpp`
- `Source/Hearthward/Building/HearthwardBuildingEconomy.cpp（栏舍接入）`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.cpp（真实饲料／产品）`
- `Source/Hearthward/Save/（仅新增生态字段）`
- `Resources/Data/gameplay.json（既有作物／栏舍绑定）`
- `Content/Hearthward/Nature/作物与栏舍资产（按实际包登记）`
- `Source/Hearthward/Tests/NatureTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-048-nature](../../contracts/CT-TASK-048-nature.md)、[CT-TASK-050-companion-activities](../../contracts/CT-TASK-050-companion-activities.md)、[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
