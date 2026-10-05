# TASK-071｜TwoCamp 新前置的只读诊断

2026-10-04。根已合入283+/0- opt-in patch。本轮首个实际write见下文；本代理只读取根与本机UE源码，只写自身QA。无Source/root/UE/build/Git操作。以下严格区分源码事实和待实际观察状态。

## 冷却：正确处理点与触发证据

**确认**：GameplayComponent.h:75/100/115的InCombat读取私有CombatRemaining，NotifyCombat置3；GameplayComponent.cpp:485–489只有在Enabled且Clock不Suspended时递减。Clock.Tick只是AdvanceSurvival，四次占旗的20A不执行Gameplay.TickComponent。

**确认**：当前Campaign damage fixture自身的HitTarget209–220→CommitOpponentHealth349–363→Campaign.Damage83–89没有NotifyCombat。ObserveDamage203–208只写真实Target的Detection/LastKnown；Record220–225只记Gameplay.Events并广播。因此514次Campaign原生伤害必然留下3秒冷却这一判断缺少触发依据。

真实触发位置：Combat.Damage164（玩家受到伤害）、Combat.Perception386（感知已交战）、Gameplay.TickCompanion478及原prototype攻击510、NatureActions230（实际野生动物受伤）。本夹具没有在Campaign阶段直接调Combat.Perception或EnemyActor.Tick；CoreTicker派发Loading也没有调World.Tick。因此是否已有非零冷却应读取实际InCombat并记录。

如果真实InCombat为true，最窄正常清理放在80名actual death事务之后、首次flag Interact之前：

```cpp
Clock->Tick(3.f);
Gameplay->TickComponent(3.f,LEVELTICK_All,nullptr);
if(!TestFalse(TEXT("Normal gameplay update ends actual campaign combat cooldown"),Gameplay->InCombat()))return false;
```

与已GREEN ecology同一公开入口。先检查Clock不Suspended；Gameplay.Tick在Loading未结束时直接return，不能清理。可只在InCombat为true时执行这三行，避免新增未触发状态的等待。实际A/W从最终point/manifest绑定，正常+3不会要求修改持久格式。

**明确调用副作用**：Gameplay.TickComponent还调用TickCompanion，并处理原OpponentActors攻击。当前公开hold使RequestedOrder为wait、弟弟未装备weapon；三prototype在真实camp+30000偏移，Hero在home_entry距prototype约1.8km、Brother仍在camp且距离prototype约430m；正式Campaign base不属于OpponentActors。按当前输入没有该路径再次NotifyCombat的条件。如果断言仍失败，读取具体真实触发者，禁止写私有CombatRemaining。

## 无Viewport加载

**确认**：Loading.BeginLoading83–96仅将viewport操作放在可空if内，Loading/Screen在外部初始化。无Viewport不阻止加载开始。FinishSession(true)116–119清AwaitingSession，由Campaign.BeginTravel137–138正常调用。

**确认**：Loading.Tick121–146不要求viewport；要求World.HasBegunPlay、FirstLocalPlayerController及Pawn，随后真实partition/resource readiness与>.7秒墙钟淡出。Hide149–158对viewport作有效性检查，并正常恢复HeldMovement。若FSlate未初始化或dedicated，BeginLoading85直接return，Loading仍false，Campaign真实travel/intro分支仍需完成。

控制器链已查本机UE5.8：GameInstance.AddLocalPlayer948–965正常将对象加入LocalPlayers，即使viewport空；PlayerController.SetPlayer5358–5360同时写Player与InPlayer.PlayerController；GameInstance.GetFirstLocalPlayerController1092–1105直接返回该本地player的controller。根夹具AddLocalPlayer/SetPlayer均存在，无需新viewport对象。

CoreTicker委托由Loading.Initialize71–73注册；现有CompleteScriptedTravel公开派发CoreTicker并保留真实墙钟sleep。已核对LaunchEngineLoop6036–6044 worker调用先于6097–6103 core ticker，本runner同步RunTest路径没有递归执行当前core delegate。不能改为调用privateLoading.Tick/Hide或FinishSession(false)。

**待观察**：IStreamingManager.GetNumWantingResources是全局实际值，源码不证明该进程已经为0；8秒超时只证明真实loading前置未结束。若发生，查看真实partition/resources/controller/intro/Busy，不预设是无Viewport或网络。

## 首营支持和z100

**确认**：Hero/Brother分别在Anchor+(0,0,100)/(0,200,100)于正常BeginPlay前公开放置；CampRoot注册后放Anchor+(0,0,100)，再InitializeFixture。tagged QueryOnly plane中心z−10、halfZ10，实际表面z0，x/y范围覆盖Anchor(-98000,-75000)。

Gameplay.EnableAdventure69–79在当前Temp地图取CampActor−(0,0,100)为Origin；真实top locations.camp偏移(0,0,100)，所以EnsureCamp自然得Anchor+100，与本新增断言一致。Hero实际halfheight90、Brother80，正常movement落地中心约92/82；CampAt以2D半径判断，camp marker的z100不要求胶囊中心恰等于100。原80次正常movement前置已保留。

Campaign scripted travel自己的landings来自真实Ground+100，Finish后正常movement再落地；camp Position与Economy.site都是z100。Opening house的WalkableFloor顶z53，production relic位置z133，Hero opening加15到148，其底z58；Brother中心133/half80的底z53。源码几何符合正常落地空间；**本轮尚无该实际House/FindTeleportSpot成功证据**，不能用坐标算术替代运行正控制。

## Gift与重复Load字段

