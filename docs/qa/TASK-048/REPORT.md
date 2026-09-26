# TASK-048 设计验算报告

2026-09-27，Windows／GameFactory Python环境，UE 5.8.2项目文档阶段。主干`e95fde185dc38d0b69c2de42ce66db0d1fd93294`；范围基线`ed1a69a005eef1e371ae76accaac6b8aecf11a71`。D1—D6待Owner批准。

受检候选／脚本提交：`bbbcda9c3eec0decf53368560927b515e1cbe2a6`。后续证据提交不改候选或计算脚本。

## 检查边界

[候选计算](calculation.json)38/38通过：ID与材料可达、无工具起步、046有限采食供给、动物伤害／头击标尺、鱼池权重和2%频率、作物照料／返种、家畜饲料预算和已有规则边界。

纸面模型沿Source/Hearthward/Camp/HearthwardCampState.cpp优先采余量最少的可用源，单份食物在批次开始时预留，消耗360普通工人·W分钟，耗尽后2880W再生。20日4人3点产320份，5人4点400份；5人3点351份并有3528W缺料。该模型不模拟实际寻路、地图阻挡、所有岗位竞争或真实玩家操作。

第一次模型按固定ID选点，恢复时反复切回大库存，使其他点迟迟无法耗尽；与既有046实现不同，检查实际代码后修正模型。保留[首次失败](calculation-first-failure.json)，未把模型错误归因于游戏Bug。

每小时理想300—450次成功钓鱼在2%下期望6—9事件，50次零事件概率0.36417。每点24鱼、首营4点96次成功期望1.92事件；这是概率算式，未声称实际玩家成功率、小游戏输入可行性或完整平衡已验证。

## 命令与证据

- `python -X utf8 docs/planning/TASK-048/audit.py`：PASS，38/38；[结果](calculation.json)。
- `python -X utf8 scripts/validate_repo.py`：PASS，0 errors；[日志](repo.txt)。
- `python -X utf8 scripts/validate_repo.py --task TASK-048 --base ed1a69a005eef1e371ae76accaac6b8aecf11a71`：PASS，0 errors；[范围日志](scope.txt)。
- `python -X utf8 -m unittest discover -s scripts/tests -v`：PASS，33/33；[工具日志](tools.txt)。

UE构建、原生、PIE、Shipping、物种视觉／骨骼／动画、实际自然地图布点与完整生活路线：NOT_RUN。源资产只核对版本库中的索引与文件路径，15套源模型不等于15种已实现动物。

## 研究依据与交付

引用Epic数据驱动内容和Random Streams官方文档见[设计末节](../../design/DSGN-R15-nature-production.md#工程参考)，只采用稳定ID、数据与状态分离以及可重复随机序列的工程经验；不把外部资料当作游戏数值出处或批准。当前仅内容提案和验算，不覆盖Resources运行表。
