# TASK-059｜实现族人分工、营地生产和公共口粮

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿061，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-043、TASK-046、TASK-052、TASK-058。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

现有生产和公共池由046／052承接。目标是20初始普通族人＋最多10救回者的真实唯一分配、可观察有限生产及清楚的公共口粮界面。两兄弟独立登记，不把人数增长等同仓储免费增长。

## 分工与生产

面板先选择人物、地区、生产区与岗位，显示五个身体岗位和实际有效工效。兄弟各占1格，仅真实在岗做该工作时工效3；严重饥饿乘70%为2.1。睡眠、倒地、旅行、另接委托或离岗均暂离停止贡献，本人即时制作暂停其后台贡献。满5格不暗中替换；一个PersonId只有一个分配。

有限批次按真实投入与worker·W推进：采食360工人W一份，木石矿15工人W两份，其他沿046表。默认优先采食→烹饪→冶炼→工作台→锻造，玩家可调整。每批开始原子取料，缺源／缺料／不安全暂停新批；源点刷新不直接发库存，仍须真实劳动。模型说已做完不能入账。

公共池全局24点／日；两地共用一次，20—30人口不增第二消耗器。食物按已批准兑换点数捐入；兄弟每餐5点换40饱食，缺口明确反馈。没有族人个体饥饿／罢工／死亡／床位需求，不将场景Routine算额外产量。

## 表现与保存

普通劳动者只在安全区域出现，与实际分配对应；工作、缺料、暂离、等待有可读标记。为表现动画摆人不建立另一生产时钟；未加载地区仍沿已批准有效岗位W推进，不用Actor数量决定产量。转营立即停旧分配，普通人不模拟运输；兄弟实际到场后才劳动。

人口来源、分配、批次投入／小数进度、优先序和公共池余额同档恢复。救援只在成功返营后增加唯一人，不以接触救援点就加人口。

## 最小验证与出口

相关原生验证4普通＋1兄弟工效7、3普通＋2兄弟9、同人跨地不可重复、暂离／饥饿、缺源及跨刷新、公共口粮一天一次；正常UI分配4人采食、切岗位、调整队列、捐食／领取、睡8小时后核对余额，再读回旧档，确认无未来产物。实际人员动作由本单基本接入，长期全篇经济由073评估；不调人口、日耗或工作效率。

## 建议施工范围

- `Source/Hearthward/Camp/`
- `Source/Hearthward/Building/HearthwardBuildingEconomy.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp（仅救回人口接入）`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/Tests/CampEconomyTests.cpp`
- `Source/Hearthward/Tests/WorldClockTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
