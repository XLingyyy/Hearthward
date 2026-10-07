# TASK-099 触地最小扩围与实施窗口

root 根据用户逐任务本地实施授权，追加以下技术范围；Task099 MD/JSON、REPORT 和 handoff 由 route_audit 唯一更新。本目录记录供其并入任务单，不并写那些文件。

- 新生产文件：`Source/Hearthward/Experience/HearthwardFootContactNotify.h/.cpp`。
- 新独立测试：`Source/Hearthward/Tests/FootContactNotifyTests.cpp`。
- 消费者：`Presentation.h/.cpp`、cue/config 和旧音效测试由 route_audit 唯一持有，本代理不写。
- 四个内容资产：`Content/Characters/Hero/AnimationV2/A_Hero_Walk.uasset`、`A_Hero_Run.uasset`、`Content/Characters/Brother/Animation/A_Brother_Walk.uasset`、`A_Brother_Run.uasset`；不修改 Skeleton、mesh、其它动画或地图。

root 已实际核验四个远端 LFS 锁属于当前用户 XLingyyy：HeroWalk 52083700、HeroRun 52083702、BrotherWalk 52083473、BrotherRun 52083472，证据 `.agent-local/qa/TASK-099/foot-locks-20261007/targets.json`。没有新建锁、重锁、强制解锁。四个实际本地文件已复制至该目录的 `backup/Content/...`，大小分别412800、341587、394379、322846字节；无 SHA 操作。随后root在批准窗口实际保存四包16条候选，当前资产与源码已冻结，备份继续保留。

生产继承实际 `UAnimNotify`，timing 只来自动画资产；没有移动速度/周期定时脚步。它读真实角色、脚骨世界位置、CurrentFloor 和脚下 Visibility 阻挡，要求 game world、非暂停/恢复、Alive、Enabled、实际正在地面移动；拒绝缺骨、起点穿透、不可行走法线和人物阻挡。20厘米短向下查询范围来自本轮候选踝高10.52–14.97厘米。成功声源位置为实际 `Hit.ImpactPoint`。

`FHearthwardFootContactReceipt` 为临时 native 值：`SuccessId`、`Epoch`、`Frame`、弱 `Source`、`FootBone`、`Position`、弱 `Material`。`Frame` 默认0合法，Notify 在同步派发前写入真实 `GFrameCounter`。消费者只接当前帧回执，每帧清理脚步去重记录，同帧不同成功 ID 仍可发声；脚步不进入全 epoch `PlayedEvents`。这防止持续步行令两个全 epoch GUID 集合无限增长，并拒绝晚帧旧回执，没有一秒节流。不持久化、不改玩法、不增加静态事件总线。Notify 调用当前玩家 Presentation 的 native public `FootContactSucceeded(const FHearthwardFootContactReceipt&)`；该方法的 false 可以表示音效抑制，不回写玩法，也不等于脚下碰撞失败。root 已批准未命名的 PhysicalMaterial 使用通用 `movement.footstep`，没有石/木/土推断。

精确测试过滤器为 `Hearthward.Iteration.Task099.FootContact.`，两项：

- `GroundGuards`：独立 GameInstance、LocalPlayer/Controller、真实 floor Box、实际 CDO 同款 mesh/scale；真实 World BeginPlay 与 PostPhysics tick，使用运动输入及 capsule physics 获得 Walking/CurrentFloor，未强制 Walking。检测真实 floor impact，并拒绝无地面、Falling、静止、缺骨、暂停和通过真实 ReceiveDamage 进入倒地的 source。
- `ActualClipDispatch`：四段真实 asset 需各有两左两右 Notify、leader-only 和0.5权重门槛。实际 SingleNode instance 根据保存动画事件区间提取并分发 Notify，在已评估脚位与真实 floor 上创建声音；测试不直接构造 receipt，不直接调用 Notify/消费者/播放方法。每段检查一个实采左接触候选，尚未覆盖实际 locomotion proxy 混合或真人听感。

只读 metadata 已真实确认零通知缺口（四段 READ、Notify=0）。随后首次Editor build因夹具访问protected PauserPlayerState而FAIL（Native未运行），已仅修成public SetPauserPlayerState。Native06真实Editor build SUCCESS，联合25项20P5F；本单ActualClipDispatch四包均0L/0R/无声音，12条错误为正式writer前预期RED。GroundGuards另1条失败是100点伤害被接受但没有验证实际HP为0/Downed，已仅修夹具使用public实际HealthBefore伤害并断言HP0/LifeDowned/DownRemaining>0；修后Native未运行，不能由该修改宣称PASS。原报告分别留`.agent-local/qa/TASK-099/native-pre-contact-20261007-05/build.json`和`native-pre-contact-20261007-06/native/index.json`，没有删错或ExpectedError屏蔽。

