# 先进三项首轮实际运行

2026-10-05，Root串行公开UEClient，Vulkan16层，锁定模型／参数，unique pool `33284f84-c86f-4731-b6dd-6b409e226c2c`。当前基线加未提交补丁，原始launch/results/stop保留同名QA前缀，Saved下保留cases.jsonl。CLI退出1；报告error=null，editor stop成功；整体不通过。

| 项 | 前置 | 实际 raw | 实际执行 | 结论 |
| --- | --- | --- | --- | --- |
| ADV-N01 指认野菜浇水 | 失败 | 未生成 | 未开始 | 公共plant入口返回false；不能计作模型或生产RED |
| ADV-N02 指认散石新采入库 | 通过 | 通过 | 未完成 | 来源8→7、弟弟stone+1、acquired/carried1，随后HoldingSafely；90秒未到Completed，入库0 |
| ADV-F01 同行钓一条留弟弟背包 | 通过 | 通过 | 通过 | 真实fish_carp+1、bait1→0、相同rod GUID耐久40→39、鱼点24→23／successes0→1、库存元数据与equipped保持、无其他增益 |

N02输入3145token，full_relevant并真实drop own_bag_optional；F01输入3031token，无drop，两者generation1、未确认时无世界变化。N02只提供实际采集信用；返营失败仍需BlockReason／Navigation／位置诊断，不以HoldingSafely推断单一原因。F01此次为普通渔获反馈“钓鱼成功”，没有实际稀有奖励分支信用。

种植夹具teleport后同帧调用Act，未等待玩家Walking；Act入口的“当前无法操作…”只来自Busy／Safe／count前置，SafeToSave拒绝Falling。这是有调用链的夹具时序候选，尚未用实际复测确认。下一轮只针对N01、N02，等待真实落地并增加公开诊断，冻结文字和oracle不改。F01已经通过，不重复执行。

本夹具含临时BlockAll地面、公开原型档池和显式定位，不提供正常新游戏键鼠／Owner体验信用。hunt、capture、camp_batch、escort、正式60/20、连续角色和联合性能仍未覆盖。
