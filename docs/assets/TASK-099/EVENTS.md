# TASK-099 事件与声音覆盖

2026-10-07 当前实现与最新受测版分开记录。最新实际build/Native为 `native-green-20261007-03`：7项Success、1条固定voice预期缺文件Warning。04轮12项7P/5F，原7项现0Warning；Progress/Combat声明阶段真RED已归档，当前消费者已实现冻结等GREEN。声音不能执行库存、伤害、奖励或任务完成。

|事件|真实事实来源与身份|当前位置/去重/生命周期|实际状态|
|---|---|---|---|
|仓储提交|Storage.OnTransferred，原双方库存与重放日志先提交；Operation GUID/epoch|既有NearStorage访问条件；相同ID一次、不同快速ID各一次；暂停/远处先观察不补播；PCM时长、Load和EndPlay清理|绑定唯一仓储CC0 WAV；原3条持续GREEN；实际音轨/Owner未验|
|玩家craft/repair/harvest成功|Gameplay.Events正式提交后的具体目标计数；epoch/key/new_count；忽略:any|本地玩家成功提示；重复OnChanged无增量不产事件；BeginPlay/Load播种历史|真实API夹具GREEN；player.craft/repair/harvest已绑定实际CC0内部候选，当前Bound验证NOT_RUN；Unbound诊断夹具显式隔离|
|NPC acquired/craft/repair/delivered|LocalAI.GetEvents合法GUID、当前campaign/epoch、正count|真实Brother.CanCommunicate距离；所有ID先Observed、只有实际音源才Played；正式Storage同ID/实例子ID不双播；camp_region_completed跳时静默|真实wood v2/craft/repair/合法receipt夹具GREEN；NPC四路已绑定真实CC0内部候选，当前新Bound验证NOT_RUN|
|落地/泳入泳出|公开LandedDelegate/MovementModeChangedDelegate；每真实Falling落地或Swim状态转换|状态/落地等待位去重；落地延到PostPhysics经过FallImpact；Load/EndPlay丢候选；BeginPlay种当前mode|实际碰撞和fixture水体Traversal GREEN；新movement cue未绑定；自然地图实操未验|
|救援/设施升级/任务领取|最终Camp.Rescued person、Facility GUID+Level、Gameplay.Claimed quest；不消费可回滚的早期RewardFact通知|epoch+稳定身份/新level；拟Load/BeginPlay播种、paused/dead先观察|2条真提交/拒绝/Load夹具已RED，consumer已实现并冻结待GREEN；原Played=1是合法固定完成字幕，夹具比较实际基线|
|挥动/命中/受击/格挡|ActionId/epoch与真实提交点已定位；正式hit/block/damage producer已发布，Presentation已订阅自身及当前Brother Survival|命中不能取碰撞bool/DamageIds登记作为成功；kind/op/target/epoch区别同动作多目标|工程接口/消费者及3个真实consumer夹具已实现冻结待GREEN；三路combat已绑定真实CC0内部候选，native消费与位置衰减待跑；挥动正式事件仍未完成|
|脚步/采集接触|四个实际Walk/Run UE metadata均READ且0 Notify；Source无接触Notify消费；SequencePlayer已确认|不能用MovementUpdated或距离定时器伪造脚底接触；真实表面映射尚未确认|UE5.8.2已只读执行，四包零Notify证据归档；接触入口工程缺失已确认，左右plant帧尚UNKNOWN；CC0 footstep候选未试听/转换|
|火/水/风环境|live campfire设施与water mesh可定位；fire burning/wind活性契约未确认|需actual loaded source、Level/Actor结束/Load清理，睡眠只重建当前loop|无已绑定环境loop；root暂不造空循环；source metadata/活动契约缺口详见工程审计|
|固定对白/动态回复|54cue/28group配置均UNPRODUCED，动态文字|已确认文字/字幕与可知条件；已有录音仍走原PCM16校验，明确未生产且缺文件不读盘gate于04轮原7项中0Warning；已有文件仍走原校验|不录音、不TTS、不以静音占位；正常字幕输入验收NOT_RUN|

当前正式映射14条：保留storage.transfer原WAV，另13条使用8个独立真实CC0源，详见SOURCES/CUE_AUDIO_CANDIDATES。Observed和Played账分开保存；未用仓储WAV充当其它音色。effects/master沿用既有设置，真实听感、空间衰减、loop接缝、正常带音轨路线和最终独立包检查均另列NOT_RUN。

证据见 [REPORT](../../qa/TASK-099/REPORT.md)；准确后续工程窗口与合法候选见 [AUDIO_ENGINEERING_AUDIT](../../qa/TASK-099/AUDIO_ENGINEERING_AUDIT.md)。

2026-10-07 新Cue/Bound窗口：源文件与sound_events仅新增本单映射，固定文本/voice_status/非sound_events字段解析后相同。原Unbound夹具显式只留storage cue；新增2条Bound真实producer/位置/衰减/Pause/Load/EndPlay夹具尚未跑。指定位置时明确FSoundAttenuationSettings线性球0→3000uu；该参数验证不替代实际近远录音与Owner试听。

2026-10-07 Foot工程追加：Presentation native FootContactSucceeded 接入真实source/epoch/SuccessId/Hit.Position，暂停/死亡/远距/Restore消费不补播；generic movement.footstep追加真实CC0源，总15映射。新Notify生产由独立writer持有，四动画包实际零Notify且root仅核四包LFS权限；待编译/精准写包/真播放与地面验证，不声称正常路线脚步完成。

2026-10-07 当前冻源计数：15个独立event ID对应10个独立运行WAV路径，包含原仓储1个文件与新增9个文件；新增9个真实源OGG/PCM16、14个新event。AudioLifecycle14 + Combat producer3 + FootContact producer2 = 预计19条，尚未运行本轮Native。动画writer准备与Source冻结不表示四包已写入或正常路线已通过。
