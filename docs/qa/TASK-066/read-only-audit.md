# TASK-066 只读审计与定向 RED 入口

日期：2026-10-04。Owner / Reviewer：XLingyyy。无 Issue。

审计基线为根集成树 `.agent-local/task051` 当前源码；沿用已批准 `docs/planning/TASK-053-074/TASK-066.md`、049 任务卡和 runtime `Resources/Data/gameplay.json`。本子代理未运行 UE、构建、修改资产、提交 Git或推进任务状态。根先授权在已登记 `Tests/CampaignAIBehaviorTests.cpp` 追加一个定向测试；取得真实 RED 后，再授权修改 `Combat/HearthwardCombatComponent.cpp` 的两个尸体谓词并提供相对根当前源码的增量补丁。真人录音仍暂缓，不影响本次工程调查。

## 已确认的生产缺口

### 保护人物被尸体逻辑接收

代码事实：

- `Campaign/HearthwardCampaignActor.cpp:39` 的 `Initialize(Id,false)` 设置 `Target->Protected=true`。无敌军记录时不会创建敌军生命值；`CombatTargetComponent.h:36` 的默认 `Health=0` 保持不变。人物仍是正常站立／行走的非战斗实例。
- `Combat/HearthwardCombatComponent.cpp:405` 的尸体候选只检查 `Health<=0`，随后做视距、正面及可见性检查，未排除 `Protected`。它可将真实族人记入 `ReportingBody`；完成 3 秒报告后写入该人物的 `BroadcastRegions` 和区域 `Alarms`。
- `Campaign/HearthwardCampaignWorld.cpp:447` 先终结未触发增援，再消费实际 `Alarms`。若对应工坊／议场基础敌军仍存活，误报也会使真实一次性增援状态从 pending 进入 spawned。
- `Combat/HearthwardCombatComponent.cpp:438` 的搬运候选同样只检查 `Health<=0`。`HearthwardCharacter.cpp:389` 先调用 `Combat->Carry`，`:405` 才进入 `Campaign->Interact`。真实族人处于 150 cm 内时，正常 E 入口可优先开始 pickup，覆盖救援／交付交互。
- Sense、Lock、伤害、处决使用 `Alive()`；其返回条件已经包含 `!Protected`。这些路径无需因本缺口改写。

最小生产候选：仅 `Combat/HearthwardCombatComponent.cpp` 的 Perception 尸体候选和 Carry 尸体候选增加 `!T->Protected`。保留已批准的非战斗人物初始化、真实敌尸、现有 3 秒报警及 120 秒报警有效期。无需改变生命值定义、Targets 总注册表、保存结构、依赖或公共 API。

定向测试与根独占 UE 的实际结果：

