# TASK-100｜全流程覆盖与技术验证

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。已复用098三个设施建立作坊入口到旗点的可编辑源场景，图见samples/Workshop-entry.png和Workshop-inside.png。尚未布入正式四区地图；入口、退路、巡逻和战斗净空的实机验证未完成。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

Active／部分实施，2026-10-07。工作树 `G:/GameFactory/Hearthward`、`codex/TASK-084-103-iteration`，参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加共享未提交实现。用户已授权逐单本地执行，设计问题可暂缓；主Agent派发稳定ID覆盖、现有规则/空间调查和知识泄漏夹具。最终受测构建和运行版本由主Agent绑定；未提交、未推送、未合并、未发布。

## 已实施范围

`../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-100/audit_campaign.py` 实际运行成功，生成 [JSON完整覆盖](CONFIG_COVERAGE.json) 和 [CSV逐ID覆盖](CONFIG_COVERAGE.csv)。有界读取当前gameplay、CampaignState/World/Actor/Quests/Interaction、UI显示、Camp事务和已有测试；未读取或修改Save实现、用户存档、Content、经济/设计配置或共享生产源码。

|实际对象|数量|完整稳定ID出处与边界|
|---|---|---|
|主线|8|main_01—main_08，每条前置、步骤、真实条件源码、触发位置和reward ID已追溯。|
|一次性支线|15|side_01—side_15；包括世界事实、实体救援、真实交付/种植/收获/用餐，完整定义和条件表达式保留。|
|控制区|4|river_gate、workshops、dwellings、assembly；Flag配置ID、实际loc_交互ID、持久zone ID分列。|
|基础驻军|80|逐ID、兵种、阶段、spawn、patrol、zone和defeat事实记录；16/20/20/24与各区声明兵种数一致。|
|固定增援|2组／8人|workshops与assembly各3guard+1archer；每组pending→spawned/cancelled一次，不增加长期反击。|
|可救援|10|rescued_01—10，每人仅属于一条主/支任务；位置、护送条件、真实到营和唯一奖励已追溯。|
|其他实体|20初始／4受保护人物，2序章／3野外敌人|均与80基础/实际增援胜利分母分开；不计入10额外救援或总驻军。|

CSV共127行，表内ID、任务前置/位置、救援归属、基础驻军zone/patrol、奖励物品/唯一reward ID及配置XP与实际Kind奖励一致性检查无结构错误。每条均有真实JSON指针和源码入口。C01静态覆盖可消费；未将配置存在或旧传送脚本当作正常完成。

## 规则、唯一性与知识边界

- Claim实际校验Safe、!Busy、当前epoch、Available、全部Conditions、未Claimed和未RewardFacts；存储预检后授XP/调库存，调库存失败回滚XP与奖励事实。side_06/09必须在营地真实交付6矿/草药，不用库存曾拥有代替。
- 救援由实际Person Actor进入CampAt且玩家不InCombat后调用RecordRescue；复制Camp状态先Rescue，重ID/上限10会拒绝，再授 `rescue:<ID>`。第五名独立族人触发 `reward:rescue5` 一次。092等待不停车已有独立首轮RED与最小修复；本轮integration的真实Fixture为Success，warning 1/error 0，正式600m往返仍未验证。
- 击杀或非致命处决都提交Health=0；败敌奖励键为 `defeat:<ID>:<generation>`。基础/增援generation实际构造为1，配置generation=0为声明，未据此改数据。序章/野外不计胜利，只有野外沿既有刷新条件推进代次。
- 胜利依实际K=N、4旗、2增援终态；N为80/84/88。严格>95%才显示剩余，对应77/80/84清敌。主线前置影响Available/日志，清敌与永久胜利不要求先领奖。
- 永久夺回通过原ReclaimHometown创建本地warehouse_access、2 bed、campfire，与唯一 `reward:hometown` 刃奖励一起事务回滚。仓储/阶级/口粮共享，建筑和岗位带本地Camp；不复制原营地设施/劳动力。main_08实际要求home_storage、home_work、home_continued，保存端的home_saved整合留101检查。
- 地图已有Discovered与Explored可知过滤；JournalEntryKnown采用QuestAvailable，15支线在occupied无额外线索门槛。UnknownSideGoalIsHidden曾实际RED：未获线索/未Discovered/Explored的side_05仍Visible=1，点93000,55000,20000；修复后最新GREEN为Visible=0且原Available=1保持。root在091展示窗口限制未知side标记，未改线索/主线Available或Campaign规则。

