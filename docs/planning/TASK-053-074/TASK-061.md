# TASK-061｜实现钓鱼、稀有奖励与藏宝图闭环

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿063，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-048、TASK-052、TASK-057。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

048已有4鱼、鱼竿实例、张力、鱼点24库存、独立2%宝藏、地图箱和唯一奖励账本。目标是实际岸边、鱼竿持握、模式输入／反馈和藏宝图到开箱的完整闭环。

## 正常操作

首营河岸至少4处钓点，沿岸相隔≥40m，落脚有支撑；装备耐久>0鱼竿、有鱼饵、脱战，投钩≤15m。E进钓鱼，左键按住收线、放开泄力、Esc取消；拦截战斗和装备切换，菜单暂停冻结但对话语义沿051。

按批准张力0.45开始、安全.15—.85、按住+.30／秒、松开−.25／秒、每1.5秒±.05挣扎；1秒投钩、2—4秒咬钩及鱼种5—7有效拉扯，理想8—12实玩秒。只有安全区增加进度，0／1立即失败，12秒超时沿现行规则。视觉呈现安全带、真实进度、咬钩与失败原因，音效不替代可读反馈。

投出扣1饵；成功才扣竿1、点位1并发真实鱼。抛出后失败／取消不退饵、不磨竿／减鱼；容量在开始与成功都检查，成功边界满包则释放鱼，不发奖励。单钓点只许一个预留。

## 宝藏与保存

真正成功拿鱼后额外2%奖励，权重35／30／20／15沿048；无保底、不替换鱼。图到手前对应箱不存在；读图写知识与地图点，箱放夺回前后均可达地表，沿053验证站位和返回路。

已拥有／已学／已开／待领取同奖换2绳＋2草药，不能重抽；猎手弓图纸与支线05共享事实。容量不足保留同一待领取包；箱整批容量不够不改变开箱状态。保存成功序号／随机流／地图箱ID／待领取及鱼竿损耗，恢复相同状态保留下一结果，不额外抽奖。

## 最小验证与出口

原生定向查开始／抛出／成功边界、满包、同点争用、待领取占用、读档奖励去重、图先箱后；正常键鼠完成4鱼至少一轮输入和一条藏宝路线。低概率宝藏用独立测试种子验证账本，真人无强制中奖修改，统计与体验分开。鱼竿源资产实际挂接及动画纳入功能表现，精修归070；不加天气增益、保底、连钓加成或水下自由游鱼。

## 建议施工范围

- `Source/Hearthward/Nature/HearthwardNatureFishing.cpp`
- `Source/Hearthward/Nature/HearthwardNatureActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenNature.cpp`
- `Source/Hearthward/Input/（仅既有钓鱼语义接入）`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp（宝箱实际落点）`
- `Content/Hearthward/Nature/及现有鱼竿资产（按实际包登记）`
- `Source/Hearthward/Tests/NatureTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-048-nature](../../contracts/CT-TASK-048-nature.md)、[CT-TASK-047-progression-inventory](../../contracts/CT-TASK-047-progression-inventory.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