- 注册名：`Hearthward.Campaign066.ProtectedResidentCannotActAsCorpse`。
- 文件：`Source/Hearthward/Tests/CampaignAIBehaviorTests.cpp`；增量补丁：`protected-resident-red.patch`。
- 复用既有 `FScopedCampaignAIWorld`，真实 `Initialize("protected_01",false)`，活守军处于其前方 6 m，玩家保持 45 m 距离；不直接设置 Protected、Health 或 Alarm。
- 真实 `Clock->Tick` 和 production `Combat->TickComponent` 按 `.01 + 2.99 + .011` 推进。保护人物不应有已报告区域、区域报警或待报告 ID。
- 同一注册中的第二个隔离场景，真实初始化 `field_slice_02` 后经 production `HitTarget` 造成死亡；检查 3 秒前无报警，3 秒后实际敌尸广播区域与报警存在，期限为当前 ActivePlaySeconds +120。
- 保护场景末尾将玩家放在人物 130 cm 内并设正常 Walking，直接调用 production `Carry(false)`，断言拒绝且 `Carrier` 无效。错误 pickup 用 production Cancel 清理；Cancel 的 pickup 分支内部完成 DropBody。
- 这保护了两个真实谓词缺陷。它尚未验证桌面 E 输入、自然地图 4 人增援实例化或完整救援路程。
- 静态补丁检查：仅新增一个注册和两个已有模块 include；`git diff --check` 通过；保留根文件实际 LF 行尾。
- 根审读并整合了此测试；首次编译发现原夹具显式调用的 DropBody 是 private。删除该多余调用，依赖已有 Cancel 的 pickup 分支后，根构建 PASS。
- 根实际运行 `diagnose055071-baseline066-compact068`，本注册状态 Fail，精确4个错误断言：保护人物有已广播区域、保护人物写入尸体报警、Carry(false) 返回 true、Carrier 有效。`.01 + 2.99 + .011` 的时钟前置、远处玩家、真实敌尸的3秒报警及120秒有效期正向对照未报失败。没有直接设置报警或伪造死亡替代 production 输入。
- 实际结果见 [根 Native 汇总](../TASK-053/diagnose055071-baseline066-compact068-native.json)；原始报告为根 `Saved/Task053/diagnose055071-baseline066-compact068/index.json`。本注册原始 errors=4、warnings=2，两个 warnings 来自隔离导航世界的 CrowdManager；业务断言与前置均实际执行。
- 已交付 `protected-corpse-production.patch`：仅 Perception 尸体候选和 Carry 尸体候选各增加一个 `!Protected` 判定，保留根当前055等其余源码与实际 LF。局部 `git diff --check` 通过。根后续整合、构建与同一注册 GREEN 仍待记录。

### 第二营地的职责人物交互入口不足

已批准 S05 与 runtime steps 明确要求“任一营地与猎人职责 NPC 核对”；S13 任务卡明确“两营地同一收件者入口”。当前 `camp_hunter`、`civilian_initial_01` 的 location 坐标固定在第一营地。`CampaignInteraction::Nearest` 对 location 使用 `Position(Id)`，仅 10 名救援者额外按 Actor 位置参加候选。

`CampaignWorld::RefreshActors` 可按 hometown 岗位把 `civilian_initial_01` 实例呈现在第二营地。实际人物位置与交付 location 仍可分离。因此在第二营地实际收件者旁，当前静态候选链无法提供 S13 交付；猎人的第二营地核对入口也未见实现。第一营地入口仍保留，全部任务没有因此永久锁死。

此项目前为设计与静态实现差异，尚未取得自然地图操作 RED。最窄正常复现：未交付 family_letter 的真实档案，在故乡已开放后将初始族人 01 正常分配至 hometown 岗位；通过正常操作接近实际人物，按 E 并观察 delivered_letter；再以未 hunter_confirmed 的已学图纸档案检查故乡猎人入口。不能直接设 quest Fact 代替这些动作。

若根确认要实现“任一营地”入口，候选为 `Campaign/HearthwardCampaignInteraction.cpp` 的真实人物候选／交付身份，以及 `Campaign/HearthwardCampaignWorld.cpp` 的职责人物呈现；确需新增位置时再登记 `Resources/Data/gameplay.json` 的精确数据范围。当前未扩写此项测试或生产，避免按静态推断提前扩大范围。

## 重点边界的已确认实现

