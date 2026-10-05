# TASK-071｜连续战役产生双营地的 Native 跨进程增量方案

2026-10-04。相对根 task051 当前 `Source/Hearthward/Tests/SaveTests.cpp` 的 ecology 已合入版本调查。仅本 QA 草稿写入；Source、Content、根工作树只读，未运行 UE/build/Git。本场景及其所有新前置均 **NOT_RUN**。ecology write 的真实成功由根提供，本草稿不替代其 read 报告。

## 最小范围与已有路径

追加独立 opt-in `-Hearthward071RestartTwoCamps`，沿用 `Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket`、显式 write/read、同一隔离 pool、稳定 `/Temp/Task071RestartFixture`。只需 SaveTests.cpp 内该函数的局部 hunk 和实际所需 include；无需生产 API、地图、快照字段或依赖。two-camps 可单独运行，也可与 ecology 联合；联合时先战役、后生态，使用同一个 fresh pool 及两项 opt-in。

原默认/no-phase 与 ecology 路径保持。既有真实 workbench72、partial rope、treasure Pending、三处完整库存、active collect、NPC memory、独立 PID、两次 LoadPoint/noGain/resave 全部保留。连续战役的前置插在真实 paid workbench 完成之后，ReadMap/rope/active collect 之前；这时没有已投入 rope 需要等待，也没有活动伙伴命令妨碍旅行或分配。

额外 include：`../Campaign/HearthwardCampaignSubsystem.h`、`../Campaign/HearthwardCampaignActor.h`、`../UI/HearthwardLoadingSubsystem.h`、`Containers/Ticker.h`、`HAL/PlatformTime.h`。`HearthwardCampaignState.h`、Combat、Capsule、PlatformProcess、Survival 与正常 world lifecycle 已存在。没有新文件 helper。

## 已确认的前置成本

实际 `Resources/Data/gameplay.json` 与 State.Initialize 注册80名 base，另有2名 prologue、3名 field；初始两增援 pending，正常报警时各增4名。清除计数只纳入 base/reinforcement。

| 区域 | base身份数 | guard / archer / heavy | 当前 axe30 的最小原始伤害调用数 |
| --- | ---: | --- | ---: |
| river_gate | 16 | 10 / 4 / 2 | 66 |
| workshops | 20 | 12 / 5 / 3 | 110 |
| dwellings | 20 | 12 / 4 / 4 | 116 |
| assembly | 24 | 12 / 6 / 6 | 222 |
| 总计 | 80 | 46 / 19 / 15 | 514 |

514来自当前血量表和 axe AttackPower30 的静态计算，尚未实测。单个最大为 stage3 heavy450HP的15次。生产 CampaignActor.Target 当前 Armor/ArmorDurability为空，HitTarget每次使用实际正 AttackPower；Native仍读取运行时 Health/AttackPower 并断言每次实际降低，不把514硬编码为成功条件。有限上限可取32次/身份，既覆盖当前15次最大，也保证失败立即报告，不无限循环。

本最小路径不制造报警；清除实际 base 后由 Campaign.Tick 自己 ResolveUntriggered，将两个 pending 自然变 cancelled，Total/Cleared为80。若运行时真实报警产生 spawned，需同时经真实死亡入口清除新增 registry身份并保存实际分母，或报告前置范围变化；不能把 spawned 手改为 cancelled。本独立用例可断言恰好80/80且两 cancelled，以固定最小路径；88身份/报警触发另留战斗验收。

额外事务包括一次真实 .4A装备切换、两个实际 scripted travel（序章开场/后巷撤离）、真实三秒开场计时，以及四次五秒占旗。加载淡出使用真实墙钟≥.7秒；A/W仅从最后真实 SavePoint读取，加载期间Clock.Suspended不计入A/W。没有把514次HitTarget转成514次World.Tick，也没有绕过生存或直接发XP。这个用例证明原生生产事务与存档，正式地图实战命中、AI进攻、导航、真人体验需独立验收。

## 夹具的四处准确增量

