# TASK-092 路线与净空取证计划

2026-10-07。当前依据084 [ROUTE_MANIFEST](../ROUTE_MANIFEST.json)的静态路由和部分开场UI观察；091尚未产生本轮新实现。首次救援限定 `camp → route_fork → slice_rescue → camp`，使用现有 `campaign.route_trace`，本单未移动任务、敌军、资源或奖励参数。

## 当前已确认的源码事实

- 玩家胶囊半径34cm／半高90cm；弟弟和Campaign族人分别为30cm／80cm。玩家／弟弟MaxStepHeight=45cm，坡度45°；Campaign角色沿ACharacter移动组件既有值，当前实机值需记录。
- Recast配置半径34cm、高180cm、Dynamic、2048固定瓦片池；仅在invoker附近生成。弟弟／玩家注册4000／5000cm，族人3500／4500cm。600m级往返不适合一次全程完整路径查询。
- 营地中心XY `(-98000,-75000)`cm、岔口 `(-63000,-89000)`cm、首救点 `(-38000,-84500)`cm。Z必须在正式加载地形读取；不沿用20000cm兜底，也不把经济营地初始16200cm当本轮测量。
- 首救族人 `rescued_01`；未接触时生成遇碰撞可投影到1200×1200×600cm附近导航，实际首落点须捕获。已接触／已保存的族人位置不通过该初始逻辑移动。
- 077既有卧室门洞280cm、楼梯宽400cm、台阶最大20cm；100cm门槛断缝与床碰撞旧修复均在现有代码中。077旧版失败仅作历史线索，不能认定当前相同缺陷。
- 跟随者距领导者超过7000cm、活敌距族人小于2200cm或WalkTo失败时进入waiting；返营需实际族人坐标进入当前营地半径且玩家不在战斗中，再唯一登记人口和成长。

官方工程依据：[Navigation Mesh Settings](https://dev.epicgames.com/documentation/unreal-engine/navigation-mesh-settings-in-the-unreal-engine-project-settings)、[Find Path to Location Synchronously](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/AI/Navigation/FindPathtoLocationSynchronously)、[Automation Test Framework](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine)。局部查询使用实际Pawn的导航参数，完整路径与部分路径分开；原生夹具结果和正常输入路线分开。

## 定向执行入口

新增 [RescueRouteTests.cpp](../../../../Source/Hearthward/Tests/RescueRouteTests.cpp) 注册两项，未由本子Agent构建或运行。root通过已接通的公开UEClient构建、发现用例数>0后执行：

1. `Hearthward.Iteration.Task092.Fixture.FollowWaitAndPhysicalArrival`：独立Game测试世界、真实动态Recast／AI PathFollowing／CharacterMovement／Campaign Interact／Camp RecordRescue。只在夹具里缩短路线、移动玩家目的地和拓宽invoker；正式数据不变。先在250cm合法交互距离启动真实Moving路径，按E式生产交互切waiting，观察是否取消路径；随后实际族人移动进入营地并验证人口／XP最多一次。
2. `Hearthward.Iteration.Task092.Live.LoadedSegmentClearance`：只在正式 `L_HearthwardWilds` Game／PIE且序章已结束、无待旅行、弟弟与首救族人均加载、三人已落地时运行。沿首救既有trace取玩家附近的两个采样点；记录地面、实际胶囊站位、三类实际角色双向完整局部路径。该项不移动角色、不生成actor、不改invoker、不结算奖励。未满足前置会明确报错，不降级为平地夹具或跳过通过。

Live仅是一个时刻的局部检查；受动态导航建成时机影响的失败先按输出坐标／agent／前置调查。它不覆盖全程连续输入、实际走到目标、敌情选择、斜坡步进或流送保存。原生Fixture若失败，先保存原始断言与路径状态，再决定正式修复；不能改夹具把真实缺陷隐藏。

## 必须另做的正常操作

| 节点／操作 | 必须记录的实际结果 | 当前结果 |
|---|---|---|
| 营地出口至岔口 | 玩家普通输入；弟弟留营与同行分层，连续位置、地面／坡度、相机、导航停顿 | NOT_RUN |
| 岔口至首救交互区 | 按既有trace接近，三人巡逻可见窗口、绕行／处理／撤退，原警戒规则 | NOT_RUN |
| 首救对象 | 接触、等待、再次跟随；危险／远离／路径失败的反馈与实际动作一致 | NOT_RUN |
| 首救返营 | 三者按原能力双向通行；实际族人入营、人口20→21、XP与奖账本一次；不注入arrived | NOT_RUN |
| 出发／救援后／到营前 | 隔离测试档正常保存、退出、继续；区域卸载重载，不复制敌人、族人或物品 | NOT_RUN |
| 美术交接 | 捕获坐标、障碍包／组件、胶囊宽高、地表／门槛／坡度、实际走廊边界和对应091指引 | WAITING_084_092 |

门洞尺寸、route_trace点和NavData配置不能直接形成冻结碰撞包络。当前不授权任何Content／umap／ExternalActors写入；095—098不得据本页替换碰撞或扩大路线。
