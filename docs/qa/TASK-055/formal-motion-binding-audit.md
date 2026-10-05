# TASK-055 正式动作／武器绑定只读准备

2026-10-04。基于根task051当前源码、已批准055、既有031及055原始QA。仅准备下一步；未启动UE、导入／修改资产、改生产或执行构建。当前人体碰撞的进一步修整由根及其他指定Agent负责，本报告不把那部分静态／原生结果当作正式近战表现验收。

## 当前正常路径与绑定事实

- `HearthwardHeroAnimInstance.cpp:84` 仍只读取 `/Game/Characters/Hero/AnimationV2/A_Hero_{Idle,Walk,Run,JumpStart,Fall,Land,Attack,Dig}`。PlayCombat:147只接Duration／Execution，把同一个Attack整段按总时长缩放；Execution只是MotionState名称变化，仍播同一clip。NativeUpdateAnimation没有读取Combat.Action／Guarding／Aiming、Traversal水中／翻越状态或Survival倒地状态来选择专用片段；死亡回Idle。
- `HearthwardBrotherAnimInstance.cpp:75` 仍只读取 `/Game/Characters/Brother/Animation/A_Brother_{Idle,Walk,Run,Dig,Attack,Wait}`。PlayAttack完整播放旧Attack，工作Running统一显示Dig；新055片段未接入。正式角色是CompanionFixture，其构造器:110绑定该Native AnimInstance。
- CombatComponent::Attack:70沿现FMove支付耐力、生成当前ActionId／ActionEpoch、捕获装备GUID，再播放旧Attack。BeginExecution:105播同一Attack三秒、Finish:254在合法终点结算。Dodge:142、SetGuard:151、Damage:159、Reload:478、Shoot:483、Tick:287的throw／crossbow时点没有专用动作驱动。Action、Elapsed、Duration、Aiming、CrossbowLoaded、RangedSelected、Guarding已提供真实读取接口，无需再建第二套动作状态或时钟。
- 实际石斧item为axe，combatClass=blunt，attack30、durability80；技能、重击倍率、严重饥饿及装备GUID仍由Gameplay／Combat现链计算。items表有combatClass／攻击／弹道字段，未找到正式动画片段、握点、weapon trace端点绑定。Source没有项目自定义AnimNotify／NotifyState生产类或消费链；当前QA没有导出资产Notify列表，不能据此宣称所有uasset的notify数为零。
- 现有真实持握模型仅 `/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe`。Character:95—103挂hand_r，rotation(0,0,-90)、absolute scale .7、无碰撞；RefreshHeldTool:146按definition=axe和数量显示，未按本人的EquippedInstance／耐久／RangedSelected选可见物。Brother没有对应持握模型链。具体其他十项来源与无盾／腿甲的证据复用equipment-animation-gap-audit.md及equipment-source-geometry.json，不重复导入。
- Sweep:222按FMove有效窗口作角色中心±55°扇形／枪直线采样；未读取真实武器刃端。LaunchProjectile:505的出射点仍是角色中心+Z30+Dir45，非弓弩muzzle。既有数值／去重／GUID／取消链必须保留，表现绑定只消费这些事实。

## 十二个新source clips：真实范围与可用局部片段

直接只读 `G:/GameFactory/test_data/outputs/hearthward/20261003_task055/assets/motion/knight61/knight61.fbx`，5,681,420 B，BinaryFBX7400。AnimationStack的LocalStart／LocalStop及ReferenceStart／Stop均从0开始且一致；Global TimeMode11、CustomFrameRate24、UnitScaleFactor100。下面“源范围”来自原始FBX，整数为24fps间隔数，不能把它冒称全部已由UE逐帧审阅。

Hero／Brother各12个AnimSequence的准确统一路径为：

`/Game/Hearthward/Assets/TASK-055/Motion/Hero/knight61_SK_Hero_Skeleton_Anim<name>`

`/Game/Hearthward/Assets/TASK-055/Motion/Brother/knight61_SK_Brother_Skeleton_Anim<name>`

逐项来源及导入结果在根docs/qa/TASK-055/motion-import-hero.json／motion-import-brother.json；新源归档记录为art_source/TASK-055/motion/knight61_motion_source.json，来自F:/Download/medieval+knight+3d+model (2).zip的tripo_convert_2e499319-8746-4ade-b2b7-972d92793aca.fbx。许可未验仍归074。