1. 在 RestartEcology 解析附近解析 RestartTwoCamps，并要求显式 write/read；使用各自 fresh pool。无 phase 时原路径照旧。
2. 在当前 Hero/Brother spawn 前，取真实 campaign `locations.camp` 的 XY：`HearthwardCampaign::XY(HearthwardCampaign::Find(TEXT("locations"),TEXT("camp")),TEXT("xy"))`，目前为(-98000,-75000,0)。仅 two-camps 使用此 Anchor。Hero spawn Anchor+(0,0,100)，Brother spawn Anchor+(0,200,100)。CampRoot注册后将真实 Camp actor 放到 Anchor+(0,0,100)，再 InitializeFixture。这样 EnableAdventure 内 Origin=CampActor-100、locations.camp偏移+100，自然建立首营(-98000,-75000,100)，无需 AddCamp 或修改 Economy.State。
3. 仅 two-camps 将原 tagged QueryOnly Box extent(80000,80000,10)扩为(200000,160000,10)，中心仍(0,0,-10)，原 InitializeActorsForPlay/NotifyBeginPlay/实际胶囊落地保留。现有地面探针增加正式 anchor的 Ground实证；Nature.EnsureWorld后验证 camp marker及全部生成资源/slot位置确有真实 tagged ground。
4. 现有三 prototype场景对手 relocation继续以实际 Site+30000偏移，保留 Alive/Health/支持面正控制。这些 prototype不进入 Campaign registry，不为完成战役杀死它们。write/read已有三对手完整恢复断言保留。

精确范围：首营(-98000,-75000)，故乡(109000,45000)；relic(92000,53500)，exit(90000,62000)；四旗(100000,41000)、(117000,35000)、(93000,55000)、(117000,55000)。base出生范围x91600..126200、y28500..62500。Nature richest source最大半径64000；两营地加此半径的保守范围x[-162000,173000]、y[-139000,109000]均被候选平面覆盖。仍须实际 Ground验证每个生成点，尺寸算术只支持夹具选择，不作为正式地图地形验收。

## 公开生命周期与连续推进顺序

当前同步 RunTest 共享GFrameCounter，原生 Movement直接TickComponent的已通过方法继续使用；不能重复 World.Tick 并假设胶囊重复推进。局部场景定位只调用公开 SetActorLocation，根据真实 Ground与各角色实际 CapsuleHalfHeight+3落位，再调用实际 Movement.TickComponent，明确断言 Walking。这是 Native场景fixture定位。序章中只在生产 intro/travel已结束后定位，不人工切 MovementMode。

### A. 正常 Start / 序章 / 后巷撤离

1. 正常 `Gameplay->EquipInstance(PlayerBag->FirstInstance(TEXT("axe")))`，Clock.Tick(.4) + Combat.TickComponent(.4)，确认实际当前 weapon GUID/axe/positive AttackPower以及!Combat.Busy。
2. 调用公开 `Campaign->Start()`。它真实 State.Initialize()，BeginTravel(prologue_relic)，创建独立 StreamingSource，Loading.BeginLoading/FinishSession(true)。先断言Phase prologue、非Legacy、80base、2prologue、3field、10people和两pending。依据实际registered Id，不自建敌人。
3. 完成真实 travel/intro/loading：以有界8秒墙钟 poll，逐次调用 Campaign.Tick(.025)、公开 `FTSTicker::GetCoreTicker().Tick(.025)`、Clock.Tick(.025)和必要真实movement TickComponent(.025)，FPlatformProcess::Sleep(.005)给实际.7秒淡出时间。Campaign完毕且Loading.IsLoading false后，再断言Clock不Suspended、双方落地/Alive。intro由Campaign自己的Delta倒数，不写Facts或Remaining。
4. 公开 `Campaign->Interact()`取得真正 relic/amulet。检查 Facts relic、实际 amulet及campaign_start_amulet RewardFact。正常 `Gameplay->ApplyCompanionDirective(Hero,TEXT("hold"))`产生 prologue_order；检查Brother在沟通范围内。无调用 Campaign.Record 代做步骤。
5. 公开场景定位Hero/Brother到真实 Ground(prologue_exit)，两个角色使用不同落点(0/180cm)，检查相距<1000、双方Alive。公开 Interact启动真实camp scripted travel，再以上真实poll完成。断言Phase occupied、prologue_placed/prologue_intro/relic/prologue_order/prologue_complete都由生产产生、两角色落在真实首营、camp已Activated且CampAt玩家=camp。

