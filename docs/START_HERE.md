# 最短上手路径

核对日期：2026-09-27。当前核对主干：`origin/main@dbba6d052b7e08e612d858ca07fc87e90dae5840`，已含043规则、044生存、045战斗及046营地经济（PR #46／47／48）。当前canonical047（原稿049）D1—D6已获Owner批准，正在本任务分支实现与验证成长／逐件装备／制作维修及schema6；见[047任务单](tasks/TASK-047.md)。046原生25/25与PIE65/65证据仍绑定其报告所列源码。Windows Demo 已发布；其受测源码为 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5`，与主干 SHA 分开看。[项目状态](PROJECT_STATE.md)和[TASK-041 账本](planning/TASK-041-baseline-ledger.md)记录实现、集成、验证及待办；[发行报告](releases/demo-20260924/REPORT.md)记录 Shipping 包和本机安装范围。

## 先看游戏

- [README](../README.md#运行)：从 `L_Bootstrap` 的标题页进入自然营地，采集、委托、仓储、建造、制作和存读档的当前操作入口。
- 自然图有地图 `M`、日志 `J` 页面入口，尚无正式非营地地点、四区敌人和完整主支线。开发场景内容与自然地图内容分开看。
- 4032 m World Partition 底座、新营地和局部自然物件已接入；全路线通行、流送、性能与 Owner 视觉仍见 [TASK-026 交接](handoffs/TASK-026.md)。AI NPC 交付口径见 [TASK-029](tasks/TASK-029.md)，Demo 资源与家具见 [TASK-030](tasks/TASK-030.md)。

## 开始一项开发任务

1. 读根 [AGENTS](../AGENTS.md) 和 [WORKFLOW](../WORKFLOW.md)；实际工作区以用户指定目录为准。Hearthward 是独立 Git 仓库，上层 GameFactory 是工具仓库。
2. 读本页、[PROJECT_STATE](PROJECT_STATE.md)、当前 `docs/tasks/TASK-xxx.json` 和该任务分支的 `docs/handoffs/TASK-xxx.md`；运行 `python scripts/agent_context.py --task TASK-xxx`。
3. 核对目录、分支、HEAD、未提交改动、Owner、审查安排、可选 Issue、资产锁、允许路径和证据对应的源码。无法核实的项目标未知，不将旧任务的提交／推送权限转给新任务。
4. 按任务来源读取最小相关代码、契约与 [CURRENT](design/CURRENT.md)；未定规则在 [OPEN_QUESTIONS](design/OPEN_QUESTIONS.md)。读实现后再修改；定向验证，完成后同步 README 和本任务交接。

TASK-041 是“当前基线、编号与验收账本”，来源规划稿写为 TASK-043；后续规划编号按减 2 解释。2026-09-23 证据中的 TASK-041/042 是历史集成标签，属于 TASK-029 的来源，不覆盖本次 canonical TASK-041。[映射表](planning/TASK-041-baseline-ledger.md#2-编号映射)保留旧证据位置。任务 JSON、审查安排、可选 Issue 和主干测试结果仍须各自核实。
