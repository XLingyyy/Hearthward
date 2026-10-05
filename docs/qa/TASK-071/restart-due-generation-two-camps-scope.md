# TASK-071｜Due、Generation2、双营地的最小扩展范围草稿

2026-10-04。只读根当前设计、QA、Source 与两域实际 manifest；未修改 Source，未执行 UE/build/Git。当前成熟两域 write/read 已实际成功，见 `restart-mature-two-domains-review.md`。以下新增状态及边界全部 NOT_RUN，需根登记准确测试范围后实施。

## 当前业务必须核对项

当前两域用例已实际进入业务：磁盘同池同点、不同 PID、两次 LoadPoint 的新 epoch；非空真实 workbench/worker0/rope Inputs wood2/Work .5/360；非空真实 treasure Pending（bow_2 实例及 ingot8）；完整三处库存、source 库存、命令计数、NPC 事件与时钟；重复加载和重存的零增益。两进程各 0 Error/0 Warning。

仍须保留这些断言。新增 Due/代次/双营地不能使原有非空代表状态被跳过，也不能把带时钟变化的“主动结算”计入“重复加载无推进”的比较。当前测试没有逐字段比较双方 Transform/Survival、所有建筑变换、全部 Campaign 终态；未来场景覆盖这些字段时需直接读取真实最终 SavePoint.World 对应值，而非只根据某个域 JSON 相同推定全系统通过。

最小新增必须关注：

| 阶段 | 必须成立的代表状态 |
| --- | --- |
| write 发布 manifest 前 | 实际来源 Remaining0、Due>W；实际野生槽位 Generation2 及 Current GUID 来自正常刷新；若保存死亡代次，Health0、Rewarded 与非空 Loot、下一 Due>W 同时成立 |
| read 每次 LoadPoint 后 | 来源 Capacity/Remaining/RefreshMinutes/Due/Blocked/Camp/Position；槽位 Id/Definition/Current/Generation/Due；对应动物 GUID/Health/Rewarded/Loot/Position，以及旧尸体身份不变；Camp/Nature Calendar 均等于同一 W |
| 重复加载与重存 | Due 不随墙钟重算、不生成新 Current、不加 Generation、不结算库存、经验或批次，不修改未领取尸体或图箱的奖励账本 |
| 主动设施推进后 | A 保持请求前值；W 只加实际 CommittedMinutes；边界到达只刷新一次；一个现有 rope 批次只结算一次且不再次扣 wood2 |
| 双营地读后 | Victory/Phase/四旗/增援终态与 Camp.Hometown 一致；两 Camp 的稳定 Id/Position；设施 GUID、投入/赠与台账与实际 Actor；人物索引在两营地全部岗位中唯一；Nature 两 camp marker 和来源键保留 |

## 先扩 Due / 非默认 Generation2：同一真实世界，两进程即可

建议在现有显式 write/read 内加一个独立 opt-in ecology 场景，仍用相同 filter、fresh pool 和 stable `/Temp/Task071RestartFixture`。原 no-phase 路径和现有两域默认场景保留。仅候选测试窗口是 `Source/Hearthward/Tests/SaveTests.cpp` 该函数及真实所需 include，QA071；无需生产 API 或持久格式修改。

正常准备顺序应放在现有 workbench 的付费前置通过之后、ReadMap/rope partial batch/active collect 命令之前，避免等待先结算已投入 rope，或拒绝正在执行的伙伴命令：

