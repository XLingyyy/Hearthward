# TASK-064｜接入自然地图真实地点、任务与玩家引导

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿066，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-049、TASK-051、TASK-056、TASK-057、TASK-058、TASK-059。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

049已有约4.99km规划路线、13节点与正式任务；051已有发现／站点／输入规则。目标是实际地形、地图标记、可交互地点和任务UI指向同一真实对象，让首次玩家沿主路完成首救和成长。

## 地点与引导设计

复用camp、route_fork、slice_rescue、route_mine、route_south_pass、route_west_bank、route_ford、route_east_bank、route_terrace、route_ridge、route_watch、route_north_trail、home_entry的稳定ID，不另建同名地点。实际地表／导航投射确认Z值；每节点记录入口／退出、可站交互点、可返回路线和流送边。

普通步行地点间目标90—150秒，用实测可走距离评估；现有远路不为凑数直接缩放世界。主路用地形轮廓、路边设施和可见参照引导，敌人区域明确可绕，首救附近有可回营路。地图保持100m探索揭示和未探索边界，发现与可旅行激活分别显示文字及形状；不把尚未到过的内容全亮。

首营任务页以当前一件可执行目标提示救援、工作台或升S2，给所需材料／前置及已知来源；不新增第二套教程任务。新玩家可跟踪／取消跟踪，背包、营地和弟弟委托入口有一致反馈；引导只在条件变化展示，不持续遮挡视野。

旅行点为camp／ford／hometown，第三站在永久胜利后可用。激活要求真实靠近并确认，旅行走052准备流送／Nav→提交；失败保留原位／原时间／进度。弟弟是否随行沿现有同行／战斗条件，未成功不伪造地点发现。

## 保存与最小验证

地点发现、激活、探索与任务步骤沿现有快照，演员加载不重开任务或发一次奖；地图状态来自领域事实。定向查未发现与未激活差异、旧地图档迁移、站点零跳时和失败回滚；正常键鼠按13节点走一条完整路线，覆盖河岸、敌人绕行、救援回营、地图跟踪和真实旅行。人类引导效果在065测，城市全23任务在066测；不把路线脚本位置采样当新玩家完成证明。

## 建议施工范围

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardScreenPaint.cpp`
- `Resources/Data/gameplay.json（既有路线／标记绑定）`
- `Content/Hearthward/World/Natural/Rebuild/及对应__ExternalActors__包（登记实际范围）`
- `Source/Hearthward/Tests/CampaignTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)、[CT-TASK-051-input-traversal](../../contracts/CT-TASK-051-input-traversal.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