## 最窄新增运行夹具

[CampaignCompletionRouteTests.cpp](../../../Source/Hearthward/Tests/CampaignCompletionRouteTests.cpp) 现有3条真实诊断夹具，最新同一轮全部Success、0 warning/0 error：

- `Hearthward.Iteration.Task100.Fixture.UnknownSideGoalIsHidden`：首轮Fail，0 warning/1 error；最新Success，未知side_05隐藏且Available保持，已知位置仍可见、读取不造线索/发奖。
- `Hearthward.Iteration.Task100.Fixture.PrologueActionGoalRemainsVisible`：实际Success，0 warning/0 error；原main_01的prologue_relic→prologue_exit行动目标仍可见，无伪造地图发现。
- `Hearthward.Iteration.Task100.Fixture.WaitingRescueUsesCurrentPersonPosition`：真实首跑Fail，0 warning/4 error；最新Success，0 warning/0 error。原main01/02已领、main03沿原Available；slice_rescue/fork已发现，rescued_01 waiting/Located且偏离旧点。未加载State.Position及实际Campaign.Tick加载Actor的未同步当前位置两者均正确，HasRoute=false；following仍指camp。诊断移动只建立前置，不计正常救援完成。

waiting首跑输出旧slice_rescue点 `-38000,-84500,20000`、route=1/route_fork；实际等待人物在 `-81000,-80000,100`，四项失败分别为State位置、去旧fork、live Actor位置及加载后去旧fork，见 [NATIVE_WAITING_RED.json](NATIVE_WAITING_RED.json)。root在091展示窗口最小修复：Resolve对waiting消费有效Actor当前位置，否则使用Located State.Position，去旧出发途经；准备视图写实际等待目标；ScreenWorldMap消费同一Resolver.World并继续可知过滤。Campaign::QuestLocation/Available/护送/奖励、Save格式和规则均未修改。

## 本轮实际原生证据

首轮9条子集8 Success/1 Fail/7 warning/1 error保留于 [NATIVE_FIRST.json](NATIVE_FIRST.json)，源integration报告时间 `2026.10.06-20.46.13`；随后waiting真实RED单独保存，不覆盖首轮。最新 [NATIVE_GREEN.json](NATIVE_GREEN.json) 完整导出3条100夹具均Success、0 warning/0 error，源 `.agent-local/qa/TASK-090-100/native-green-20261007/index.json`，报告时间 `2026.10.06-21.07.52`；该联合轮15/15 Success、8条warning、0 error。全部绑定6fcf基线加dirty展示修复。原7条Campaign复用本轮integration Success仍保留其自身时间/7 warning，规则未修改；未将两个快照写成一次10项运行。正常地图/键鼠、真人和全通关仍未验。

知识RED原文：`Expected 'Tracking an available side quest must not reveal an unknown location' to be false.` 旧复用7条warning来自5条Campaign067夹具的World清理提示：`LogWorld: UWorld::CleanupWorld called on a world that has begun play, missing call to EndPlay (Untitled)`；最新100三夹具均零warning。不隐藏警告，也不据其推断正式游戏故障。

root在091展示写窗口完成实际RED/GREEN和最小呈现修复；本子Agent没有修改QuestGuidance或Available。夹具无新风格依赖，能独立运行，只覆盖Resolver读取边界，不能代替渲染/键鼠或完整战役。沿[UE官方Automation Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine)已有游戏测试入口，未引入新框架。