| 边界 | 当前证据与结论 |
| --- | --- |
| 提前图纸学习 | S05 条件是 hunter_record + hunter_confirmed。真实猎人交互做 `KnownRecipes.Add(craft_bow_rare)` 并登记确认事实，未要求“本次新学会”。钓鱼已学会时仍能追认。缺少专门的先学后任务原生样本；静态链未见锁死。 |
| 提前开箱 | 四 loot_* 交互直接记录持久 Fact，未按 QuestAvailable 锁世界行为。S12 开箱仅记 workshop_cache，金属锭由 Claim 的奖励事务统一发放。重复 E 是幂等 Fact，不重复给锭。缺少开箱早于实际 forge 访问并跨保存／重开的针对性样本。 |
| 主线前置 | Available 检查父任务 Claimed，Conditions 仍读取已存在事实。日志展开与世界动作分开；先做后续真实动作可追认。Legacy main_01 单独跳过，旧 Demo 11 项配置保留独立入口。 |
| 唯一任务奖 | CampaignQuests::Claim 同时拒绝 Claimed 与 RewardFacts 的已有 ID。交付输入与奖励输出先 CanAdjust，XP + Store.Adjust 失败时回滚 XP / 事实。交付 Fact 只在事务成功后加入。 |
| 10 个 RescueId | 初始化从同一 23 项配置登记 rescued_01..10。Validate 检查十个精确身份，SaveGame::Validate 同时要求 arrived 与 Camp.Rescued 一致。World Tick 通过实际人物 CampAt、安全／危险链才调用 RecordRescue；任务领取不造人口。 |
| 营救奖励 | Camp::RecordRescue 先复制并做唯一 Rescue，再登记 rescue:<id> 的 600 XP；第五名沿 reward:rescue5 发唯一弓。重复 Rescue 拒绝；既有049后日谈样本已经验证人口30和重复拒绝。 |
| 四保护人物 | World 单独生成 protected_01..04，未进入 Enemies / Rescued。Alive() 排除，故其不计 K/N、不进 Sense / Lock、不受 HitTarget。尸体两个谓词遗漏是本次明确缺口。 |
| 一次增援 | State::RegisterReinforcement 只接受 pending、occupied、未胜利；固定四 ID 一次加入后设 spawned。先 ResolveUntriggered，因此基础敌军全清后同帧报警不能复活已取消触发器。 |
| 旧报警重复 | 已报告真实敌尸记 BroadcastRegions；CombatTests 已有同尸不刷新报警、受到正伤害中断、持续交战保留报警、脱战后过期的生产回归。无需重写同样的测试。 |
| 保存与再开 | Campaign 快照保留逐人阶段、敌军 Generation、奖励相关 Fact、旗帜和两增援终态；Gameplay 保存 Claimed / RewardFacts / KnownRecipes，Camp 保存 Rescued。全档校验检查 Campaign Victory 与 Camp Hometown 一致。M08 明确允许在任一营地保存，当前 home_saved 的 CampAt 规则符合该步骤。 |

## 23 项正常条件与已有证据入口

任务唯一数据仍在 runtime campaign 表；本表仅映射现有条件来源，不复制任务配置。

| ID | 必须经实际操作取得的条件 |
| --- | --- |
| main_01 | 遗物领取、有效弟弟指令、实际撤离旅行完成。 |
| main_02 | Nature 木石 harvest、真实工作台、真实岗位分配、有效弟弟安排。 |
| main_03 | 实际发现近郊、人物接触、rescued_01 实际报到。 |
| main_04 | 真实工作台与已报到人物、Camp 主动付材升阶、打开升阶入口。 |
| main_05 | 实际激活渡口、望点交互、发现故乡入口。 |
| main_06 | 任意两区真实清敌并完成各 5 秒占旗。第一旗同时证明该区清敌，条件重复读取旗事实符合步骤依赖。 |
| main_07 | 所有实际驻军／已登记增援被清除、两增援终态、四旗、实际永久胜利事务。 |
| main_08 | 故乡实际共享仓储入口、故乡生产设施或岗位、任一营地实际保存并标题继续同世界。 |
| side_01 | 实际接触并报到 rescued_02、03，再领取任务奖。 |
| side_02 | 实际接触并报到 rescued_04、05，再领取任务奖。 |
| side_03 | 实际接触并报到 rescued_06、07，再领取任务奖。 |
| side_04 | 实际接触并报到 rescued_08、09、10，再领取任务奖。 |
| side_05 | 住区匣记录、猎人核对；已学图纸仍可完成。 |
| side_06 | 发现真实矿点或实际采矿、营地显式交付 6 ore。 |
| side_07 | 实际发现并明确激活渡口。 |
| side_08 | 实际发现并交互山脊标记。 |
| side_09 | Nature 实际 herb harvest、营地显式交付 6 herb。 |
| side_10 | 河门匣事实、营地纪念入口交互。 |
| side_11 | Nature 实际播种 grain、成熟后实际 harvest；持有 grain 不计。 |
| side_12 | 工坊匣事实、真实可用 forge 目录访问；任务统一发 4 ingot。 |
| side_13 | 住区信件事实、初始族人 01 的实际交付入口。 |
| side_14 | 议场记录事实、营地记录台实际抄录入口。 |
| side_15 | Camp 实际捐粮至少10点、实际公共口粮用餐成功。 |

