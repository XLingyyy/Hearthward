# TASK-101 字段恢复与暂态矩阵

同版7项技术回归已实际Success、0错误；Loading输入模式夹具有1条EnhancedInput warning，见native-green-20261007.json。candidate-2/version.1的旧副本最新节点实际恢复、隔离保存与第三进程明确Continue已部分验证，见OS_ORIGINAL_COPY_COMPATIBILITY.json；当前version.2未构建，四阶段与真正ActualLoad→新暂态UI组合仍NOT_RUN。当前读到的写格式为 `HearthwardSave::CurrentSchema=9`、`NPCStateVersion=3`，以本轮实际源码为准。本单不改变字段、版本、迁移或时钟规则。这里列核对目标，未到达的四个合法进度节点不会以夹具结果代填。

|领域|现有实际保存来源|读档应核对的事实/原规则|本轮证据范围|
|---|---|---|---|
|随身/弟弟/共享物资|World.PlayerItems/BrotherItems/StorageItems|数量与容器身份、重量、容量、稀疏布局恢复；没有声音或显示产生物资|既有ActualLoad与FileIntegrity定向回归已在7项最小相关过滤器中实际执行（详见REPORT边界）；四阶段NOT_RUN|
|可用量与保留|当前Inventory/Storage Available及Reservations|可用量是当前持有量减当前有效保留；Load使旧epoch操作失效，不复活旧保留|ActualLoad/PartialSettlement本单原生Success；089 AccessAndTimeline另单Success（1条warning）；完整Load后旧卡NOT_RUN|
|装备实例与耐久|PlayerItems/BrotherItems/StorageItems.Instances|同类实例按GUID核对定义、耐久、唯一声明；装备/快捷引用指向同一实例|ActualLoad包含已保存实例；正常节点与全部四快捷栏NOT_RUN|
|个人委托|CommandId/CommandRevision、AgentGoal、Phase、Requested/Acquired/Carried/Delivered、NPCReceipts/NPCOperations|ID和持久事实恢复，票据epoch变新；携货不能当入库，重复回执不再扣材/发奖|ActualLoadPointInvalidatesOldCommandTicket与PartialSettlement已在7项最小相关过滤器中实际执行（详见REPORT边界）|
|提案/模型请求|无新增Save字段；LocalAISubsystem.ResetForSnapshot|pending/callback/确认提案不恢复成新档动作；Serial及现有Current检查失效旧结果|087开发夹具已有3/3GREEN；真实HTTP晚回包和跨LoadUI确认卡NOT_RUN|
|队伍与岗位|CampEconomy regions.Workers/Brother/Enabled/Batch/Completed/BatchStopAt|同一人不跨区重复岗位；暂停/批次/投入保留；睡眠不重复计弟弟劳动|Camp.PartitionSleepAndRestoration与既有批次边界；四阶段NOT_RUN|
|人口/救援|CampEconomy已救援人集合与Campaign/Gameplay救援事实|恢复当前人口及身份，不重复创建人口、奖励或提示|093 ReceiptTimeline另单原生Success，不属于本单7条；完整救援节点NOT_RUN|
|营地等级/设施|CampEconomy camps/facilities及Gameplay buildings|两营地、设施GUID、等级、位置、付费累计/赠送标记恢复；不重发礼物|既有实际Save具备接口；093旧首轮UpgradePreviewAndReplay Fail历史保留，修正夹具Focus后定向Success（1条warning/0错误）；四阶段和二营地独立重启NOT_RUN|
|任务/奖励|Gameplay tracked quest、RewardFacts及Campaign quests/facts|已完成状态及账本保持；读取/重建不再给经验和唯一物品|既有Save ActualLoad子范围；正常永久夺回节点NOT_RUN|
|控制区/敌人|Campaign enemies.Generation/Health/Stunned、Flags/Reinforcements|同代次终态/刷新规则保持，刷新不奖励；永久清空不误重生|Time.DomainPartitionRefreshAndEpoch已在7项最小相关过滤器中实际执行（详见REPORT边界）；正式四区实战存档NOT_RUN|
|世界时钟|ActiveSeconds/CalendarMinutes、ClockVersion/InitialDay/InitialMinute|同一边界恢复；显示起点与经过时间分开；Camp/Nature cursor与W一致|ActualLoad、DomainPartition与Camp分段睡眠已在7项最小相关过滤器中实际执行（详见REPORT边界）；真实暂停/对话/设施8小时NOT_RUN|
|生存/动作计时|PlayerSurvival/BrotherSurvival、HP/Hunger/Stamina、PlayerTimer/CompanionTimer|恢复当前允许的due/timer；危险保存仍拒绝；新epoch不继续不受支持事务|既有ActualLoad夹具；真实消费/救援/跌落/濒死节点NOT_RUN|
|知识/记忆|Knowledge/KnowledgeRevision、NPCMemory|同一Campaign知识恢复；历史事件不被新UI当新救援或入库播报|ActualLoad本单Success；093 ReceiptTimeline另单epoch诊断Success；真实模型和完整故事节点NOT_RUN|
|材料追踪|ScreenWidget中的FHearthwardCraftingTracker，未序列化|跨页可保留当前session目标；Load/新游戏epoch变化后清空并重读可用物资|源码确认epoch读时清空；真正ActualLoad→UI目标清空尚NOT_RUN|
|个人/队伍显示投影|PresentationReadModels返回值，未序列化|每次读取当前权威值，旧CommandId/epoch卡不能执行|085/086同版原生已通过；真正Load后旧UI卡与正常输入尚NOT_RUN|
|仓储操作卡|ScreenStorage的StorageEpoch/Operation/Selection/Quantity，未序列化|旧卡不能在新epoch结算；GUID实例不串件；失败不添物资|089 AccessAndTimeline另单同版Success（1条warning）；不属于本单7条，真正Load后旧仓储卡仍NOT_RUN|
|营地回营摘要/升级卡|CampFeedback会话Entries/EventIds和epoch+tier action，未序列化|Load播种已恢复事件、清空来源摘要/Notice；旧升级卡拒绝，状态文字可读|093 ReceiptTimeline另单Success；UpgradePreviewAndReplay与LocalFacilitiesAndWork旧首轮Fail历史保留，修正夹具Focus后定向两Success（合计2条warning/0错误）；ActualLoad新UI组合NOT_RUN|
|SFX/字幕声源|PresentationComponent Effects/ObservedTransfers，未序列化|Load/EndPlay销毁当前PCM源；不会扫描并补播历史转移；固定voiceUNPRODUCED|099真实事务RED→同版原生GREEN 3/3（0警告/错误），另单证据；实际有声/试听NOT_RUN|
|页面/输入/加载遮罩|Loading/Screen当前会话状态，不增加Save字段|加载结束依当前页面恢复GameOnly/UIOnly；设置Apply、IME按真实输入核对|UI069.LoadingRestoresLatestPageInputMode已在7项最小相关过滤器中实际执行（详见REPORT边界）；OS键鼠/IME NOT_RUN|

兼容包头：现有Read接受HWS1/HWS2/HWS5/HWS6/HWS7/HWS8/HWS9对应实际历史边界；不能把连续1—9均存在当契约。未来WriterVersion、未知包头、校验/截断错误拒绝，迁移/修复按既有预览与明确同意入口，保留原件和备份。原用户pool.hws未直接写读；其只读copy的独立复制件已在candidate-2/version.1实际列出4旧节点、加载最新节点、隔离写Compatible-v8目录pool并通过第三进程明确Continue。legacy及backup各35262 bytes、输出660502 bytes，原件/只读copy未写且未重复hash；精确格式不由目录名推断。当前version.2未构建，最终同版兼容及四节点完整字段等值NOT_RUN。