1. 用现有 Inventory.TryAdd / Storage.Adjust 明示补给 wood4、stone4；真实 SelectBuilding(campfire) → preview → ConfirmPlacement → Clock.Tick(5) 与 Timer.TickComponent 完成真实付费篝火。记录新增设施 GUID、Paid wood4/stone4、实际 Actor。保留工作台选择 GUID，不用“任意第一个设施”寻找新篝火。
2. 从 Nature.EnsureWorld 已生成的野生槽位选实际 hare（health20，generation初始1）。使用生产 `Combat.HitTarget` 与新 Event GUID 经过真实伤害/死亡通路：HitTarget → Gameplay.CommitOpponentHealth → Nature.DamageAnimal，产生 Rewarded/Loot 和 `Slot.Due=W+2880`。输入伤害取项目实际攻击值，重复有限次并断言正常死亡。此为显式 Native 伤害 fixture，不能计作真人挥刀或物理命中验收；不直接调用可传目标 Health 的 DamageAnimal，不写 Health/Rewarded/Generation/Due，不调用 SetCorpse 替代死亡事务。
3. 正常退出战斗冷却：NotifyCombat 实际保留3秒 (`GameplayComponent.h:100`)；用已公开正常 Gameplay.TickComponent / Clock.Tick 的 A 推进处理真实冷却，再断言 InCombat false、双方 SafeToSave。Clock.Tick 自身不会执行 Gameplay.TickComponent，不能只推进 Clock 就假定该冷却已消失。
4. Hero 靠近该付费篝火；Hero/Brother 及所有有 Survival 的实际角色离所选槽位大于8000cm，并检查附近500cm没有建筑或 pen。通过 Camp.WaitAtCampfire(id,480,currentEpoch) / Clock.RequestTimeAdvance 的真实设施请求推进六次480分钟。每次检查请求完成、双方生命/饥饿；需要补给时使用公开库存和 Survival.Eat，食物成本保留。不得直接写 Hunger/Health 或暂停生存系统。确认第一次真正刷新产生 Generation2、新 Current GUID、健康动物和 Due=-1。
5. 再通过同一生产伤害通路杀死真实 Generation2 当前动物，保留非空未领取尸体 Loot 与第三代未来 Due。从已有 herb_patch 选实际来源，Hero 在明确记录的场景定位后执行两次 Nature.Act(harvest) + 真实五秒 Timer，正常消耗 capacity4、得到 herb4，形成资源 Due=W+2880。恢复首营篝火/工作台附近安全站位；不写 Sources.Remaining 或 Due。
6. 之后按现有两域用例 ReadMap、开启一批 rope、留下0<Work<360、创建 agreement 和 active collect，再 SavePoint。从实际最后保存点的 Camp/Nature 标准 Parse 取 source key/slot ID/current GUID/两具尸体与 Due、Generation2，写入同一 QA manifest。原始文件只由 SavePoint / LoadPoint 操作，不自建模拟 HWS 或安装快照。

`Nature.RefreshDue` (`NatureSubsystem.cpp:209–217`) 检查所有 Survival Actor 距槽位80m、建筑/pen 5m，满足后新建 GUID、加一次 Generation；`CampState.RefreshDue` (`215–218`) 到期把真实来源恢复 Capacity 并清 Due。资源 Remaining 属于 Camp.Sources，不在 NaturePoint.Remaining；两套 Due 不可混读。鱼点另有 NaturePoint.Due 和 Successes，当前候选不提供钓鱼序号/鱼 Due 信用。正式野外敌人使用 CampaignEnemy.RefreshDue 和 Combat.Generation，亦属于独立域，本候选不提供其代次信用。

read：在两次 LoadPoint 和 noGain/resave 比较结束后，才执行公开 `Brother.Cancel(Hero)` 正常取消保存的 active collect 命令，再靠近真实篝火请求跨过两条未来 Due。原因是 `WorldClockSubsystem.cpp:133–134` 会拒绝 `Brother.EquipmentBusy()`；`CompanionFixture.h:65` 对当前活动命令返回 busy。Cancel 会产生正常 cancelled 事件，这一主动变化从该步骤单独取基线，不能算作重复加载增益。

等待后应观察 Generation2 的已保存死亡 Current 被恰好一个新 Generation3 Current 替换，旧尸体与 Loot 保留；来源由0恢复Capacity、Due清除。再等待一个有效60分钟请求，活第三代不再次换 GUID/加代次，未采来源不累积资源。rope 只有 wood2 一批，sharedwood0；即使等待较久也只能发 rope1，完成计数只加1、投入不重扣。下一次请求后核对不再额外发 rope。当前方案保留实际请求与生命失败中止行为，不把 bool Accepted 当作完整等待完成。

图箱一次正常 claim / 再次 claim 零增益可在以上 read noGain 和生态结算后另加很小动作块，复用现 Pending 实例 GUID；必须定位到实际 treasure、保证容量并执行真实五秒动作。当前两域记录只证明 Pending 恢复，不包含领取信用。

## 双营地：单独 opt-in 场景，正式夺回事务是前置

