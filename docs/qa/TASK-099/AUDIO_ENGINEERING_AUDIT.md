# TASK-099 工程剩余与真实事件证据

2026-10-07 工程审计。真实04轮 Editor build PASS，Task099 12项7P/5F：原7项现0Warning，Progress两项确认消费者缺口，Combat三项确认声明阶段未发布。随后progress与combat消费者及3条真实consumer夹具已实现/静态检查并冻结，尚未GREEN。四个实际脚步序列的UE只读metadata均READ且0 Notify，脚步生产入口缺口已确认。

|项目|已确认生产入口与问题|最小可授权窗口/下一步|音源与确认边界|
|---|---|---|---|
|挥动|Combat.Start生成ActionId/epoch；Sweep实际active窗口处理。石斧先核真实持械、BladeBase/BladeTip、hand_r与clip，缺任一会Cancel。按钮按下/Start不足以证明实际挥动|Combat .h/.cpp在真实active几何校验后发布每ActionId一次的正式事件；Presentation消费；真实取消/快速不同动作夹具|原Kenney包knifeSlice/knifeSlice2可作候选，CC0确定；声音语义/听感待试听，不借仓储WAV|
|真实命中|HitTarget的DamageIds在229先登记，235–237才实际伤害/尸体提交；Projectile碰撞不等于有效伤害。一次长刃ActionId可命中多目标|Combat producer提交后发回执，身份包含kind/operation/target/epoch；需要真实撞击点时Sweep与Projectile传入Hit.ImpactPoint，不能用Actor中心冒称接触点|chop/metalClick有合法来源；未试听，不宣称木/金属/人体命中素材已完成|
|受击/格挡|Damage 180登记ID，182躲闪拒绝；Guard.Hit接受在188；ReceiveDamage接受在195。弟弟与环境/摔伤经过Survival.ReceiveDamage，缺Combat组件|Survival.OnDamageSucceeded与Combat.OnCombatSucceeded已发布成功回执；Presentation已订阅自己及当前Brother，SuccessId/epoch去重、暂停/死亡/30m外先观察、实际TargetPosition定位，尚未Native GREEN。不读health/inventory差值猜成功|RPG包未确认人体受击声；可先接正式事件，cue未绑定静默|
|真实脚步/采集接触|Hero/Brother以SequencePlayer运行Walk/Run/Dig，现源码没有接触Notify消费；MovementUpdated只证明移动。UE5.8.2实际读取四Walk/Run均READ、0 Notify/NotifyState，foot_l/foot_r轨迹存在。接触生产入口缺失已CONFIRMED|先只读实际左右足/趾完整骨骼与逐帧压缩Pose，结合对应mesh/floor验证plant时点；之后申请一个自定义UAnimNotify .h/.cpp、Presentation消费/定向测试和下列准确四包窗口。当前Source/Content仍冻结，未添加Notify或假脚步|CC0包footstep00–09均存在；文件名无石/木/土分类。Source/Config/Data未找到PhysicalMaterial/SurfaceType映射，不能按视觉颜色或材质名字伪判表面|
|落地/泳入泳出|公开LandedDelegate与MovementModeChangedDelegate已绑定；Landed早于fall damage，PostPhysics才消费候选。真实fixture碰撞与Traversal水面模式转换已GREEN|当前Presentation内完成；自然地图正常入水/浅水站立与音频仍另验，不能把Swim entry当所有涉水声|无对应cue绑定，静默；不需Owner批准才能接引擎事件|
|救援/升级/领取|RecordRescue 342的GrantExperience先通知且343可能回滚，344才最终State提交；Facility upgrade 323才Level++；Claim113添加ID、117最终OnChanged|两条真实API Native已RED：所有观察计数0，正式提交与实际Load成功。Presentation已在PostPhysics读取最终Rescued/person、Facility GUID+Level、Claimed/quest，Load/BeginPlay播种、paused/dead先观察后抑制；Level1不冒充upgrade，尚未GREEN。不改reward/Save/生产者|目前均无cue，静默；工程可直接推进，听感独立待审|
|营地火|有稳定Facility GUID、Building.ResolveFacility真实actor与campfire静态model/FurnitureInteraction；没有已确认Lit/Burning状态或点火/灭火公开事件。Paused表示生产暂停，不能据此认定火被灭|当前只记录缺口。明确常燃/活动火状态契约后，才确定声源活性；不新增燃料/天气/听觉AI玩法|原包无已确认fire loop；不能把campfire设施存在写成正在燃烧，也不能拿一段操作声伪循环|
|水环境|experience lake_west来自TASK026 lakes[0]；脚本将Lake_0/River/Sea真实mesh保存为water标签、空间流送actor。配置椭圆永久存在，不代表mesh当前已加载|先取得运行时已加载mesh/tag/稳定source关联元数据。源须来自live actor，初始loaded/LevelAdded/延期ActorSpawned登记；ActorEndPlay/LevelRemoved/Load/退出清理。Root当前明确暂不造无cue循环系统|原包无已确认water loop；自然场景目前只有静态水mesh，不等同流速/瀑布活动契约|
|风环境|Source未找到已确认运行风源/区域/室内停止契约；WorldPresentation负责光照/sky，未接音源|待确认真实Wind源或既有区域映射，不能给整个Wilds硬编码风声并创造天气/听觉规则|原包无wind loop；候选来源/许可未登记|

加载生命周期可复用UE公开ActorSpawned、Actor.OnEndPlay、LevelAddedToWorld/LevelRemovedFromWorld。SpawnBuilding先Spawn actor，随后才写Completed tag与Interaction kind，源匹配必须延后一PostPhysics检查，不能在Spawn回调里提前判定已完成设施。睡眠不补播八小时历史离散事件，恢复仅按当前活性源重建。

