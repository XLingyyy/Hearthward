# TASK-056｜实现敌人AI、潜入警戒和自然地图遭遇

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿058，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-043、TASK-045、TASK-049、TASK-052、TASK-053、TASK-055。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与已确认缺口

Combat已有发现条、尸体报警、最后已知位置和诱饵记忆，CampaignActor已有巡逻及Seen战斗。当前CampaignActor失去Seen后直接回巡逻，没有消费调查／搜索位置。本单让状态对应实际导航与动作，先用自然地图现有3人遭遇验证。

## 感知与状态

每观察者独立记录对玩家／弟弟的发现条。120°正面、无遮挡、R=12＋13L米；增长r=.2×clamp(10／max(d,1),.4,2)每A秒，失目击下降.1／秒；只用权威时间／区域亮度。两个兄弟感知不共享精确位置，模型不参与敌人决策。

巡逻→可疑调查→确认战斗→最后已知位置搜索→安全回岗；诱饵进入其实际投掷点调查。调查／搜索目的地来自已记录刺激，失去目击后禁止持续追踪真人坐标。保留既有批准计时与搜索退出，若实现尚无明确退出字段先在契约补准确条件，不静默增加任意难度旋钮。受伤只确认可观察来源。

尸体可见开始3A秒传播，伤害／死亡／击晕／动作中断取消；成功才标该尸体已传播地区并设置alarmUntilA=now＋120。同尸体不刷新，新尸体取更晚截止；持续目击／战斗不被倒计时清除。跨区追兵保留来源，邻区自行感知。双方离敌区且无战斗30A秒结束潜入，不复活、不返钱；不追入首营安全生产区。

## 三人遭遇与持久化

复用field_slice_01—03、现有方形巡逻和≥35m绕行，给正常玩家观察、伏击、正面战斗和逃离选择。先清敌再接近首救点，出现真实战利品、经验和救援变化；不要求这三人进入故乡胜利分母。解卡保留敌人ID、血、异常、生命代次及奖励；只能回合法原岗，无合法点报告阻塞，不能直接杀死解决。

击晕与击杀等价清敌与奖励。野外4日刷新／故乡永久不复活／尸体领取沿043、052，Actor卸载不另开生命代次。新增状态的稳定值加入既有快照，读档时恢复记忆但废止旧动作。

## 最小验证与出口

原生测双兄弟发现、失目击坐标不更新、传播被打断、同尸体不重报、跨区及死亡去重。正式遭遇正常键鼠验证偷袭／击晕／拖尸、诱饵、报警、遮挡逃离、搜索回岗、保存退出和回来；遮挡后敌人必须去最后已知点，不能只验证UI标签。全80人场景与有限增援由066承接。无LLM敌人、无限增援或额外帮派系统。

## 建议施工范围

- `Source/Hearthward/Campaign/HearthwardCampaignActor.h`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Combat/HearthwardCombatComponent.cpp`
- `Source/Hearthward/Combat/HearthwardCombatTargetComponent.h`
- `Source/Hearthward/Combat/HearthwardCombatTargetComponent.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Resources/Data/gameplay.json（遭遇和现有AI字段）`
- `Source/Hearthward/Tests/CombatTests.cpp`
- `Source/Hearthward/Tests/CampaignTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-045-combat-alert](../../contracts/CT-TASK-045-combat-alert.md)、[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
