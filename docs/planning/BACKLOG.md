# 任务拆分索引

2026-09-18：013改为本机模型接入，原存档任务保留为016。实际实现与验证见PROJECT_STATE和各任务交接；流程Blocked不代表尚无本地成果，详见任务记录。内部TASK编号不等同于GitHub Issue编号。

| 任务 | 目标 | 依赖 | 流程状态 |
|---|---|---|---|
| [TASK-000](../tasks/TASK-000.json) | 仓库治理、环境锁定与两机启动验证 | 无 | Blocked |
| [TASK-001](../tasks/TASK-001.json) | 裁定R01—R05并发布设计决定 | 无 | Backlog |
| [TASK-002](../tasks/TASK-002.json) | 批准共享契约并建立唯一公共声明 | TASK-000 | Backlog |
| [TASK-003](../tasks/TASK-003.json) | 建立 Hearthward UE 5.8.1 工程与第三人称灰盒行走基线 | 无 | Blocked |
| [TASK-004](../tasks/TASK-004.json) | 获取／创建UE基础资产库与独立展示场景 | TASK-003 | Backlog |
| [TASK-005](../tasks/TASK-005.json) | 正常运行世界时钟与暂停冻结 | TASK-003 | Blocked |
| [TASK-006](../tasks/TASK-006.json) | 五秒持续动作与中断基础 | TASK-003, TASK-005 | Blocked |
| [TASK-007](../tasks/TASK-007.json) | 持续动作实时进度反馈 | TASK-006 | Blocked |
| [TASK-008](../tasks/TASK-008.json) | 个人背包基础数量、负重与行走减速 | TASK-003, TASK-007 | Blocked |
| [TASK-009](../tasks/TASK-009.json) | 个人背包查看界面与默认暂停 | TASK-005, TASK-007, TASK-008 | Blocked |
| [TASK-010](../tasks/TASK-010.json) | 库存、重量、共享容器及真实转移 | TASK-008, TASK-009 | Blocked |
| [TASK-011](../tasks/TASK-011.json) | 距离交互、五秒读条和中断反馈 | TASK-006, TASK-007, TASK-008, TASK-009 | Blocked |
| [TASK-012](../tasks/TASK-012.json) | 伙伴安全委托与多趟采集交付 | TASK-006, TASK-010 | Blocked |
| [TASK-013](../tasks/TASK-013.json) | 本地Qwen模型接入与轻量检索推理链 | TASK-012 | Blocked |
| [TASK-014](../tasks/TASK-014.json) | 灰盒资产与独立验证场景 | TASK-003 | Blocked |
| [TASK-015](../tasks/TASK-015.json) | 背包和伙伴UI状态壳 | TASK-009, TASK-012, TASK-013 | Blocked |
| [TASK-016](../tasks/TASK-016.json) | 世界知识快照与回档隔离 | TASK-002, TASK-005, TASK-010, TASK-012, TASK-013, TASK-015 | Blocked |
| [TASK-020](../tasks/TASK-020.json) | M2小闭环集成与合并后验证 | TASK-010, TASK-011, TASK-012, TASK-013, TASK-014, TASK-015, TASK-016 | Backlog |
