# TASK-047 验证报告

日期2026-09-26；工作树codex/TASK-047-equipment-recipes，Windows／Python标准库。主干基线dbba6d052b7e08e612d858ca07fc87e90dae5840；范围批准记录61e2b6fa7e7c2982143bf49e69519604c470c928。

## 设计检查

候选计算已通过32项，完整结果见[calculation.json](calculation.json)。覆盖逐级曲线、52/87技能预算、旧ID保留、材料／工具依赖可达、046配方保持、装备维修来源、20敌耐久标尺、局部维修取整与初始负重。

这些检查只证明设计数据内部的指定关系。所有自然源点、奖励ID的布置及真实背包／维修／存档操作均未执行；候选未获Owner批准。

## 运行边界

UE Editor构建、原生测试、PIE、Shipping：NOT_RUN。本单没有修改Source／Resources／Content／Config；设计计算不是玩法实测。047实例化及schema6未实现，当前运行仍为schema5。

## 绑定提交与实际命令

受检设计／计算脚本提交：`b250b3b544e3e4da08740251aeee56b118259b1a`。环境：Windows、Python 3.13.5，仅标准库；LFS大资产未下载。随后提交仅登记本报告和日志，不修改设计／计算逻辑。

| 命令 | 实际结果 | 证据 |
|---|---|---|
| `python -X utf8 docs/planning/TASK-047/audit.py` | PASS，32/32关系检查 | [计算结果](calculation.json) |
| `python -X utf8 scripts/validate_repo.py` | PASS，0 errors，检查48个任务快照 | [日志](repo.txt) |
| `python -X utf8 scripts/validate_repo.py --task TASK-047 --base 61e2b6fa7e7c2982143bf49e69519604c470c928` | PASS，0 errors，允许路径内 | [日志](scope.txt) |
| `python -X utf8 -m unittest discover -s scripts/tests -v` | PASS，33/33 | [日志](tools.txt) |

计算得到：切片1,500经验／8级／7技能点，中期18,260／31级／27点，首版37,720／45级／39点；60级65,785经验，52/87点。建议开场每人装备与消耗品重20.5，剩余79.5。38件耐久装备均有维修基准，普通武器在指定无甲／无技能躯干标尺下20敌需修。

计算脚本检查设计关系与取得依赖，不证明图中已经存在对应资源、后勤NPC、奖励任务或新装备资产。局部维修整数点划分检查与向上取整公式共同说明拆分维修不能降低总材料费；没有把它声称为运行事务或UI实测。R11／R13／R14待批准，不能将PASS视作批准。