**Loading.Tick是private**（LoadingSubsystem.h:20），不可直接调用；Hide也private，不使用ProcessEvent/private setter或 FinishSession(false)虚报失败来消除加载。正常委托由CoreTicker公开派发。UE5.8 LaunchEngineLoop.cpp:6036–6044先AutomationWorker.Tick，6097–6103再CoreTicker.Tick；AutomationWorkerModule.cpp:65–99/622–699在worker处理请求中同步启动RunTest。因此当前 Native worker执行路径在core ticker外，局部公开派发不会递归进入其已执行element。Ticker.cpp:101–102会明确拒绝正在执行element递归；这个证据限定到当前runner路径。若根更换为ticker内直接调用测试，改成标准latent命令让引擎正常派发，不能在该路径硬调用core ticker。

### B. 真实敌人死亡和四旗

1. 玩家公开scene定位到home_entry(101000,25000)地面，Campaign.Tick(.5)实际 RefreshActors。此点到全部当前base出生点小于65000cm；逐registered base要求实际 Actor存在、Target.CampaignTarget、!Protected、Target.Id==Enemy.Id、Generation1、Alive以及真实ground支持。此阶段不手动SpawnCampaignActor、写Enemies或调用可任意传Health的Campaign.Damage。
2. 对每个已注册base Id逐次用实际 `Combat->HitTarget(Target,Gameplay->AttackPower(),TEXT("body"),false,FGuid::NewGuid(),Hero)`；每次 Health必须降低，终态由真实 HitTarget→CommitOpponentHealth→Campaign.Damage→SetCorpse产生。检查实际Cleared增加、稳定 defeat:<Id>:1 RewardFact与实际死亡对象。当前组件Targets遍历真实world所有身份，XP不依赖私有DamageOpponent。
3. Campaign.Tick(.5)自然 Sync/ResolveUntriggered。断言Total==Cleared==80、两Reinforcements cancelled且Flags仍0，证明仅击杀尚未赢。不直接调用State.ResolveUntriggered/RegisterReinforcement作为输入准备。
4. 对实际 `campaign.zones` 的四个Id依次定位到其 `loc_<Id>`附近180cm且仍在260cm真实Nearest范围，Campaign.Tick(.5)保证实际位置已grounded。保存当前Flags和A，公开Interact，断言Busy且Flag未即时增加。Clock.Tick(5) + Campaign.Tick(5)满足真实ActivePlaySeconds Deadline；检查A精确+5、实际Flag新增一次、Action结束。邻区/field当前活目标若实际在bounds内，ZoneOccupied会拒绝，应报告真实前置，不能清除Flag检查。
5. 最后一次正常Campaign.Tick经ReadyForVictory提交真实ReclaimHometown。断言Victory、Phase reclaimed、ReadyForVictory/Validate、Camp.Hometown、两camp Id、Discovered/Activated hometown。检查真实赠与 warehouse_access×1、bed×2、campfire×1在hometown、Paid为空、Level1、四个不同GUID及实际 Building.ResolveFacility。检查唯一共享hearth_blade GUID/UniqueClaim hometown_hearth_blade、RewardFacts reward:hometown且总数1；不手调用ReclaimHometown或GrantItemReward。

### C. 两营地真实唯一人员分配，再原核心保存