现有 Camp.State 只有首营。单独调用 ReclaimHometown、给 Camp.AddCamp 或恢复手填 Victory JSON 会跳过批准的正式夺回过程；SaveGame.cpp:264 还要求 Campaign.Victory 与 Camp.Hometown 相同。不能借用 CampaignEndStateTests 中手填 cleared Health/Flags 的隔离输入来宣称真实战役完成。

最小复用选择是同一隔离池已有正常 Campaign 夺回事务产生的当前保存点，保持原保存来源；当前本次两域 manifest 没有此点。若需新建 Native 前置，就复用真实 Campaign.Start / 序章 Interact / 公开伙伴指令和真实 Travel，走到 occupied 阶段，再让已注册稳定驻军身份经正常伤害事务清除、各区 Interact 连续5A完成占旗，Campaign.Tick 自己 ResolveUntriggered/处理已触发增援并在 ReadyForVictory 后提交夺回。四旗不直接写，80以上身份及增援总数取实际 registry/状态，不硬造80名敌人或绕过生存。

Native 可以明示补给与场景定位，并采用已核对的公开伤害入口缩短准备；这仍只提供实际生产事务与存档信用，正式地图正常战斗/真人夺回要独立记录。禁止静默改成“预先全部 cleared”的场景。

现有地板 x/y 在 ±80000cm；正式 camp(-98000,-75000) 和 hometown(109000,45000) 均需要扩展支持。可先把同一 tagged QueryOnly plane 候选 extent 扩为 `(200000,160000,10)`，覆盖两营地、四控制区及现有64000cm资源偏移上限；准确作用范围必须按运行时生成点 Ground/occupancy 逐项断言，保留实际文件/package身份。该支持平面为 Native 场景夹具，不能替代正式地图地形验收。

还需在 Campaign.Start 之前把真实玩家/伙伴/CampActor 的 fixture 初始站位按正式首营锚点准备，正常 EnableAdventure 建立正确首营；不能先建立原点首营，再让正式 Travel 去 -98000 造成营地/激活位置不一致。新世界仍使用标准生命周期与实际胶囊自然落地；不改 MovementMode。

正式事务提交后检查：Victory true、Phase reclaimed、两 camp marker、hometown travel 激活、赠与 warehouse_access/bed2/campfire 的实际设施和 GUID、唯一 hearth_blade 奖励事实。之后 Nature.EnsureWorld 正常扩展新营地资源。现野生 SlotId、starter domestic、鱼点键是全局去重，资源 Key 按 Camp 前缀区分；不能断言所有动物/鱼按两营地翻倍。

只选一个实际人物索引在正常 AssignWorker 中从首营区域移到 hometown 区域，然后另一个人物留首营，生成真实唯一跨营分配。write 保存两营地与岗位/设施/任务终态；read 两次 LoadPoint 逐稳定 ID 比较，并要求工作台/赠与 Actor 重建、同一人物不在两个岗位重复、同一奖励实例 GUID 与事实只出现一次。Victory SavePoint 在营地添加 home_saved 属于已有生产行为；当前 LoadPoint 默认 Restore(S.Campaign) 不带 Continued。标题 continue 的 home_continued 是另一条真实菜单路径，不能在本 Native read 中伪造或借该断言代验。

## 建议施工与运行顺序

1. 先完成已成功两域 QA 更新，不重复当前已通过、无新增影响的验证。
2. ecology opt-in 的 Source 测试增量：真实 paid campfire、真实资源耗尽与 Generation2、两个 Load noGain、取消命令后真实设施跨 Due。根定向 build + fresh write →完全退出→fresh read。
3. 双营地前置独立落地；优先复用真实批准夺回进度，缺时才准备完整正常 Campaign Native 事务场景。登记相同 SaveTests 内准确窗口，保证旧场景仍原样运行；根实际确认准备成功后再运行同池跨进程。
4. 正式地图菜单、真实异步 HTTP/UI 跨加载、其他快照代表和真实旧档迁移保留独立门槛，不用新增 Native 两域结果消除它们。

参考官方已有二进制序列化/加载机制：[Saving and Loading Your Game](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US)。本项目继续使用既有标准序列化的 HWS pool 与公共 SavePoint/LoadPoint，不新增存档框架。