保留的官方原包 `art_source/TASK-099/kenney-rpg-audio/kenney_rpg-audio.zip` 为964837 bytes。包中有Preview与51个Audio OGG，包含10个footstep、chop、knifeSlice两项、metalClick等；原License与[Kenney官方页](https://kenney.nl/assets/rpg-audio)均给出CC0。当前只抽取/转换仓储候选，以上新候选均未转换、未绑定、未试听。没有音色授权风险足以阻止正式事件接口工程；具体声音首件/混音/循环接缝仍需实际素材与试听。

四脚步资产严格来自当前两AnimInstance与094登记：Hero AnimationV2 A_Hero_Walk/Run、Brother Animation A_Brother_Walk/Run。094对角色/动画输入权利记录为UNKNOWN_ACCOUNT_AND_INPUT_RIGHTS，属于候选发行素材门槛；Notify只读元数据检查照常可做，不能用该未知权利替代技术调查。

引擎事件语义参考[Epic ACharacter](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/ACharacter)及[LandedDelegate](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Character/LandedDelegate)；项目实际版本的判断以本机UE5.8源码为准。Owner试听、真实音轨、材质衰减和最终Shipping检查均NOT_RUN。

## 四动画包实际零Notify与后续最小技术窗口

原始只读证据 `.agent-local/qa/TASK-099/animation-contacts-20261006T225948Z-dcdcc6b1/metadata.json`，完整归档 [metadata](animation-contacts-20261006T225948Z-dcdcc6b1.json)。UE实际版本5.8.2-56702186，2026-10-06T22:59:48Z读取，四项READ/0 Notify，changed_asset_packages=[]。公开UE启动/退出证据另归档 [launch](animation-inspection-launch-20261007-01.json) / [stop](animation-inspection-stop-20261007-01.json)。这是metadata检查，未播放动画、未验证足底接触、未改uasset，无法据此给左右落足时点。

|精确候选技术锁|长度/报告帧数|实际骨骼|现Notify|
|---|---|---|---|
|Content/Characters/Hero/AnimationV2/A_Hero_Walk.uasset|2.3333332538604736s / 56|SK_Hero_Skeleton；foot_l/foot_r轨迹|0|
|Content/Characters/Hero/AnimationV2/A_Hero_Run.uasset|1.25s / 30|SK_Hero_Skeleton；foot_l/foot_r轨迹|0|
|Content/Characters/Brother/Animation/A_Brother_Walk.uasset|2.3333332538604736s / 56|SK_Brother_Skeleton；foot_l/foot_r轨迹|0|
|Content/Characters/Brother/Animation/A_Brother_Run.uasset|1.25s / 30|SK_Brother_Skeleton；foot_l/foot_r轨迹|0|

这四个路径当前均为只读；父任务需给准确四包写锁后才能修改。实际脚步入口缺失属于工程工作，不归为Owner音色设计阻塞。toe/ball完整骨骼、足底mesh偏移、每次接触帧/左右顺序、跨循环边界与walk/run混合期均UNKNOWN。

最小后续分两步实施：

1. 只读采样：使用本机UE5.8 `UAnimPoseExtensions::GetAnimPoseAtFrame` / `GetBoneNames` / `GetBonePose(..., EAnimPoseSpaces::World)`，读取当前四包的Compressed Pose、完整实际脚/趾骨与父链。引擎的World选项指component space（AnimPose.h:36），不能冒称游戏世界坐标；还须结合对应实际mesh transform/足底和真实floor接触读取验证。帧时点按序列实际采样率/GetTimeAtFrame读取，不能按时长假设左右各一次或固定FPS。只能先输出接触候选，实际地面与动画预览验证后才锁定左右plant时点。
2. 获得接触证据后，单个自定义 `Animation/HearthwardFootContactNotify.h/.cpp` 的Notify实例用真实FootBone区分左右，在四包已证实plant时点添加事件。自定义Notify允许保留原Skeleton与两AnimInstance，避免为命名Skeleton Notify额外修改Skeleton。运行时仅从引擎真实Notify回调、当前实际骨骼位置与真实地面trace发出接触，非grounded/暂停/死亡/无有效接触/远距/未绑定静默，不使用时间或走路距离轮询。正常循环和混合由实际Notify与Locomotion sync leader规则验证；Load/EndPlay丢瞬态候选，不能补播历史步数。真实Clip播放+左右接触/floor、走跑混合、跳跃/Swimming、暂停/Load/退出夹具接入新Notify与Presentation定向范围。

技术施工需最窄Source锁：上述新Notify .h/.cpp、Presentation .h/.cpp、AudioFeedbackLifecycleTests.cpp；Content锁只有上述四包。当前未实现这一步，也未写Content。实际表面分类需真实Hit.PhysMaterial/SurfaceType配置；现Source/Config/Data没有已确认映射，先使用独立未绑定footstep事件即可，不由颜色或资源文件名猜木/石/土。Kenney CC0 footstep00–09仅为合法候选，未转换、未绑定、未试听。

API依据为本机UE5.8 AnimationBlueprintLibrary/AnimPose.h与[官方动画库](https://dev.epicgames.com/documentation/unreal-engine/API/Editor/AnimationBlueprintLibrary/UAnimationBlueprintLibrary)。旧GetBonePoseForFrame接口在本机标记deprecated，采样方案使用AnimPose，禁止将该接口的bone-local位移直接当足底高度。