## 已有可复用原生入口

本轮以下7条全部Success，0 error，共7 warning；前两条049零警告，其余067保留真实World清理warning（FlagUsesActualActiveSeconds为3条，另4条各1条）：

- `Hearthward.Campaign049.RegistryAndVictory`：实际State稳定ID/80—88分母、严格95%阈值、一次增援与终态、胜利状态往返。
- `Hearthward.Campaign049.ContentContract`：正式23任务、运行目录步骤数量、奖励物品和10稳定救援ID。
- `Hearthward.Campaign067.FlagUsesActualActiveSeconds`：实际WorldClock 5s与40cm移动中断，生产Interact。
- `Hearthward.Campaign067.VictoryActivatesActualSecondCamp`：生产Tick/Reclaim、真实四赠送建筑/共享仓储和唯一刃。
- `Hearthward.Campaign067.FailedSurvivalRejectsAutomaticVictory`：真实失败后不自动获胜/开营/发奖。
- `Hearthward.Campaign067.SecondStage.PromptUsesWeightedControlProgress`：实际提示70/30加权与旗数。
- `Hearthward.Campaign067.SecondStage.OccupiedGiftsStayOnLegalGroundOrRollback`：真实已付费篝火占位时，新赠设施合法放置或整笔回滚。

这些测试采用隔离/构造状态；本单不会把直接清兵/摆旗夹具判为正常通关。Schema7Migration已有但涉及真实临时文件，101按其隔离纪律决定复用，本单未读取或运行存档实现。

## 验收与具体缺口归属

|用例|本轮结果|证据/未闭合条件|
|---|---|---|
|T100-C01 覆盖表|PASS（静态范围）|127行精确ID/指针/源码、全部数量一致、结构错误0；实际触发与正常完成由后续用例验证。|
|T100-C02 四区可走|NOT_RUN|[四区现有说明](../../planning/TASK-084-103/TASK-100/ZONE_COVERAGE.md)已写；092真实净空、正常进入退出/旗帜路线仍需实玩。|
|T100-C03 清敌占旗|Native部分PASS；正常场景NOT_RUN|7条049/067实际Success；真实战斗、击晕和旗帜一次结算仍需正常场景。|
|T100-C04 主线全程|NOT_RUN|8主线真实状态链已追溯；需新档自然连续夜袭→营地→救援→成长→夺回→第二营地，不采用旧逐点传送。|
|T100-C05 支线唯一|知识边界RED→GREEN；完整支线NOT_RUN|side05未知位置已真实复现并修复，已知位置/开场不回归；15支线逐条合法触发/完成/重复领取仍未执行。|
|T100-C06 第二营地|Native部分PASS；正常使用NOT_RUN|真实赠送/占位回滚/失败拒绝等067夹具实际Success；需正常设施/岗位/往返，不能只看State。|
|T100-C07 通关继续|NOT_RUN|原永清/奖励/继续事实已定位；101隔离保存/自然退出/读档/离区重载待执行。|

空间表现缺口：源码每区4座同一028房屋、相同原生杆/方块旗和箱子，已分清“现有配置差异”与“独立区域布景未制作”。区域样板、近远景效果与095—099资产Owner审看待定；未生成或假落新掩体/退路几何。想增加资源、敌人、奖励、门槛或新战争系统才是额外设计决定，本单无这些变更。

工程继续项：既有四区正常游玩、主支线/奖励唯一定向回归、101保存继续及当前独立Shipping候选。095—099批准资产首件与Owner区域样板是表现前置，不能泛化为全单需人工。本子Agent只写100允许Tests和文档，091生产展示修复归root；Campaign/Content/经济/设计/Save规则未改，选包为空。脚本语法、JSON解析、127行CSV唯一键与7复用/3新增过滤器注册、定向diff检查已通过。
