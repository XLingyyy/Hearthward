# 最短阅读路径

核对日期：2026-10-05。远端主干基线67fb0784已集成043—052与动物／更新兼容同步；053—074批准施工在独立集成树.agent-local/task051继续，已获Owner授权提交并推送至施工分支，提交记录见交接。当前源码、工程证据和未运行出口见[项目状态](PROJECT_STATE.md)及[施工交接](handoffs/TASK-053.md)。

## 先看游戏与制作源

- [README](../README.md)是玩法、构建和运行入口。正常新游戏从夜袭序章开始；自然世界和各模块原始验证见对应任务报告。
- [动物演示操作](qa/TASK-051/使用说明.md)说明14种动物的近景、弟弟接近、致命伤和重置；专用地图使用独立临时档池。
- [建模与动作制作说明](../art_source/TASK-051/制作说明.md)提供Blender／FBX、贴图、连续预览、指导、工具代码与克隆恢复步骤。先运行`git lfs pull`取回二进制。
- [更新兼容报告](qa/update-compatibility-20260930.md)说明旧档保留、新版进度隔离、冲突清理及正式版更新提示。

## 开始一项开发任务

1. 阅读[AGENTS](../AGENTS.md)、[WORKFLOW](../WORKFLOW.md)，确认实际工作目录。Hearthward是游戏仓库，GameFactory是独立工具仓库。
2. 阅读[PROJECT_STATE](PROJECT_STATE.md)、当前`docs/tasks/TASK-xxx.json`和任务分支的交接；运行`python scripts/agent_context.py --task TASK-xxx`。
3. 核对目录、分支、完整HEAD、未提交文件、Owner、授权范围、相关契约和实际LFS锁。当前施工批次为[TASK-053—074](planning/TASK-053-074/REVIEW.md)，053—064、066—072及074已激活工程施工或定向调查；根集成目录为`.agent-local/task051`，分支`codex/TASK-053-traversal`。
4. 读取最小相关实现、[设计基线](design/CURRENT.md)和[未定规则](design/OPEN_QUESTIONS.md)。实施后执行匹配验证，更新README和本任务交接，记录真实提交与推送状态。

TASK-041及早期AI标签与canonical任务的对应关系见[映射账本](planning/TASK-041-baseline-ledger.md)。原始测试只证明各报告中列出的版本与路径；没有执行的当前UE／发行检查不得写为PASS。