1. Nature.EnsureWorld正常为新hometown增加来源，断言Nature.Camps含camp/hometown，存在两营的实际Camp Sources；对各源的Camp/Id/Position做真实支持面检查。resources Key为<Camp>_048_<definition><index>；wildlife SlotId/starterDomestic/fish key全局去重，不要求动物或鱼翻倍。
2. 玩家公开scene定位到已恢复的hometown安全范围；Economy.Advance(0,false)按真实敌人/占地重算Safe/Blocked。公开 AssignWorker(camp_forage,2,epoch)，再 AssignWorker(hometown_forage,2,epoch)，最后 AssignWorker(camp_forage,1,epoch)。检查2仅在hometown_forage、旧camp_forage移除2、1只在首营；home_work由AssignWorker生产记录。保留两个region Enabled=false，不添免费产物。
3. 玩家/弟弟公开scene回到原camp，检查双方SafeToSave/现实沟通范围。原rope最终AssignWorker(worker0)不覆盖人物1/2的两营分配。controller恢复yaw0；paid workbench仍first实际设施Id，材料计数没有战役奖励wood改变。
4. 原ReadMap、7A/W、partial rope、agreement/exchange、active collect、SavePoint(true)照旧。Campaign未要求Claim主线才夺回，避免额外任务材料奖励改变原计数。Victory SavePoint在任何营地保存会把home_saved加入**实际保存**Campaign副本（SaveSubsystem.cpp:281–285），写manifest从Point.World.Campaign读取此Fact。

## manifest / read 的局部新增

write现有Manifest中新增 two_camps=true、campaign_state=S.Campaign、hometown_reward_id来自实际S.StorageItems唯一hearth_blade、campaign_experience来自真实savedGameplay。保存前Parse实际S.Campaign / S.CampEconomy / S.Nature，要求合法连续prologue→occupied→reclaimed事实、Victory与Camp.Hometown一致、两营位置/设施/岗位确有非默认数据。不能manifest自填任何结果字段。

read按flag检查manifest，然后标准Parse `Expected.Campaign`，先验证它与manifest字符串一致。两次实际 LoadPoint后分别增加：

- State的Version/Phase/Legacy/LegacyHometown/Victory；Flags/Facts集合；全部Reinforcements终态；Enemies数量与稳定Id/Group/Zone/Kind/Stage/Home/Located/RefreshDue、Combat.Id/Region/Generation/Health/Position/Rotation/bStunned/Detection/LastKnown/Seen/ReportRemaining/ReportingBody/BroadcastRegions/ArmorDurability/HitRemaining/InvestigationRemaining/Investigation；全部People身份/Location/Position/Located/Stage/Escort及Positions。未实时调用Campaign.Tick来代替恢复或引起前置推进。
- 两个Camp稳定Id/Position、Hometown、人口/救援账本；所有Facility稳定GUID/Kind/Camp/Level/Paid/Paused和实际ResolveFacility；通过Gameplay真实saved buildings数组核对建筑position/rotation及对应actualActor变换。不同床的两个GUID不可合并。
- 两营对应Sources所有稳定Id/Camp/Item/Position/Capacity/Remaining/RefreshMinutes/Due/Blocked；所有Regions的Id/Camp/Job/Facility/Workers/Player/Brother/Enabled/ToRations/Priority/Batch/Completed/BatchStopAt。PlayerEfficiency/BrotherEfficiency和Status为重算运行态，不作为持久字段。人员1/2全域唯一；0仍为原partialrope队列。
- Nature.Camps包含两marker，所有实际来源Key/GUID匹配write，不对全局wildlife/starter/fish增加重复副本。原Pending实例和三处完整库存断言继续运行，验证hearth_blade同一GUID/UniqueClaim仅一份；Gameplay XP/RewardFacts与真实savedGameplay相同。
- 玩家/弟弟、source、Camp真实Transform及view恢复保存值；Survival版本、实际已保存生命/饥饿due/疗程字段按当前结构逐字段比较。两域只追加本场景的实际持久数据，不声称已覆盖真人其他schema代表。

在原public SavePoint noGain/resave之后标准Parse After.Campaign，保持上述业务字段；原CampEconomy/Nature exact比较保留。新保存点不可再次发设施、hearth_blade或XP，四旗/分母/死亡身份/两分配不变。Campaign home_continued在此LoadPoint路径不应新增，明确保留实际菜单continue另验（Restore默认Continued=false）。无需在read再次Start或重新打80人。

## 必须由根实际观察的首个错误来源

当前尚未执行新前置，以下是明确可诊断条件：

