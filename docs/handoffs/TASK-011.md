# TASK-011 距离交互交接

## 本次实现

交互发起组件绑定现有TimedActionComponent；目标SceneComponent显式配置MaxDistance，生产默认0禁用，未猜正式距离。
开始时拒绝无效/未配置/越界/暂停/正在移动/动作占用；交互中重复启动返回false并保留当前目标与计时。
运行中检查目标与距离，目标销毁/移远取消；移动和正伤害沿用006中断事件。交互检查是计时Tick的前置条件，避免目标失效的同帧先完成计时。
暂停期间沿用引擎冻结，不另建时钟；5秒完成时再检查目标，清空本次状态后发一次OnInteractionReady。
回调只表示交互条件满足，领域消费者必须再验证材料与结算，不能把计时Ready当成建筑完成。
E临时选择范围内最近目标；正式输入表R23、遮挡/完整选取规则尚未确定。
HUD增加目标无效、距离过远和计时完成等短提示，中断沿用007提示，避免重复显示。

## 设计与隔离

依据GDD Q043/Q170。未改设计、地图、配方、CT-001或004任务。
开发命令 `Hearthward.Interaction.CreateTestTarget` 创建PROTOTYPE_ONLY测试方块，距离200cm，放在角色前150cm；不授予材料或创建建筑，Shipping不注册。
2木材消费只存在于PIE脚本的完成回调，验证未完成不扣物、材料不足不成功；无正式建筑/奖励消费者。
本单仍prototype_only，正式prototype_approval未填。用户执行授权用于本地实现与独立夹具验证，未冒充Ready审批。
Issue、独立评审仍缺项，流程保持Blocked；未合并main，无资产改动或新锁。

## 提交绑定

基线 `c8797954ecca8374f35df67cf102107fd3d4de14`，分支 `codex/TASK-011-distance-interaction`。
tested_commit：`00443867e430f8e4647aa589c0638240cc75aa4c`。最终构建与定向HUD检查对应该实现提交前相同工作树；2项原生和46项PIE通过后仅改HUD中断提示去重，受测交互/计时代码未改。后续绑定提交仅更新交接与项目状态文档。用户授权完成后直接提交推送。

## 验证

UE5.8.1 / Win64 / Development Editor；MSVC14.44.35228、SDK10.0.22621.0，构建/测试/编辑器生命周期均经UEClient公开API。
- 最终构建通过：[build.json](../qa/evidence/TASK-011/build.json)。
- 原生 `Hearthward.Actions` 实际2/2通过：[原始报告](../qa/evidence/TASK-011/automation-index.json)。既有13条启动期Condition failed保留于[结果摘要](../qa/evidence/TASK-011/automation-result.json)，未声称日志无错误。
- 两轮真实PIE共46/46检查：[结果](../qa/evidence/TASK-011/pie-results.json)、[复现脚本](../qa/evidence/TASK-011/verify_interaction_pie.py)。包含200cm边界接受/201cm拒绝、重复开始、移动、零/正伤害、4秒后移远、目标销毁、暂停冻结、完成一次与途中材料被消费。
- 每轮两次完成回调的实际计时均5.0秒；第一次消费成功，第二次缺料失败，未重复扣物或产生建筑。
- 最后仅修正HUD的重复中断提示，交互和计时路径未改；未重复原生与完整PIE，定向显示检查2/2通过：[结果](../qa/evidence/TASK-011/hud-results.json)、[脚本](../qa/evidence/TASK-011/verify_interruption_hud.py)。

三张截图已逐张检查：[进行中](../qa/evidence/TASK-011/running.png)、[计时完成并扣2测试木材](../qa/evidence/TASK-011/ready.png)、[单条中断提示](../qa/evidence/TASK-011/interruption-hud.png)。
仓库结构检查与当前任务声明路径检查通过；[范围记录](../qa/evidence/TASK-011/scope-check.json)不等于基线正式审批认证。

首次构建复现旧库存与动作调试文件匿名命名空间QueryCommand重名：之前修改文件被Adaptive Unity分开编译，本次合并后暴露；库存变量已单行更名为InventoryQueryCommand，控制台命令字符串不变。
该必要修复已记入本单允许路径。失败构建保留于本机 `.agent-local/task011-output.txt` 指向目录。

截图用HighResShot输出1280×720，不代表物理窗口/DPI测试；输入为Enhanced Input注入并检查E映射，未声称物理键盘实测。
没有运行打包、正式建造、存档或营地交互验收。T-006仅当前独立场景范围通过。
复现命令和手动操作见[构建和测试入口](../qa/BUILD_AND_TEST.md)。
