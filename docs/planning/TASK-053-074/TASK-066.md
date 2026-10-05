# TASK-066｜制作夜袭、故乡四区与八主线十五支线

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿068，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-049、TASK-056、TASK-059、TASK-064、TASK-065。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与内容来源

049已接可操作夜袭、四区、80驻军／两组各4人增援、正式23任务和救援；本单完成真实场景、节点表达与全篇正常条件验收。故事、固定台词、任务奖、布点和稳定ID沿[049任务卡](../TASK-049/QUESTS.md)及运行表，不另写一套剧情或复制23条配置。

## 章节与任务

主线8项：带火离开→把营地安顿下来→林中未归的人→新的立足之地→重认归路→让旗帜落下→归火→两处灯火。逐项落实真实地点、前置条件、当步交互、弟弟职责、终态及唯一奖。夜袭取护符后带弟弟从后巷撤离；到营后只进入后续状态一次。

支线15项：河门后的人、作坊的余火、旧屋仍有人、议场的归人、猎人的图样、矿脉的走向、渡口旧标、山路回声、药草留给归人、门楣上的旧物、下一季的种子、炉边的旧料、没有送出的信、岗哨上的记录、公共的一餐。四组营救合计10可救者；其余取物／知识／生产与实际物品／设施事件关联。支线既能敌占时做，也有夺回后的安全入口，不因提前学图纸或已开箱锁死。

## 四区与遭遇

河门16＝10普通／4射手／2重型；工坊20＝12／5／3；住区20＝12／4／4；议场24＝12／6／6。每区两巡逻、岗哨、两出入口、旗帜和救援／取物点。行路、遮挡、亮度、尸体藏匿和撤离实际可用；射手射线与弹体真实遮挡，重型门槛沿045／047。

工坊／议场各4固定增援只在本区基础敌仍存活且第一次成功报警时触发一次；基础敌全部清除前未触发则cancelled。没有无限刷兵，不要求故意报警才能通关。四个受保护非战斗人物与敌人类型分开，感应／锁定不得让其成为可攻击敌人。

## 状态、资产与验收

任务步骤、RescueId返营、奖账本、敌人Generation、旗帜、增援终态和安全入口一起保存；旧11个Demo任务独立留历史，不按同名猜正式完成。复用已有敌人、房屋／家具候选和049布点，射手／族人临时外形明确标记，070正式定稿。

定向原生覆盖多条件步骤、未触发增援取消、重复奖励、保护人物、支线前后世界兼容；正常游戏逐个完成8主线和15支线的条件路线，保存退出并从每种关键世界状态继续。生态相关支线／供给需060—062整合后补验，063完整能力同步消费。不以直接设任务完成或80敌同时加载站桩当全篇通过；总时长和平衡由073。

## 建议施工范围

- `Source/Hearthward/Campaign/`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/gameplay.json（批准23任务与场景绑定）`
- `Content/Hearthward/World/Natural/Rebuild/及对应外部Actor（先登记实际包）`
- `Content/Hearthward/Campaign/及Content/Characters/Brother/（登记实际敌人／族人资产）`
- `Source/Hearthward/Tests/CampaignTests.cpp`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)、[CT-TASK-045-combat-alert](../../contracts/CT-TASK-045-combat-alert.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