- actual travel如果Busy未结束，检查真实StreamingSource completion、Floor、FindTeleportSpot和Loading.IsLoading；8秒墙钟超时直接失败，不能set目的地/Phase或关加载。
- Start的正常opening House/胶囊是否可落地需运行确认；已经有正式House和脚本落点实现，但flat Native几何尚无本轮证据。
- base Actor若因实际spawn碰撞缺失，先报具体registryId/期望ground，不手动补actor或写Health。
- 四旗若未开始/提前中断，核对Nearest、ZoneOccupied、Alive/Safe、真实A Deadline与epoch；不要手填Flags。
- Reclaim若未产生四个gift，报告实际Camp反馈/占地与材料事务，不预加gift。
- root不得把这些fixture前置失败计为Save恢复业务RED；前置成功后才评估跨进程字段与零增益。

## 根建议最窄执行

已内存生成 `restart-two-camps-opt-in.patch`，283行新增、0行删除；根审阅/集成并定向build；fresh write先确认连续进度、保存点与manifest，再完全退出启动同池read。只运行当前filter：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-two-camps-write --pool <fresh-UUID> --extra-arg=-Hearthward071RestartPhase=write --extra-arg=-Hearthward071RestartTwoCamps
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-two-camps-read --pool <same-UUID> --extra-arg=-Hearthward071RestartPhase=read --extra-arg=-Hearthward071RestartTwoCamps
```

正常菜单保存→退出→继续、正式地图导航/两站点public Travel、真人终局、真实模型HTTP/真实UI回调迟到、真实玩家旧档与schema迁移仍为独立gate。普通 public Travel在CampaignWorld.cpp:381–388要求真实Nav.ProjectPointToNavigation；QueryOnly平面不提供Nav，当前用例的公开scene定位不占此信用。

## 依据

生产事实：CampaignWorld.cpp:43–49/61–65/91–149/179–220/354–474；CampaignInteraction.cpp:51–97；CampaignState.cpp:31–79/81–130；GameplayComponent.cpp:69–83/266–277/349–363/376–399；CampSubsystem.cpp:29–45/56–63/286–299；CampState.cpp:79–107；NatureSubsystem.cpp:90–177；LoadingSubsystem.cpp:83–96/116–158；SaveSubsystem.cpp:184–258/281–285。本文仅读取根当前文件，无checksum。

公开流送完成接口复用Epic官方 [UWorldPartitionStreamingSourceComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UWorldPartitionStreamingSourceCo-)；本机UE5.8 WorldPartitionStreamingSourceComponent.cpp:109–115在不存在partition subsystem时返回true，仍由项目真实Ground/FindTeleportSpot检查落点。存档复用既有标准SavePoint/LoadPoint与HWS pool，不新增持久化框架。

## 已交付 patch 的准确边界

`restart-two-camps-opt-in.patch` 只输出到自身QA071。Python内存应用当前root context，确认原源码全部保留且变化仅为insert；原no-phase尾和其后所有测试逐段相同。仅做词法定界符平衡和实际API/字段声明核对，无UE编译、原生运行或 Source写入，状态NOT_RUN。

read完整领域检查采用实际加载后的 Campaign.State.Snapshot / Economy.State.Snapshot / Nature.Describe，与实际diskpoint中对应标准序列化比较，覆盖其全部持久字段；并独立核对非默认Victory/原序章事实、四个实际设施actor、全部建筑身份/位置/yaw、两名跨营分配、唯一reward GUID/XP、双方Survival字段/Transform。没有调用Restore建立预期世界。manifest明确记录实际保存的四gift GUID数组，read检查其实际savedFacility及四个不同身份。

联合ecology在原查找Fire后仅追加一个TwoCamps条件，用Camp=camp与实际Paid wood4/stone4选此次真实付费篝火，避免故乡免费gift误选；未opt-in的原分支保持。联合运行只需在上述两条命令均追加 `--extra-arg=-Hearthward071RestartEcology`。当前生产允许对同一个owned axe再次正常switch，因此原ecology装备前置可以保留。

新 Source用例不计正常菜单、真人、HTTP/真实UI迟到或正式地图NavTravel信用。根提供ecology实际write/read已GREEN，两进程分别0warning；其独立原始结果/review仍由根 `restart-ecology-runtime-review.md`记录，不将该结果挪作两营地信用。
