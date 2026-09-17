# TASK-009 个人背包查看界面与默认暂停

Owner：XLingyyy；当前会话单写。实现与本地验证完成；流程Blocked（Issue、独立评审未落实）。
分支 `codex/TASK-009-inventory-panel`，基线 `47207be2c199f0b2247c0f24cdbe090b873f45e9`，承接008。

## 设计依据与实现范围

GDD v0.3 第4章、Q026/Q153规定默认查看界面暂停、倒计时冻结；第7/14章要求背包信息可查看。
本单展示008已实现的五种普通物品：中文名称、实际数量、单重及总负重/容量。零数量不列行，空容器提示“背包为空”。
复用原生Canvas HUD和Enhanced Input，无新依赖、资产或UI数据副本。Tab为灰盒临时入口，按一次开关；持键不连续切换，暂停期间可关闭。
只恢复本界面发起的暂停，已暂停世界打开再关闭应保持暂停；现有世界时钟和动作随引擎暂停冻结。
现有动作HUD保留。普通移动、视角输入在暂停中停止，关闭后恢复。

## 边界

正式输入表R23、关闭一般暂停的设置例外R09保持OPEN；临时Tab不构成设计定稿。
不执行004，不做装备、任务物品、食物使用、兄弟转交、升级、存档、共享仓储或新暂停管理器。
空灰盒容器不确定故事开局物品；原设计和CT-001保持原状。

## 验收

- Editor构建与已有2项库存原生测试通过。
- 两轮真实PIE验证开关、持键、暂停时钟/动作、恢复与原有暂停保留；新PIE界面关闭且容器为空。
- 截图核对空背包、混合数量、小数重量、满载与已暂停动作；不同输出尺寸下可读。
- 完成实现、证据与交接后提交推送任务分支，不合并main。路径见[机器任务单](TASK-009.json)。

工程参考：[Epic UInputAction](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EnhancedInput/UInputAction) 的 bTriggerWhenPaused，
[Epic Input Overview](https://dev.epicgames.com/documentation/unreal-engine/input-overview-in-unreal-engine) 的 Started 输入事件。