049 已有证据应复用，但必须准确限制证明范围：

- `Tests/CampaignTests.cpp` 的 RegistryAndVictory、ContentContract、Schema7Migration 覆盖 23 条配置、10 个身份、80 /84 /88 分母、增援一次性／清基军后 cancel、快照与旧档迁移；固定死敌 / 旗帜是状态夹具，不能视作正常通关。
- `docs/qa/TASK-049/final-fixed-native.json` 留存049当时 10/10 Campaign + Save 结果；没有据此声明当前批次全测试已经运行。
- `final-fixed-pie.json` 实际清敌执行、旗、交付等使用 production API，但 `method` 明示 supply、scene relocation、garrison health fixture；其最后标题继续失败。之后 `final-continue-pie.json` 纠正该步骤。不可把前一个结果单独标为整套成功。
- `final-epilogue2-pie.json` 的 84 项结果为成功，包括十人报到、人口30、唯一弓、重复营救无 XP、四营救支线、真实 forge／播种／收获／公共餐、最终23项领取和保存。脚本真实调用交互／营地事务，同时明示物资供应和救援 camp-edge relocation；未证明全程正常带路返回。
- `verify_campaign_world.py:124` / `:134` 明确直设敌生命用于区域转换夹具；`verify_campaign_epilogue.py` 的接回在营地边缘缩短路线。当前需保留这些事实，不能将其转换成066正常流程 PASS。
- `route-full-follow2-pie.json` 已记录4953.43m真实玩家／弟弟行走，使用 QA ×3 时钟；可复用路线证据。它不包含十个救援人物的完整返回路程。
- 当前根已集成067旗主动时钟、失败拒绝胜利、第二营地激活、进度提示与合法赠礼落点修复。066不重新列这些旧缺陷，也不重复其回归。

最窄后续验证建议：已取得本次一个066测试的真实 RED，下一步只修两个尸体谓词并运行同一注册及既有尸体报警用例。提前知识／箱事实采用一条 production 交互链保护早做、保存／再开、领奖一次；十人接回复用049脚本的身份／奖励断言，新的正常路线证据需由实际输入和行走取得，不能通过直接 RecordRescue 或设完成 Fact 代替。已有快照／分母／同尸报警样本通过后无需重复扩展同类状态组合。

## 外部工程参考

[Epic UE5.8 AI Perception](https://dev.epicgames.com/documentation/unreal-engine/ai-perception-in-unreal-engine?lang=en-US) 将感知对象与刺激来源作为明确输入，并支持按类别／归属选择感知目标。此处借鉴目标筛选边界，继续保留项目现有 deterministic Perception 与稳定人物身份；现代码遗漏的 Protected 判定应在尸体消费入口补全。

[Epic Saving and Loading Your Game](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US) 说明以现有 SaveGame 保存跨会话状态。049已有完整快照／账本／迁移机制，本次没有引入第二套任务保存框架。

上述文档提供工程原则。项目内已确认缺陷、任务语义及旧运行证据来自当前生产代码、已批准049/066文档和上述仓库 QA；本文没有把官方示例当作本项目运行结果。
