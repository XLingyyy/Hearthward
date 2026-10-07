# CT-TASK-085｜本轮只读显示投影与动作边界

状态：ADOPTED（2026-10-07按用户本轮执行授权核对采用；工程验证待执行）。提供者：TASK-085；首批消费者：086，后续088—091、093按实际需要增补。源码基线：`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，当前实现为`codex/TASK-084-103-iteration`工作区改动，未提交。

此契约只补充表现层，不替代已有库存、伙伴、营地、世界时间、存档和战役契约。实施前085核对实际接口，复用同义类型；本稿列的是逻辑字段，不要求照字面新建全部C++结构，更不新增全局运行服务。

## 1. 通用快照

每次投影包含当前时间线标识（`FGuid`）、实体/任务/事务的既有稳定ID、有效性与原因，以及显示数据。来源失效/世界销毁/Load/新游戏后，快照失效。未提供的信息是Unknown而不是0；不能通过“缺值默认0”告诉玩家任务已完成。个人数量及队伍完成批数使用`TOptional<int32>`；无任务/无源时不提供数量值。

读取不改变状态。投影中的Enabled/Reason为提示，不是执行权限；按钮在执行瞬间通过原命令入口重新校验距离、危险、设施、数量、epoch与约束版本。不得将模型文本/界面状态写回权威事实。

## 2. 个人委托视图

|逻辑字段|权威来源与语义|
|---|---|
|TaskId/ItemId/Phase|现有Command/Goal及GetPhase，不新发任务ID|
|Requested|GetRequested，目标量|
|Delivered|GetDelivered，已真实交付量|
|Carried|GetCarried，当前任务携货，不能当成交付|
|Acquired|GetAcquired，只作已有定义下的累计采得信息；不得与Delivered相加冒充完成|
|RemainingToDeliver|max(0,Requested−Delivered)，必须注明这是尚未交付，不是还需采集|
|Destination/BlockReason|经可知过滤的目标名称和真实受阻原因，未知对象不泄漏坐标|
|AllowedActions|从当前任务类型/阶段投影既有Resume/Cancel等操作，不支持的动作不伪造|

个人完成由原任务终态决定。不得只看“已采得+携货≥目标”就标完成。个人自动换点不在本契约。

采用接口为`FHearthwardCompanionWorkView`、`HearthwardPresentation::ReadCompanion`与`ProjectCompanion`。`HasTask`保留已有Completed/Cancelled记录；`Terminal`仅由这两个真实phase确定。`CompanionPhaseText`覆盖全部现有phase。`DestinationText`只描述现有交付容器角色（营地仓储/玩家背包/弟弟背包），未定义的其他活动留空，不输出未知目标坐标。

`CanResume`表示当前属于HoldingSafely/WaitingAtCamp且交流和页面状态允许尝试原续接；资源、安全、路线等执行条件继续由ResumeBlocked复查。`ResumeReason`/`CancelReason`分别解释不可用动作。数量源不一致时返回不可用状态，不修正权威数据或伪装完成。

## 3. 营地队伍视图

显示当前岗位/资源、分配族人数、弟弟是否在队/是否真正到岗、持续生产开关、等待刷新/受阻原因和原生产累计口径。每岗五人、最多四族人加弟弟；不得自动抽调已占岗位。累计入库不是“本次委托已完成百分比”；本轮没有定量停队。

可见工人动画只表现权威岗位，不触发产量，离屏/暂停动画不改变既有后台生产。弟弟必须真正到岗才计其劳动力。

采用接口为`FHearthwardCampWorkView`及`ReadWorkParty`。Region/Camp/Job来自现有`FHearthwardCampRegion`；Workers为该岗位真实人员副本；AssignedBrother与BrotherWorking分开，后者读取原`UHearthwardCampSubsystem::BrotherWorking()`。`CompletedBatches`保持原Region.Completed的批次数单位；具体累计产物沿原`DescribeWorkParty`口径，不将批次数直接显示成份数，也不形成完成百分比。

## 4. 材料追踪视图

一个会话内只追踪一个配方+批数。Needed取正式材料表乘批数；Available复用原库存Available与实际营地可用仓储规则；Deficit=max(0,Needed−Available)。仓储参考数和现地可用数明确区分。所有重量沿现有API的UI单位与换算，不能猜配置原始weight单位。

材料齐备≠完整可制作；完整可制作使用CraftingStatus。此视图不预留、不消耗、不自动制造/派单。页面切换保留，Load/新游戏/切档清空，不增加Save字段。

## 5. 回营与成长视图

本次成果只能来自已有的真实成功回执或已观测状态转移。库存净差混合生产/消耗/交付，不能拿来断言某次弟弟委托入库。已有可持久回执不足时，加载后只显示当前营地状态，不编造上次行程摘要。

升级/领奖/救援仍调用原事务；通知不得执行奖励。显示去重用当前epoch与已有事件ID，不新增奖账本或Save schema。

## 6. 导航视图

主任务目标与途经入口分开。人工路线节点只影响提示，不是任务完成条件，不强迫经过。只读取可知的任务/地点/位置；偏离、返走、读档、阶段变化和传送后重新求解。无适用路线时退回现有方向标记，不画没有可达证据的路径。

保持既有4032米级地形与3000×2000米查看区域的概念分离。无黑雾不开放未知剧情；地图暂停不误挡合法传送，但其他激活/危险/集合/可通行落点限制不变。

## 7. 五种不同操作

|操作|允许效果|绝不隐含的效果|
|---|---|---|
|关闭/返回页面|关闭当前显示、恢复正确焦点与既有暂停设置|取消正在执行工作|
|取消提案|丢弃未确认候选|撤销已生效任务或退还材料|
|取消模型请求|使该请求及迟到回复失效；可回手动入口|取消当前伙伴工作|
|暂停/继续队伍|调用既有队伍生产开关入口并保留原进度|取消个人任务、重建人员分配|
|续接/取消个人委托|经原安全/epoch/对象检查处理相应任务|丢弃携货、跨点重规划或创建队伍|

## 8. 生命周期、刷新和测试

低频可见UI优先事件驱动/失效更新；无事件时有界刷新，不为此全量重构框架。地图朝向和屏幕投影按实时需要更新。订阅在离开/销毁时注销；不能捕获过期裸指针。

适配函数按调用取得值快照，不建立服务、订阅、后台定时器或对象缓存。当前Widget已有0.2秒有界刷新，地图朝向仍由原实时路径更新。085未新增控件，086开始消费状态卡时登记稳定LayoutId/Component并做渲染验证。

个人动作使用`FHearthwardCompanionWorkView::Matches`及`CanApplyPersonalAction`比较快照epoch/CommandId、检查当前来源与阶段。085已在原ScreenActions续接/取消路径接入现值检查；086负责新状态卡携带对应epoch/CommandId，旧卡不得改为针对当时的新委托。AI确认卡继续使用原CandidateId和Ticket校验。

085提供只读、对象失效、时间线和动作分离的定向原生测试；086验证交付/队伍展示契约；088验证材料真值；089验证实例/转移；090—091验证追踪与导航不改任务；093验证反馈不发奖；101验证时间线和重复结算；102测成本。2026-10-07根Agent通过公开UEClient完成本轮构建，085原生实际发现4项并4/4 Success，零警告/错误；证据为`.agent-local/qa/TASK-085-088/build-repair2-20261007/result.json`及`native-baseline-20261007/index.json`。085实际渲染、真实输入、Owner/真人验收仍为NOT_RUN；原生通过不代替这些层。

## 9. 采用记录

2026-10-07，Owner在当前Codex会话要求“再去逐个执行文件夹Hearthward_TASK-084-103里的任务……如果没有设计上的问题需要人工确认就自己一直做就行”。据此采用本批既定展示适配，保持原玩法、执行入口和Save格式。根Agent将085本地实现委派给task_audit；共享动作文件完成后已释放，README由根Agent串行收尾。

当前提供者版本为上述基线上的dirty实现；新ReadModels不使用AI的`FHearthwardNPCContextSnapshot`作为UI账本，该类型继续用于有裁剪档位的模型上下文。材料追踪、导航、回营反馈的逻辑边界已采用，实际新增类型分别等088/091/093需要时实现，085不预建未消费结构。Owner视觉/真人验收和远端发布未由本记录代填。
