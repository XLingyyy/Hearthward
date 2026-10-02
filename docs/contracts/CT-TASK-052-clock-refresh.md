# CT-TASK-052｜统一时间、刷新与恢复

状态：**APPROVED（Owner2026-10-02确认，施工和任务分支提交推送已授权）**。版本0.2，2026-10-02。Owner／Reviewer：XLingyyy。对应[052方案D1—D6](../planning/TASK-052/PLAN.md)；父玩法为[DSGN-R01](../design/DSGN-R01-world-time-persistence.md)已批准范围。

本候选覆盖当前schema8代码的接线；不沿用旧052分支的schema4契约，也不将043的公共接口草案解释为已经批准。

## 1. 唯一提供者与数据所有权

WorldClock拥有有效实玩秒A、累计游戏分钟W和初始显示日期／M0。Camp、Nature与Campaign拥有现有批次、个体、点位和敌人状态，Storage／Inventory继续拥有库存。领域Calendar为与W一致的游标，不能自主推进或按系统时钟补算。

时间协调复用现有`UHearthwardWorldClockSubsystem`，扩展当前`FHearthwardClockSnapshot`；不复制同名时钟结构。业务入口为睡眠、篝火及内部普通Tick，恢复仅由Save安装已验证快照。旅行复用Campaign的加载流程。

## 2. 请求和结果

候选`RequestTimeAdvance`包含：Kind（Sleep／Campfire）、当前CampaignId、TimelineEpoch、OperationId、起始W、设施ID及选择的游戏分钟。Sleep必须480分钟；Campfire只接受Owner确认的60／240／480。普通Tick为内部调用，不需要每帧生成GUID。UI不提交产出、敌血或替代世界状态。

结果包含OperationId、起始W、实际到达W、CommittedMinutes、状态（Completed／Rejected／StoppedAtFailure）和可见原因。拒绝至少区分INVALID_DURATION、STALE_TIMELINE、STALE_REQUEST、PAUSED、LOADING、BUSY、UNSAFE、FACILITY_UNAVAILABLE、RULE_UNRESOLVED。没有推进不能笼统显示“生存失败”。

当前epoch内以OperationId记录已提交请求及结果；同ID重复返回原结果，无再次加时或产出。同ID载荷不同拒绝。恢复生成新epoch并清空会话请求缓存，旧epoch请求失效；该缓存不作为跨回档可继续执行的任务保存。恢复前的新世界请求不能写入恢复后的快照。

发起事务前确认真实设施及位置、双方状态和所有现有消费者；开始后冻结输入世界写入。已有独立采集／护送／生产委托正在执行时，显式反馈需先处理当前任务，避免跳时模拟代执行。领域忙碌、保存恢复或加载过程中拒绝重入。

## 3. 时间与显示字段

- A：double有效实玩秒；正常推进，暂停／加载／跳时不增。
- W：double累计游戏分钟；普通1:1推进，跳时按实际CommittedMinutes推进。
- InitialDay：正整数游戏起始日，候选新档为1。
- InitialMinute：`0≤M0<1440`，候选新档1200（20:00），旧档迁移0。
- ClockVersion：候选1；当前格式必须存在且等于受支持版本。

完整经过日数`floor(W/1440)`和显示日期`InitialDay+floor((M0+W)/1440)`分别派生。四日全局周期只使用W；昼夜、HUD、环境光照／感知使用`(M0+W)%1440`。候选time配置为：version=1、initial_day=1、initial_minute=1200、sunrise_minute=360、sunset_minute=1080、light_transition_minutes=30、campfire_wait_minutes=[60,240,480]；已获Owner确认，进入gameplay.json的唯一time段。现实日期仅用于存档文件时间。

## 4. 事件提交

本次目标W确定后按最早领域边界推进。生存预演和提交使用同一饥饿技能／恢复参数；兄弟任一最早失败时间是全部领域的上限。失败点后没有生产、成长、刷新或奖励。

同W顺序沿043：真实动作／伤害、生存失败、生产／生态／刷新、任务和区域、可见回执、保存安全检查。共享材料竞争沿046已有岗位优先序。到期恰好等于目标W必须提交，字段不存在或规则缺失不能默认制造产出。