| name | 源局部范围／24fps间隔 | 已有可用局部片段证据及最窄用途 | 实际事件／notify还需确认 |
| --- | --- | --- | --- |
| idle | 0—15.333333s／368 | 只有整段候选；现正式Idle保留。 | 无战斗结算事件；循环接缝需确认，不能为了数量新增notify。 |
| walk | 0—2.333333s／56 | 新整段候选；031当前Walk已有in-place修复，先保留正式版本。 | 确认新段pelvis净位移与脚接地后才取循环；脚步声点属于表现。 |
| run | 0—1.25s／30 | 新整段候选；031当前Run已有位移及初始偏移修复，先保留。 | 同walk；不得双重应用动画根运动和胶囊位移。 |
| chop | 0—6.583333s／158 | 031当前角色自身Chop有1.333333—2.25s攻击裁切证据；只能作为新源的复核候选，不能自动当作055同名段已验证。 | 对真实石斧先取一次准备／下劈接触／恢复；确定刃轨迹后对齐现FMove，并登记可取消点。 |
| dig | 0—16.375s／393 | 整段候选，未登记一次铲／斧工作周期；现正式Dig保留。 | 现TimedAction拥有一次真实结算，不允许每次工具接触重复结算。需实际工具类型／工作周期。 |
| wait | 0—6.0s／144 | 整段候选；现Brother Wait已有正常状态消费。 | 无伤害事件；循环／退出接缝尚未确认。 |
| jump | 0—2.208333s／53 | 031旧Jump实测apex0.708333s，并派生JumpStart约0.3s、Fall约0.4s、Land约0.2s。新源没有这些局部窗验证。 | 起跳／落地仍由真实Movement状态决定，脚接触事件只同步表现，不再加位移／成本。 |
| climb | 0—3.458333s／83 | 整段候选；源名不能证明051翻越可用，Brother没有新增攀爬能力。 | 需实际障碍高度、手脚接触与Traversal 1s时序（.3上升／.5越过／.2下降）复核；不把全段压缩当成功。 |
| slash | 0—6.583333s／158 | 根实际Hero／Brother单节点pose QA通过，未取得一次有效斩击的局部窗。 | 刃接触起止、一次挥击方向、返回握持、可取消点；不能共用整段6.58秒压缩给刀／枪／斧。 |
| hit_to_side | 0—1.25s／30 | 根两角色pose QA通过；只读信息不足以指定受击0.25s或失衡阶段的局部截点。 | 需要反应开始／恢复姿态，Damage已接受后才播；不从notify再扣HP或护甲。 |
| fall | 0—3.0s／72 | 根pose QA通过；当前空中Fall来自旧Jump的apex姿态，与新fall用途未建立对应。 | 先实际查看是否倒地／倒下，再绑定Survival Downed／Dead；不能按名字换空中Fall。无伤害notify。 |
| swim | 0—5.708333s／137 | 根pose QA通过，未登记平移／浮力、完整循环或正式水面姿态。 | 由Traversal.IsInWater和Movement速度消费；水中位移仍由PhysSwimming负责，划水事件不加一套消耗。 |

只有旧031的Chop攻击窗和Jump拆分具有实际局部片段证据。新055四段pose QA证明加载、同骨架和关节确实改变，覆盖不是完整语义窗／打击点／接触／取消验收。弓draw/release、弩reload/fire、盾／武器格挡、翻滚、失衡、执行／击晕、无耐力尚无已验专用片段；十二个源clip不能自动覆盖全部操作。

## 现有规则时间与片段对齐约束

| combatClass | 轻击准备／有效／恢复（总时长） | 重击准备／有效／恢复（总时长） |
| --- | --- | --- |
| shortblade（Move默认） | .18/.12/.30（.60s） | .40/.16/.44（1.00s） |
| longblade | .28/.18/.44（.90s） | .55/.22/.63（1.40s） |
| spear | .25/.12/.38（.75s） | .50/.15/.55（1.20s） |
| blunt（石斧） | .35/.18/.47（1.00s） | .65/.20/.65（1.50s） |

这些来自CombatRules::Move，继续作为唯一基准，不添加第二张伤害时间／费用表。现有其他真实时点：翻滚总.55s／前.3s无敌；弓至少.2s才释放、到1s蓄满、release立即支付／生成后recoil.35s；弩reload1.8s终点装填，fire总.5s／.1s生成；throw总.8s／.25s生成；执行3s终点结算，伤害接受后hit.25s。

031 finalize_motion.py先找前半段右手最高点peak，再以peak-.5s至随后最低点裁Attack。已记录start1.333333、end2.25，因此按该算法推得peak1.833333，位于裁段的54.545%。PlayCombat整段缩放到石斧轻击1s，峰值约在.545s；现有效段.35—.53s在该峰值之前结束。这是既有脚本／结果的时间推导，未把最高点直接称为刃命中点，但足以说明总时长匹配没有证明下劈与伤害窗口匹配。

[Epic Animation Notifies](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine) 区分单点事件和带Begin/Tick/End的Notify State。当前Native SequencePlayer图可继续复用；没有仅为notify切换完整Montage体系的需求。若新增事件，输入仅为当前ActionId／epoch及表现标记；接受／去重／取消和FMove窗口仍由Combat负责，不从notify重新计算数值。低帧跨窗需要真实源时间的武器端点采样，不能只在同一当前姿态重复扫中心扇形。

## 最小下一步，待根登记及实际资产QA

1. 先闭合已经有真实模型和旧攻击裁段的石斧：在当前石斧Mesh确认柄握点、刃根／刃端、hand_r实际世界变换与一次挥击源窗，按准备／有效／恢复分别对齐FMove。最小准确生产候选为Character::RefreshHeldTool／held组件、HeroAnimInstance::PlayCombat与NativeUpdateAnimation／PreUpdate，以及CombatComponent::Attack／Sweep／Cancel；不写武器通用框架，也不顺手改其他动作的数值。
2. 显示／轨迹只读取本人EquippedInstance及实际Definition／Durability，ranged选择沿Combat现状态，装备转交／破损／读档广播沿现链。真实刃段扫掠仍保留ActionInstance、ActionEpoch、HitIds、ActionId和ChargedWear；墙阻挡使用现Visibility channel。不能以新增刃端模型展示替代真正轨迹碰撞。
3. 用最窄实际角色／Mesh回归证明窗口外零伤害、真实刃段接触只结算一次、墙阻挡、取消／旧epoch关闭，以及GUID武器耐久只扣本人实例。原CombatTests已有1/30/120fps中心扫掠、去重／消耗／处决及取消覆盖，继续复用；新增只针对真实模型轨迹，不能把旧测试名称的“Real sweep”当作新刃端已验。
4. 一套正常近战和远程、格挡耗尽、破损、转交／付费维修／保存恢复仍由根跑正式地图正常输入。先完成已有石斧链后，按各武器真实握点与专用源窗扩展；弓弩、盾、腿甲和缺动作另列准确资产，缺项继续保留未验。055不修改070风格方向、074许可状态或真人1v3的未执行记录。
