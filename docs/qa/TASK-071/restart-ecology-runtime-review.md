# TASK-071｜跨进程生态到期与代次恢复实测

2026-10-04。受测对象是集成树基线67fb0784上的未提交生产补丁与显式Native生态夹具；未提交、未推送。最新Editor Development构建通过。

实际write `craftsource068-green-ecology071-write`、read `restart071-ecology-due-generation-read` 均通过，生态案例各1/1、0错误、0警告。池为 `E6111AB7730C4F18977540A7E56213C9`；write PID11484、read PID3548，两个独立进程已退出。真实手动节点GUID `99D991004E541D2B719E04A77BC163C5`，池内实际2点。原始index在Saved/Task053对应目录，写点manifest在Saved/Task071/该池目录。

写点通过真实付费工作台、篝火、生产伤害事务、设施等待及采集形成成熟状态。A=33.925000006333秒，W=2913.925000006333分钟，原点第1日20:00。动物槽nature_hare0保存Generation2、Current=8FA1ABD646289FA49E66DEAC6F9704F5、Due=5773.425000006333；两代旧尸体真实未领取Loot，实际已结算经验60。营地草药源camp_048_herb_patch0保存Remaining0/Capacity4、Due=5786.425000006333。资源刷新Due位于CampEconomy.sources，Nature.points的due=-1并非该共享源的到期字段。

read真实LoadPoint两次和重存先核对完整库存、装备GUID、进行中命令、NPC事件、A/W原点、成熟投入批次、宝图Pending、动物身份与两条未来Due，无重复收益。之后公开取消实际active collect，再通过真实篝火六次480W等待跨到期边界：活兔仅刷新为Generation3一次，草药源仅恢复Capacity一次，已投入rope批次只完成一批，Shared wood保持0，经验保持60。再等待60W，没有额外代次、资源、绳索或旧尸体收益。每次等待检查完成回执与W准确增加、A不增加；生存食物由真实Eat消费。

上述属于显式Native夹具，动物死亡调用真实HitTarget事务，场景准备使用公共位置与地面查询；未测正常挥斧物理命中、键鼠新游戏、菜单退出继续、正式两营地连续进度、鱼刷新或HTTP/UI迟到回调。TASK-071保持Active，这些出口保留未运行。
