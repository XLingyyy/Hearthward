# TASK-092 工程调查与定向检查

日期：2026-10-07。状态：Active／部分实施。源码HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加本轮共享工作区未提交改动；最终受测编译与原生结果由root回填绑定，当前不造实现SHA。未提交／未推送／未发布。

## 本轮范围与证据

调查开工采用084 [静态路由和部分开场观察](../../planning/TASK-084-103/ROUTE_MANIFEST.json)，当时091尚无本轮新实现。定位已知077门槛／床碰撞修复、三种实际胶囊、局部动态Recast／invoker、首救跟随／waiting／返营代码及旧版077修复证据；随后091节点已有独立原生结果，仍不能替代正式通行。

目前未取得本轮真实通行阻断、掉地或坡度不可达复现；未据旧版失败记录重修当前几何。未移动任务、敌军或资源位置，未改奖励、警戒、速度、人口、存档、gameplay.json、quest_guidance.json或Content。未写根README、084 ROUTE_MANIFEST或其他任务文档。

新增 [RescueRouteTests.cpp](../../../Source/Hearthward/Tests/RescueRouteTests.cpp) 两项可执行检查：

| 测试过滤器 | 覆盖与执行层 | 初始结果 |
|---|---|---|
| `Hearthward.Iteration.Task092.Fixture.FollowWaitAndPhysicalArrival` | 隔离平地Game世界；真实动态Recast、AI路径、CharacterMovement、生产Interact／RecordRescue，验证明确等待、实际族人进入营地和唯一人口／XP | 首轮 Fail：2 error／1 warning；最小修复后Success：0 error／1 warning |
| `Hearthward.Iteration.Task092.Live.LoadedSegmentClearance` | 正式Wilds Game／PIE，只读当前加载地面、实际站立胶囊、三类真实角色双向完整局部导航；无生成／移动／注入状态 | NOT_RUN，需正常走完序章并加载弟弟／rescued_01且三人已落地 |

Fixture中缩短路线、调整玩家目的地和扩大invoker仅为独立诊断种子；它不证明600m正式首救路线、实际普通输入、同屏可见性或整个控制区完成。Live前置不满足会明确报错；不能把缺世界／未加载导航当确定路线Bug，也不能退化为平地通过。

## 已复现缺陷与最小修复

`CampaignInteraction.cpp`的族人交互由following切waiting只改Stage，未取消Actor当前AI MoveTo。`CampaignWorld.cpp`后续Tick只处理following人员，所以旧路径可能在waiting期间继续。

主Agent经UEClient构建成功后首跑Fixture，真实报告 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json` 中该测试为Fail，error=2、warning=1。原始条目已在修改前保存至 [NATIVE_RED.json](NATIVE_RED.json)，其中两条失败：`Explicit wait cancels the active production AI path`、`Waiting person does not move toward camp`。合法250cm内实际AI Moving路径切waiting后未取消并继续移动；这是隔离真实导航/CharacterMovement的运行复现，尚未声称正式地形全路线复现。

原警告：`LogCrowdFollowing: Unable to find RecastNavMesh instance while trying to create UCrowdManager instance`。测试已建立完整实际Recast路径并执行后续移动；warning按原文保留，没有被隐去或据此改导航配置。

基于RED，只在本单允许的 `CampaignInteraction.cpp` 添加AIController包含，并在人物Stage切waiting时从既有Actor/controller调用StopMovement。根因是权威等待状态与活动AI路径未同时停止；未改变跟随距离、危险条件、走速、人口/经验或任何坐标。源码和定向diff检查通过；主Agent实际integration已重跑同一Fixture为Success（0 error／1 warning），不将隔离诊断通过当正式路线通过。

## 逐验收项

| 用例 | 工程准备 | 正式结果／缺项 |
|---|---|---|
| T092-C01 正向通行 | 按现有route_trace和局部invoker设计可执行检查 | NOT_RUN：营地→岔口→首救全程普通输入和弟弟自然到达 |
| T092-C02 观察撤退 | 已保留三巡逻坐标、22m救援危险条件与原警戒；没有重配遭遇 | NOT_RUN：正常视角观察、绕行／处理／撤退 |
| T092-C03 救援对象 | 生产跟随／等待／继续、真实移动、唯一人口／XP诊断 | 隔离Fixture已RED确认明确等待不停车并最小修复；GREEN已实际Success；正式返营普通输入NOT_RUN |
| T092-C04 伙伴留营 | 路线探针不要求互斥任务并发，既有工作／跟随测试可复用 | NOT_RUN：真实留营与同行分别操作并记录反馈 |
| T092-C05 双向碰撞 | 实际胶囊及局部双向NavData查询，077门洞／台阶为已存在几何 | NOT_RUN：真实双向行走／冲刺／原攀越及跟随者能力；未冻结碰撞 |
| T092-C06 存读档流送 | 三节点正常保存继续与区域重载步骤明确 | NOT_RUN：隔离档真实恢复、敌人／族人／物品不复制 |
| T092-C07 资料交接 | [路线／净空计划](../../planning/TASK-084-103/TASK-092/CLEARANCE_PLAN.md)和 [095—098技术建议](../../planning/TASK-084-103/TASK-092/TECHNICAL_HANDOFF_095_098.md) 可消费 | 部分静态交付；真实Z／碰撞包络／截图／091本轮采用版本未就绪，保持WAITING_084_092 |

## 验证责任和限制

本子Agent未启动、停止或构建UE。root负责UEClient公开入口构建、发现测试数>0、原生执行以及当前普通输入能力的真实路线检查；工程／正常输入／Owner视觉分别记录。

原生源码使用当前5.8本地公开NavigationSystem／NavigationData头文件核对查询接口，未新增依赖。官方工程资料已列于净空计划。公共组合校验、任务范围基线与根README由root统一集成；未批准提交意味着包含任务快照的真实提交基线校验当前无法完成，不改验证器。

此次正式源码修复已引用首轮RED，修复后同一Fixture实际Success，0 error／1 warning，原条目见NATIVE_GREEN.json。后续美术当前没有碰撞冻结证明或Content授权；Owner设计确认与工程环境缺项分开，详见094具体决策和本单技术建议。

原生GREEN证据：.agent-local/qa/TASK-088-101/native-integration-20261007/index.json，原始报告时间 2026.10.06-20.46.13；同一Fixture Success，0 error／1 warning，完整条目/设备见 [NATIVE_GREEN.json](NATIVE_GREEN.json)。warning保留原CrowdFollowing提示；联合轮其他任务失败不混入092结果。
