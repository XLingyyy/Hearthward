# 初始Backlog｜尚未领取

这张表仅是启动包快照；实际状态与Owner看GitHub Issue。内部TASK编号不等于Issue编号。

2026-09-17 已阅读 GDD v0.3。第一个实施任务为 [TASK-003 任务单](../tasks/TASK-003.md)，
从 TASK-000 拆出工程创建及最小行走验证；不等待 TASK-000 的两机验收结束。任务单已编写，尚未实施。

| 任务 | 目标 | 依赖 | 原型 | 状态 |
|---|---|---|---|---|
| [TASK-000](../tasks/TASK-000.json) | 仓库治理、环境锁定与两机启动验证 | 无工程前置 | 否 | Blocked，部分准备完成 |
| [TASK-003](../tasks/TASK-003.md) | UE工程与第三人称灰盒行走基线 | 已准备的本机工具链；开工授权与任务归属待确认 | 否 | Backlog |
| [TASK-001](../tasks/TASK-001.json) | 裁定R01—R05并发布设计决定 | 无工程前置 | 否 | Backlog |
| [TASK-002](../tasks/TASK-002.json) | 批准共享契约并建立唯一公共声明 | TASK-000 | 否 | Backlog |
| [TASK-010](../tasks/TASK-010.json) | 库存、重量、共享容器及真实转移 | TASK-002 | 是，待批准 | Backlog |
| [TASK-011](../tasks/TASK-011.json) | 距离交互、五秒读条和中断反馈 | TASK-002 | 是，待批准 | Backlog |
| [TASK-012](../tasks/TASK-012.json) | 伙伴安全委托与多趟采集交付 | TASK-002 | 是，待批准 | Backlog |
| [TASK-013](../tasks/TASK-013.json) | 世界知识快照与回档隔离 | TASK-002 | 是，待批准 | Backlog |
| [TASK-014](../tasks/TASK-014.json) | 灰盒资产与独立验证场景 | TASK-000 | 是，待批准 | Backlog |
| [TASK-015](../tasks/TASK-015.json) | 背包和伙伴UI状态壳 | TASK-002 | 是，待批准 | Backlog |
| [TASK-020](../tasks/TASK-020.json) | M2小闭环集成与合并后验证 | TASK-010, TASK-011, TASK-012, TASK-013, TASK-014, TASK-015 | 是，待批准 | Backlog |

M0后先共享契约，再独立领域，最后由集成人接入测试地图。R01—R05设计工作并行开展，不由功能Agent猜答案。