事务Busy覆盖预检后的提交区间，Save捕获／恢复和模型执行都检查该状态。库存回调不能在Camp已前进而Clock尚未前进时捕获快照。通知在稳定状态后发出；生产只通过既有真实Inputs、Work、Outputs、Completed状态去重，不建立新的产出副本。

## 5. 刷新身份与到期

| 领域 | 既有身份／状态 | 更新规则 |
|---|---|---|
| 静态自然物及生产源 | Camp Source.Id，Remaining、Capacity、Due、Blocked、RefreshMinutes | 全耗尽才排期；按自然白名单间隔，解除阻塞后一次恢复；扫描注册不重新排期 |
| 048资源点 | Nature Point.Id／Key及其Camp来源 | UI、玩家和岗位共享同一容量；流送只重建Actor引用 |
| 野生动物 | WildSlot.Id、Current、Generation、Due和Animal.Id | 沿048物种周期；一次新个体；受保护位置保留pending，不积欠收益 |
| 野外人类敌人 | Campaign Enemy.Id、Combat.Generation、RefreshDue | 清除W=d时due为`(floor(d/5760)+1)*5760`；恰好周期边界清除等待下一边界 |
| 故乡／增援 | 原敌人稳定ID和清除集合 | 无周期复活；K/N、四区控制、永久胜利和唯一奖励保持 |
| 作物／家畜 | Crop.Id，Animal.Id及Pen.Id | 成熟等待真实收获，按实际饲料与容量成长／繁殖；不走野外再生 |

跨多个野外周期只对一个已清除实例恢复新代次；下一周期没有实际再次清除就保持活实例。刷新不自动授予击杀经验、掉落或任务奖励。同代次击晕和击杀共享既有结算身份。

## 6. 旅行

旅行请求使用现有真实站点和epoch。预检固定玩家、交战弟弟和本次参战活敌集合；不接受UI指定回血名单。战役加载期间冻结A/W，所有必随角色落点验证成功才提交。

交战倒地弟弟位置改变但DownRemaining保持，未交战弟弟不瞬移。参战活敌只回血；已清除／击晕、盔甲、异常剩余时间、警戒、胜利和奖励不变。任一落点失败则位置和敌血保持。落地后再判断保存安全；延后的自动保存请求最多一个。

## 7. schema9／HWS9与安装

新增字段在现有FHearthwardWorldSave中，复用已保存A/W、Camp、Nature、Campaign及库存；各领域只升级自身必需字段。Operation结果缓存为当前epoch会话状态，恢复时失效，不保存可重试的跨时间线请求。

| 输入格式 | 候选处理 |
|---|---|
| schema8/HWS8 | 按旧格式严格校验；保留A、W和领域结果；补ClockVersion=1、InitialDay=1、InitialMinute=0；迁移野外全局due |
| 已支持schema1／2／3／5／6／7 | 先走当前已验证迁移链到8，再做8→9；仅确实无W的旧格式沿既有W=A逻辑 |
| schema9/HWS9 | 验证时间版本、有限非负A/W、W≥A及领域游标一致；无历史默认补齐 |
| 未集成HWS4、未知／未来格式、坏数据 | 明确拒绝并保留原件，不进入旧版修复路径 |

旧field RefreshDue合法时可由`old_due-5760`还原清除时刻，按下一个全局四日边界迁移。还原时刻必须有限、非负且不晚于存档W；不满足时返回兼容冲突，保留原档，不猜时间戳。迁移后的due可早于保存W，表示受阻待刷新。清除时刻存在则直接使用；故乡不生成due。

迁移不直接刷新Actor或生成奖励。恢复按既有epoch顺序安装时钟、领域、兄弟、库存和知识，然后重建引用及危险状态；不调用Advance重放。安全条件允许后再开放输入和处理待到期事件，按当前身份最多一次。原件备份、新版进度隔离和冲突确认继续沿现有SaveCompatibility流程。

新格式必须与当前更新兼容读取、未来版本提示及所有保存入口同时接通；代码回滚后不能用旧程序读取HWS9。保留升级前备份和旧档，不静默清档或降级重写。

## 8. 批准和验证

Owner已确认PLAN的D1—D6、此契约及施工路径，任务JSON已登记批准，工程字段按本契约进入公共声明。当前未更改Source、Resources或存档文件。真实验收按[052矩阵](../qa/TASK-052/MATRIX.md)，各用例绑定实际实现SHA；本轮均NOT_RUN。
