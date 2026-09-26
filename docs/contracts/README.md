# 契约入口

这里的文件均为待团队批准的工程草案，不构成新增玩法决定。

| ID | 草案 | 作用 | 当前状态 |
|---|---|---|---|
| CT-001 | [世界与库存](CT-001-world-state.md) | 权威变更、稳定ID、成功／失败、单位 | DRAFT |
| CT-002 | [伙伴委托](CT-002-companion-command.md) | 输入、任务状态、执行、取消与回档失效 | DRAFT |
| CT-003 | [保存恢复](CT-003-save-load.md) | 格式、快照、时间线、知识与恢复顺序 | DRAFT |
| CT-TASK-043 | [世界时间与持久状态](CT-TASK-043-world-time-persistence.md) | 候选时间结算、刷新／旅行事务与领域快照；产品规则按DSGN-R01批准子范围，公共接口及迁移仍待审查 | DRAFT / APPROVAL_REQUIRED |
| CT-TASK-044 | [生存状态与原子结算](CT-TASK-044-survival-transitions.md) | 用药／救援、A/W事件、费用／中断、残伤去重与同档恢复；产品D1—D6已获Owner采纳，实现与schema5见044报告，PR #46已合入 | IMPLEMENTED_ON_TASK_BRANCH |

每次批准留下真实PR和人类审查人；没有批准链接就不能标Approved。
先批准最小契约，再提交代码中的唯一公共声明和夹具；提供者与消费者据此并行。
不需要每个私有函数都写契约。涉及跨模块可观察结果、共享数据或保存格式时才进入此目录。

[CT-TASK-045 动作命中与警戒](CT-TASK-045-combat-alert.md)：APPROVED，覆盖命中去重、费用与中断、观察者信息、尸体广播和保存边界。对应产品D1—D6已获批准，D4统一3秒静音零耐力处决。
