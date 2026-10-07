# TASK-099 实际保存后的脚步验证边界

本记录为只读源审查，未修改Source/Content、未启动UE。写入原报告实际四包各4条、两左两右、0.5权重、follower=false；两段Run接缝实存9.999999747378752e-05秒。它证明候选已保存，不证明角色触地或声音已经运行通过。

现有`Hearthward.Iteration.Task099.FootContact.ActualClipDispatch`每段只提取一个左候选的正向短区间，通过真实SingleNode Notify→producer→consumer→AudioComponent链检验发声。它没有提取其它左/右事件，没有跨循环接缝，没有运行Hero/Brother实际AnimInstance图；Brother动画仍挂在夹具的plain Player角色上，不覆盖真实Brother source身份。GroundGuards保护真实地面/空中/静止/缺骨/暂停/倒地边界，与循环和混合是不同范围。

本机UE5.8`AnimSingleNodeInstance.cpp:359–390`的`SetPositionWithPreviousTime`直接调用`GetAnimNotifiesFromDeltaPositions`。后者在current<previous时将区间解释为倒放。因此把previous设为动画末尾、current设为开头的两点调用，不能证明正向循环接缝：它可能扫描倒放的大区间。需要实际启用looping并按正常Delta推进，或使用生产图的自然loop路径，让UE的`GetAnimNotifies(StartTime,DeltaTime,TickRecord.bLooping)`分段处理接缝；没有必要重写引擎或把Notify手工广播作为证明。

当前Hero和Brother的Walk/Run均加入同名`Locomotion` SyncGroup；leader-only可限制follower通知，这是源确认的配置。Hero gait权重随实际GroundSpeed在350–600变化，Brother在220–320变化；两者还有Idle/动作层。blend领导切换、0.5门槛下的通知保留、实际混合脚位能否命中20厘米查询范围，尚无运行证据。这里没有确认生产Bug，不能从SingleNode结果宣称这些路径通过。

既有两项FootContact在真实07已经Success/0warning/error。root随后授权先将原ActualClipDispatch补为两Run实际右接缝正向短区间和真实SpawnActor Brother来源，经public QueryInventory绑定当前Player/Brother；新断言目前NOT_RUN。该最小补验不覆盖跨末尾wrap、SyncGroup领导切换或正式AnimInstance图，不能据此声明正常混合循环通过。

后续只有实际正常路线揭露丢步/重复/空中声，或root明确要补该层验收时，再考虑一条实际Hero/Brother正式AnimInstance的真实floor/movement input集成验证，按正常World/PostPhysics tick从Walk进入Run并穿过一次Run循环接缝；观测真实ImpactPoint和即时AudioComponent，不强制Walking、伪造CurrentFloor、receipt或Notify。当前不增加该测试、不扩全矩阵或声音风格验收，生产和四资产保持冻结。

## 最新实际09／13验证补充

Root第09轮扩展的两项Foot Native已实际Success；第13轮完整28/28中这两项仍Success，0选中warnings/errors。ActualClipDispatch新增两Run真实保存右脚近起点正向interval与真正SpawnActor Brother/InitializeCompanion/public QueryInventory当前绑定派发已通过；触发实存9.999999747378752e-05s，不是精确0，也不算末尾跨wrap。GroundGuards继续通过。精确结果见 [13原始index](../native-final-audio-20261007-13/index.json)、[23原对象子集](../native-final-audio-20261007-13/task099-subset.json)。

本轮-NoSound/-NullRHI只证明真实Notify/foottrace/receipt→AudioComponent，不证明设备听声。正常Hero/Brother AnimGraph混合、SyncGroup leader切换与自然跨wrap仍NOT_RUN；前述“新增断言未跑/只左候选”的阶段记录为补验前历史。Source与四精准包继续冻结。