**确认**：ReclaimHometown286–299只在正常ReadyForVictory/Tick中执行；先加hometown，再AddGift四设施并GrantItemReward。设施或奖励失败会恢复原Buildings/Economy.State并返回false；本夹具不补gift或改Victory。

AddGift73–101自己Ground/CheckGeometry，生成不同GUID并RegisterFacility(Paid={}，Level1)；building actor root位置为保存basePosition+boxHalfZ（BuildingComponent43–53），补丁实际actor位置/yaw检查对应此公开几何。床为两个独立GUID，不合并。

恢复顺序：SaveSubsystem394先Restore真实CampEconomy；Gameplay.Restore662再Builder.Restore实际buildings；RegisterFacility245–247遇既存GUID返回false且不改旧设施/paid/regions；Nature.Restore417；Campaign.Restore418。Campaign.Restore69–75只正常Parse/ResetActors/移动Saved，Continued默认false，不新增home_continued。ResetActors后原生read没有Campaign.Tick，所以不会为验证重做战役、刷新敌人或产生新gift。

Victory SavePoint281–285会向保存副本加home_saved；首个write点包含它，两次Load之后实际State已有同一Fact，resave set重入不增加第二份。Campaign.ReadyForVictory仍要求真实80+终态/四flag；SaveGame264–265还要求Victory==Camp.Hometown且People.arrived与Camp.Rescued对应。当前top quests确有main_01，连续prologue追踪目标可通过Gameplay.ValidateSnapshot。

读取检查使用完整标准Campaign/Camp/Nature snapshot和实际gift/building actors，另核对四gift manifest GUID、uniqueClaim与三处完整库存。标准UStruct序列化不会收录CampRegion的非UPROPERTY Status/PlayerEfficiency/BrotherEfficiency。JSON原文一致性是否保持由真实跨进程测试确认；若只发生集合/格式变化，要先逐领域定位语义事实，不能删断言掩盖额外结算。

最早实际失败应按现有顺序归类：首营/落地 → actual travel/intro/loading → relic/order/escape → 注册Actor真实spawn → death奖励 → flag → victory/gift →管理/安全 →Save/独立Load/noGain。前三者前置未成立时不计Save业务RED。

## 首次实际 WRITE：Source.Camp 的夹具语义错误

根原始报告 `Saved/Task053/restart071-two-camps-write/index.json`：writer PID4168，pool `35dbdccf-4795-4980-97ee-2b72e34a008c`，单个定向测试4.482961秒，1 Error、0 Warning。真实日志为 `Saved/Logs/Hearthward-backup-2026.10.04-01.16.40.log`，33/469行完整命令包含NullRHI、TwoCamps、Ecology；1901行与报告同一705行失败。实际已执行至80注册敌人死亡、四旗、合法胜利、四个实际gift actors及unique blade、两个Nature camp markers；当前尚未执行管理、冷却末尾断言、最终capture/write/read。本次无Save业务RED依据。

原705断言将所有Sources限定为camp/hometown，但源码并无该契约：

- CampSubsystem234–243：新source存 `Camp=State.CampAt(Position)`；已有source保留原camp/position，不按键前缀修改。
- CampState60–63：CampAt按当前营地Tier的二维半径判断，营外返回NAME_None。实际Resources/Data/gameplay.json的T1 radius_m=50；最大T8为120。
- NatureSubsystem122/179–182：ore正常位于距生成Site 16000–28000cm，rich ore50000/64000cm，wild seed6500/11000cm；所有resource依然注册有限source。`camp_048_`或`hometown_048_`只记录生成site，不表示当前管理半径。
- 既有真实 `docs/qa/TASK-049/final-epilogue2-pie.json` 的camp.sources中，camp_048_ore_vein0..3、rich_ore_vein0..1、三种wild_seed各0..1的camp确为"None"。这是已有运行证据，不给本次未完成restore信用。

现有复合断言未记录Id/各操作数。按当前资源顺序和注册路径，首个预期错误是camp_048_ore_vein0的Camp=None；**本次具体首个Id待根下一轮诊断输出确认**。没有证据表明Position与Ground不等。Nature.AddPoint105–113先用真实Nature.Ground将Position写为terrain根，Campaign.Ground43–49同样只接受Landscape/NatureGround；保留当前`.01`根位置断言。

建议的最小夹具修正为每个source.Camp必须等于公开State.CampAt(actual Position)，保留全体真实Ground/Position检查；另要求camp与hometown分别至少一个实际source，保证两营地都独立注册管理范围内的源。重复Load仍比对完整saved Camp/Nature snapshot、stable source identity。本代理未改Source；根已自行将此修正合入当前SaveTests702–711，下一轮实际结果尚未提供，因此不再生成重复patch。

## Load/Viewport 最窄结论

SaveSubsystem.LoadPoint425–436在当前World中直接Restore(saved World)，不调用OpenLevel、BeginLoading或创建Viewport。EnablePrototype137–150使用当前stable Temp map；EnableNaturalWorld155–175才是另一个正式L_HearthwardWilds入口，本夹具没有正式菜单/地图读档信用。

Loading的本地player/controller链已由公开AddLocalPlayer、SetPlayer、Possess与正常BeginPlay建立。首个实际write已经通过CompleteScriptedTravel及正常prologue/camp流程，当前没有加载/viewport失败。无需新增viewport或生命周期fallback。后续独立read应检查LoadPoint首个actual return/status与Participants/Restore结果；没有理由通过创建viewport或私有Hide来改变in-place restore路径。
