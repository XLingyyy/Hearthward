# Hearthward 项目状态

更新：2026-09-17。TASK-003 已提交推送；TASK-004 仅任务单、未执行；TASK-005 世界时钟已完成本地实现及验证。

| 项目 | 实际状态 |
|---|---|
| 开发根目录 | 独立 Hearthward 仓库，与上层 GameFactory 工具仓库隔离 |
| 工作流 | v1.0 已导入；根 WORKFLOW.md 为维护入口，团队采用仍为 DRAFT |
| GitHub | origin 为 XLingyyy/Hearthward，公开仓库；当前按授权推送任务分支 |
| 当前分支 | codex/TASK-005-world-clock；基于5ec0648；用户已授权提交推送，提交绑定见交接 |
| 游戏实现提交 | 0732b7fc142e3089764a7101645259d12c97e9df |
| Git LFS | 已启用；4 个灰盒资产持锁，ID 见 TASK-003 交接 |
| 工具链 | UE 5.8.1、MSVC 19.44.35228.0、SDK 10.0.22621.0 |
| 工程 | 根 Hearthward.uproject；Source、Config、灰盒 Content 和本地框架插件源代码已提交 |
| 构建／操作 | Development Editor 编译通过；两轮 PIE 18/18 测量通过；实键与鼠标操作留证 |
| 仓库检查 | TASK-003 范围检查通过，工具自测 29/29 通过；不代表游戏全量验收 |
| 设计 | v0.3 原件未改；R01—R25 保持 OPEN，未新增设计决定 |
| TASK-005 | 正常运行世界时钟；构建通过，2项原生测试与两轮PIE共16项检查通过；见TASK-005交接 |
| TASK-004 | 人物基模、树草、岩石、房屋等基础资产任务单；Backlog，未下载或制作资产 |
| Issue／评审 | GitHub 连接器创建 Issue 返回 403；独立评审人未分配；TASK-003 仍为 Blocked |
| 打包／两机验证／完整 M0 | NOT_RUN |
| 模型与付费资产生成 | NOT_RUN |

TASK-003 成果和 TASK-004 任务单已推送。当前用户授权拆分、执行并提交推送 TASK-005，明确不执行 TASK-004；未授权合并。
[TASK-005 交接](handoffs/TASK-005.md)记录测试工作树、时间规则边界及真实验证。
[工程交接](handoffs/TASK-003.md)记录证据及初始化日志中的未解决提示；
[资产任务单](tasks/TASK-004.md)定义首批范围、设计边界、许可和导入验收。
运行测试发生于实现提交前工作树，提交绑定说明见交接；该条仅描述 TASK-003 的提交绑定；TASK-005 有独立构建和运行测试证据。

启动来源见 [TASK-000 交接](handoffs/TASK-000.md)及 [STARTER_VALIDATION](qa/STARTER_VALIDATION.md)，不替代游戏证据。
