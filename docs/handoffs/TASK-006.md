# TASK-006 交接：五秒持续动作与中断基础

日期：2026-09-17；Owner：XLingyyy；当前会话单写。
任务：[TASK-006](../tasks/TASK-006.md)；分支 `codex/TASK-006-timed-action`。
基线 `604e4f694934c5669e93ad81061bae2d503e76d6`；依赖未合并的 TASK-005，禁止先合并本任务。
测试时为 UNCOMMITTED_WORKTREE，提交绑定在后续文档提交补记；不将基线冒充受测实现。

## 完成内容

新增角色五秒动作组件，复用世界时钟实玩秒；默认Idle，不自动开始。
非零主动移动输入和UE正伤害事件中断；零伤害、转镜头不打断。暂停冻结，暂停中开始被拒绝。
运行中重复开始返回false且保留原进度；完成／中断均只广播一次，重新开始进度清零。
完成计时被钳制到5秒，事件在首次观察到达到5秒的组件帧触发，可能晚一帧。
开发命令 `Hearthward.Action.Start`／`Hearthward.Action` 提供开始与快照；Shipping不注册。
后续建造／救助消费者必须自行验证对象、材料等并完成真实结算；Completed不代表建筑或救助结果。
未修改任何Content、输入映射、004任务单或原设计；没有新增键位、资产锁、健康状态或库存状态。

## 验证与证据

- [构建](../qa/evidence/TASK-006/build.json)：UE5.8.1 Development Editor / Win64 首次构建成功，MSVC14.44.35228、SDK10.0.22621.0。
- [原生报告](../qa/evidence/TASK-006/automation-index.json)：`Hearthward.Actions` 2项实际执行，2通过，0失败。
- [执行摘要](../qa/evidence/TASK-006/automation-result.json)：保留命令、用例数和诊断；完整进程输出仅保留于本机运行目录。
- [真实PIE](../qa/evidence/TASK-006/pie-results.json)：两轮共32项全部通过，包括开发命令、重复开始、镜头、暂停、完成、中断、重试、角色移动和新会话隔离。
- 首轮暂停前后进度均为0.9899330958724022秒，墙钟暂停2.5秒；首次完成观测为5.023324001580477实玩秒，重试为5.01677729934454秒。
- 每轮实际监听到2次完成、2次中断；持续移动及重复中断没有重复事件。
- [暂停](../qa/evidence/TASK-006/round1-paused.png)、[完成](../qa/evidence/TASK-006/round1-completed.png)、[移动中断](../qa/evidence/TASK-006/round1-interrupted.png)截图已人工检查，状态与进度可见，角色及灰盒场景正常。
- [日志摘录](../qa/evidence/TASK-006/pie-log-excerpt.txt)记录原生快照。

引擎启动仍出现先前TASK-003／005已记录的13条 `LogAutomationTest: Error: Condition failed`，业务用例报告均Success，未隐藏诊断或声称整个日志无错误。
本次运行使用Enhanced Input动作注入及原生ApplyDamage；未新增实键／伤害战斗端到端测试，未实现健康扣除。
未运行打包、两机验证、最终读条UI、材料结算、建造或救助闭环；不以基础组件验收替代这些后续功能。

## 复现与集成

[BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)包含UEClient入口；[PIE脚本](../qa/evidence/TASK-006/verify_action_pie.py)可重复两轮验证。
用户已授权完成后直接提交推送本任务分支；未授权合并main。Issue与独立评审缺项，流程状态保持Blocked。
新任务未在父基线，按用户拆单执行授权做本地声明范围核对；该检查不冒充基线审批认证。
无新资产锁；旧TASK-003锁的归属不受本单影响。回退本任务实现提交即可移除组件和角色接线，既有世界时钟及灰盒资产保留。
