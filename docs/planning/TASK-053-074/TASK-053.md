# TASK-053｜补齐正式基础通行与导航连接

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿055，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-044、TASK-049、TASK-051。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

051已实现玩家翻越／游泳／坠落，当前导航是动态Recast和Invokers，049路线有历史连续行走记录。缺少覆盖正式地图的双方通路、流送边缘和异常落地证据。本单交付可复查的路线／连接清单及实际导航，优先修地形、阻挡和连接。

## 玩家流程与实施

逐段核对首营→岔路→首救→矿点→渡口→故乡外围，以及四区至少两处进出与撤离边。主任务路径宽≥1.5m、净高≥2.1m，有不用翻越／游泳的步行绕行。河岸应可由水中回到受支撑岸面，落脚处无悬空、夹缝或重叠胶囊。

台阶≤0.45m、坡≤45°；翻越仅(.45,1.2]m障碍、距离≤.8m、落脚深≥.6m，显式E，1A秒／8耐力；中断不返耐力、不无敌、留在实际位置。复用051动作和胶囊检测，关键路径不强迫精准跳跃。

弟弟通过现有导航组件寻找合法路。确需动作连接时，提供有稳定ID的入口／出口、单向或双向标志、可用角色、落脚和占用检测；先保证两端流送及Nav可用，再启动真实移动动作。取消、受击、入口受阻或卸载时保留物理位置并重新寻路。游泳连接只在可验证的入水／出水路线使用，仍付相同体力和溺水成本，不能把无路情况当到达。

复用051浮游3m/s、每A秒Smax／25耗耐、零耐下沉0.5m/s与10A秒逃生期限；坠落按h=v²／2g，≤3m无伤、3—12m线性至Hmax、≥12mHmax伤害。可救援地面零血进Downed，深水／虚空进Dead；落地伤害只结算一次。

## 状态与失败

地图连接不保存Actor指针；若增加连接动作状态，沿当前保存规则只在合法边界落档，读档废止旧回调。弟弟堵路先等待／重试并说明原因，不能瞬移玩家背后。地图演员卸载与导航待生成分开记录，只有实际可达才报告完成。

## 最小验证与出口

原生覆盖高度／耐力边界、零耐、落脚堵塞、受击／取消和一次落地伤害；正常键鼠在实际路段完成步行、翻越和往返上岸，记录两个角色的路线和流送节点。另测睡眠／读档／旅行中断动作及长距离返营。修过的路段复验即可；全图导航和性能由064／072继续验收。Owner检查通路可读性。新增自由攀岩、飞行、无限自动跳跃均不在本单。

## 建议施工范围

- `Source/Hearthward/Experience/HearthwardTraversalComponent.h`
- `Source/Hearthward/Experience/HearthwardTraversalComponent.cpp`
- `Source/Hearthward/Companion/HearthwardCompanionNavigationComponent.h`
- `Source/Hearthward/Companion/HearthwardCompanionNavigationComponent.cpp`
- `Source/Hearthward/Companion/HearthwardCompanionFixture.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Content/Hearthward/World/Natural/Rebuild/L_HearthwardWilds.umap及对应World Partition外部Actor（施工登记实际包名）`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-051-input-traversal](../../contracts/CT-TASK-051-input-traversal.md)、[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
