# TASK-008 交接：个人背包基础数量、负重与行走减速

日期：2026-09-17；Owner：XLingyyy；当前会话单写。
任务：[TASK-008](../tasks/TASK-008.md)；分支 `codex/TASK-008-personal-inventory`。
父基线 `bc5b48c6081621f826d5edc03bf3c45bf983c849`；堆叠依赖TASK-007，禁止先合并。
测试时UNCOMMITTED_WORKTREE，受测实现现已提交为 `3dae1f275e2b6a955c22d6ee0d79e0839c9fe76e`。
测试完成后仅整理证据、任务和交接文档，功能代码未再修改；此绑定不宣称重新执行测试。

## 实现与边界

- 五种普通物品独立定义，容量100；采用整数百分之一重量单位，只存一份物品数量。
- 同步整批增加／扣除，返回Success、InvalidCount、UnknownItem、CapacityExceeded、InsufficientItems。拒绝时不改数量、重量，不广播变化。
- 增加前通过剩余容量除以单重验证数量，极大int32请求不会因乘法溢出放行。
- 角色监听成功变化，常态行走速度实时按350×(1−0.1r)计算；HUD左下只读显示实际负重。
- 耐力消耗倍率1＋0.1r可查询，但没有耐力系统消费者；没有声称已产生耗耐。
- 原设计7.8初配重量已从原DOCX核对；没有编造装备、关键物品、食谱、开局奖励或背包升级。

本单是角色本地普通物品子范围，不是CT-001／TASK-010完整实现；该草案及Backlog未修改。
每次同步调用是一次新操作，不接异步重试、operation_id或timeline_epoch。共享仓储、转移、存档和持久化需后续契约评审。
开发Add／Remove命令表示授予／扣除，不会在场景生成掉落、不代表已完成拾取交互；Shipping不注册。
灰盒每次生成角色时容器为空，不定义故事开局库存；004、资产、原设计和输入映射均未修改。

## 验证

- [构建](../qa/evidence/TASK-008/build.json)：UE5.8.1 Development Editor / Win64通过；MSVC14.44.35228、SDK10.0.22621.0。
- [原生报告](../qa/evidence/TASK-008/automation-index.json)：`Hearthward.Inventory` 两项实际执行通过，覆盖五种重量、2000箭精确满载、混合库存、失败无副作用、移除和独立实例。
- [执行摘要](../qa/evidence/TASK-008/automation-result.json)保留启动阶段13条先前已知的Condition failed诊断；没有把业务用例成功表述为整份引擎日志无错。
- [PIE结果](../qa/evidence/TASK-008/pie-results.json)记录两轮共44项检查全部通过；实际速度均为350／332.5／315／350cm/s，7次成功操作各发一次事件，失败不发事件。
- 逐张检查[半载](../qa/evidence/TASK-008/1-half.png)、[满载与动作](../qa/evidence/TASK-008/1-full-action.png)、[卸载与中断](../qa/evidence/TASK-008/1-unloaded.png)、[1999箭](../qa/evidence/TASK-008/1-arrows.png)：实际显示50.00、100.00、0.00、99.95；中文清晰，负重文本与动作HUD无重叠。
- [原生日志摘录](../qa/evidence/TASK-008/pie-log-excerpt.txt)保留开发命令成功及非法数量拒绝记录。

首轮PIE脚本在Pawn出现后立即查询输入映射，映射查询无结果而StopIteration，未执行库存断言。
测试改为等待当前Pawn对应的四键移动绑定就绪，避免按UObject名字排序假定移动动作；仅重跑PIE，未修改生产代码或重复原生测试。

## 复现与集成

入口见 [BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)，[PIE脚本](../qa/evidence/TASK-008/verify_inventory_pie.py)可重复两轮验证。
实际输入通过Enhanced Input动作注入；未新增物理按键录制、打包或两机验证。
领域拾取、共享仓储、兄弟转交、装备、关键物品、存档、请求去重均NOT_IMPLEMENTED，不以本单替代完整经济验收。
用户已授权完成后提交推送；不合并main。Issue与独立评审缺项，流程状态保持Blocked。
本地范围核对使用当前用户授权的任务声明，不冒充父基线中已有审批。无新资产锁，旧TASK-003锁不受影响。
回退本任务实现提交可移除背包、负重HUD及速度接线；005—007原机制保留。