写入脚本 `docs/qa/TASK-099/foot_contacts/write_contact_notifies.py` 已由root实际通过公开UEClient执行。先检查新编译类、四包身份、四包零通知、备份和实采帧时间，然后在第一次资产修改前创建新的证据目录；允许上层目录存在，但目标目录已存在时直接拒绝，避免四包保存后才因目录冲突无法写记录。再在内存创建16条候选。它用本机原生 `AnimationLibrary.add_animation_notify_event_from_source` 明确保存0.5权重、follower=false与实际 Notify 对象；不直接覆盖数组、不重导入。实际原报告确认四包各两左两右、0.5门槛、follower=false，skeleton、length、frame count、bone tracks及原notify tracks前后相同。仅保存四个已加载asset，不保存全Content。原包已有通知时直接拒绝，未实现覆盖或清理重试。

顺序由root独占：CPU full已结束 → 新producer/consumer实际Editor build SUCCESS → 四包writer实际保存 → 保存后既有两项Native实际Success。原始写入报告与公开启动/停止结果按字节归档于[asset-write-20261007-01](asset-write-20261007-01/write.json)：`SAVED_4_ASSETS_16_CANDIDATES`，launch/stop的ok均true。原private路径`.agent-local/qa/TASK-099/water-foot-write-20261007-01/contacts/write.json`、`launch.json`、`stop.json`及runtime/profile保留。两段Run原请求接缝0秒，实际保存的trigger_time为9.999999747378752e-05秒，UE规范化事实原样记录；末端没有第二条接缝事件。本代理未执行引擎或资产写入。循环blend、空间听感与Owner试听尚未验证；资产候选保存不作为物理触地PASS。现有SingleNode测试与需要的最窄补验边界见[只读审查](READ_ONLY_REVIEW.md)。

实际07原报告已归档于[完整index](../native-post-contact-20261007-07/index.json)、[构建](../native-post-contact-20261007-07/build.json)、[结果](../native-post-contact-20261007-07/result.json)，全24P，本单19P；Foot两个用例均Success/0warning/error，是真实ground guard和每段一个左候选的SingleNode→Notify→实际脚位blocking/walkable trace→consumer→AudioComponent链，不证明正常AnimGraph或真实Brother source。08另一个旧Save隔离新UUID实际1P，路径独立；具体用例0错误与每个进程13条启动Smoke错误分开保留。

07完成后，root授权只在原`FootContactNotifyTests.cpp`扩既有ActualClipDispatch断言，没有新测试编号、生产或资产改动：两段Run读取实际保存的右接缝事件时间，在已评估真实右脚pose上提取正向短区间，要求恰一个实际声音；另真实SpawnActor Brother/InitializeCompanion/实际Controller与胶囊movement获得walkable floor，经public LocalAI.QueryInventory绑定实际Player/Brother，再提取Brother Walk实际Notify并要求玩家consumer创建一个AudioComponent。没有手工Notify/receipt/播放、没有forced Walking或fake CurrentFloor。该新增范围尚NOT_RUN，07历史成绩不迁移为新断言PASS；root须fresh build后Foot-only测试。近起点正向区间不等于跨末尾wrap或正式AnimGraph混合，相关未验状态保持。

## 最新实际09／13验证补充

Root第09轮扩展的两项Foot Native已实际Success；第13轮完整28/28中这两项仍Success，0选中warnings/errors。ActualClipDispatch新增两Run真实保存右脚近起点正向interval与真正SpawnActor Brother/InitializeCompanion/public QueryInventory当前绑定派发已通过；触发实存9.999999747378752e-05s，不是精确0，也不算末尾跨wrap。GroundGuards继续通过。精确结果见 [13原始index](../native-final-audio-20261007-13/index.json)、[23原对象子集](../native-final-audio-20261007-13/task099-subset.json)。

本轮-NoSound/-NullRHI只证明真实Notify/foottrace/receipt→AudioComponent，不证明设备听声。正常Hero/Brother AnimGraph混合、SyncGroup leader切换与自然跨wrap仍NOT_RUN；前述“新增断言未跑/只左候选”的阶段记录为补验前历史。Source与四精准包继续冻结。
