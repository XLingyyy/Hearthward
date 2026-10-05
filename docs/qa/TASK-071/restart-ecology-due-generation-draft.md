# TASK-071｜生态 Due / Generation2 增量草稿

2026-10-04。基于根当前已通过的 SaveTests 稳定两域夹具生成。仅 QA071 中 patch 与本文，未写 root/Source，未运行 UE/build/Git。新增用例 **NOT_RUN**，现有成熟两域实际结果见 `restart-mature-two-domains-review.md`。

## 补丁边界

`restart-ecology-due-generation.patch` 190行新增、0行删除：3个实际所需 include；现有 ActualLoadPointInvalidatesOldCommandTicket 函数内增加 opt-in `-Hearthward071RestartEcology`。需要显式 write/read phase；原 no-phase 末段及其后全部测试保持字节一致。原两域代码与断言未删除；未带 ecology flag 的 write/read 不执行新增构造或结算，不要求旧 manifest 有 ecology 字段。

无新增类、生产接口、持久字段或依赖。局部两个 lambda 只负责明示 Native 场景定位/真实 paid request，另一 write 局部 lambda 调用既有生产伤害事务。

## 实际 API 与准备时序

1. 在原 paid workbench 前置通过后，公开库存补给 wood4/stone4、双方 wild_food 各12份。转控制朝向180度，在工作台相反足迹正常 SelectBuilding(campfire)、preview、ConfirmPlacement、Clock5A/Timer完成；检查实际 Paid wood4/stone4、设施可用和材料扣除。
2. Gameplay.AttackPower 未装备时为0；公开 EquipInstance(真实axeGUID)，通过 Clock .4A + Combat.TickComponent 完成生产换装动作，确认真实 weapon GUID/AttackPower。未直接写 Equipment 或 Durability。
3. 从 EnsureWorld 实际 hare slot 获取 generation1 Current，生产 HitTarget 使用实际斧攻击力/独立事件GUID杀死。正常 DamageAnimal 事务创建未领取肉/皮账本和 W+2880 Due；没有直接设 Health、Rewarded、Loot、Due 或 generation。
4. Clock3A + Gameplay.TickComponent3秒结束真实 NotifyCombat 冷却；断言实际 InCombat false。双方离槽位超过80m的条件明示检查。
5. 使用真实 paid campfire 的 RequestTimeAdvance 6次480W。每次请求有真实 Facility/Campaign/Epoch/OperationId/StartW；检查 Accepted、Completed、Completed状态、CommittedMinutes及准确W变化，A不增加。调用正常 Nature.Tick(.5)恢复实体并观察实际 generation2新Current；不能用 Accepted 单独声称等待完成。
6. 正常再次杀死真实 generation2并冷却。Hero 场景位置由实际 tagged地面查询、实际胶囊半高和 public SetActorLocation准备，再通过真实 Movement.TickComponent贴地。Nature.Act(harvest) 两次、每次实际 Clock5A/Timer完成，耗尽真实 herb capacity4并得到herb4，建立第二条未来Due；然后回到首营安全地面。
7. 原 ReadMap / rope partial .5/360 / agreement / active collect / SavePoint 照常执行。manifest身份与experience取实际写点标准Parse；保存的 CampEconomy/Nature 字符串包含真实 Due、Current、Generation2及两具未领取尸体。A/W自然分离并使用原有实际数据绑定断言。

八小时原地等待的饱食基础消耗 `480*100/2880 ≈ 16.67`。wild_food真实食物值20；每次等待前 Hunger<80时用 Survival.Eat消费实际物品，保证双方存活与安全。各12份共6kg，准备/读取阶段都保留真实剩余物品及消耗；未赋值 Hunger/Health、未暂停生存，真实中止回执会令前置失败。

## read 观察与主动跨界

首先按原两域路线做两次 public LoadPoint、完整库存/Clock/命令/NPC对比和 resave noGain。新增比较包含 actual source capacity/remaining/refresh/Due/blocked/camp/position、actual slot id/current/generation2/Due/definition/position、两具实际尸体health/rewarded/loot与Actor、paid usable fire和experience。每次资源及动物 Due>保存W前置必须真实成立。

完成 noGain/resave 后才 public Brother.Cancel(Hero)，确认当前 active collect 结束。此取消是明确的正常后续操作，可能产生 cancelled事件，不计作加载增益。现时钟会拒绝 Brother.EquipmentBusy，所以不能跳过此动作。

随后6次实际480W设施请求跨过两条保存Due，观察恰好一个 generation3新Current、健康未奖励新动物、resource由0恢复Capacity/Due=-1，以及原已投入 rope批次仅Completed+1/rope+1、sharedwood仍0。再等待实际60W，活代次/Current、未采库存、rope完成和输出均不能增加；两具旧尸体未领取Loot保留，刷新不重复发hunt经验。

未调用 Clock.Install/AdvanceCalendar，未修改移动模式、结果快照、source计数或 due；未用模拟HWS替代 SavePoint/LoadPoint。新增 Native 伤害入口及场景定位明确属于显式fixture，未验证挥刀物理命中或正常键鼠路线。鱼Due/Successes、正式敌人代次、双营地长链及正式菜单/HTTP/UI迟到回调未纳入本补丁。

## 静态核对与根运行

已核对真实 public 方法/字段与UE TickComponent签名；增量不包含直接写 Health/Hunger/Current/Generation/Due/Remaining/Rewarded 或 Clock.Install/SetMovementMode/SetCorpse/DamageAnimal调用。原 no-phase tail/后续测试在内存生成草稿时逐段一致。尚未编译，以上核对不提供运行通过信用。

根在自己的集成树审补丁并构建后，使用一个新UUID，write完全退出再read（两阶段同flag/同pool）：

```powershell
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-ecology-write --pool <fresh-UUID> --extra-arg=-Hearthward071RestartPhase=write --extra-arg=-Hearthward071RestartEcology
& G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-053/run_native.py --filter Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket --label restart071-ecology-read --pool <same-UUID> --extra-arg=-Hearthward071RestartPhase=read --extra-arg=-Hearthward071RestartEcology
```

runner 的 extra-arg 已核对为 action=append。根应首先识别新前置是否成立，再区分保存、恢复和主动结算业务结果；本轮实际未运行。

官方已有原生测试与组件调度参考：[Automation Test Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine)，[UCharacterMovementComponent::TickComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/TickComponent)。项目 API 与生存/设施条件依据根当前实际源码。
