# 最短阅读路径

2026-10-09 当前工作分支为 `codex/TASK-084-103-iteration`，尚未合并main或发布。当前实现及完整剩余项见[本批执行状态](planning/TASK-084-103/EXECUTION_STATUS.md)，候选结果见[103报告](qa/TASK-103/REPORT.md)。

2026-10-09 候选9 `0.2.0-preview.20261009.1` 已完成Shipping Build/Cook/Stage/Archive，受测源码 `2d55f0c9bb6d3cc305d6aa2f3bcdfa281f834a2f`，总289.83秒，Cook 0错误/1条MCP许可提示。目录 `F:/HearthwardDemo/iteration-084-103-20261009-9/Windows`，包含候选8之后的两草种接触阴影修复。五份随包资料已安装，CPU CMD实际拉起游戏进程及窗口；Computer Use首次捕获和重新绑定后重试均返回 `FrameArrived timed out: timed out waiting on channel`。本包标题版本、新游戏、保存/继续和Vulkan入口均未验证，此错误尚不能归因为游戏崩溃。未生成ZIP，未发布，整批验收仍未完成。 [候选9证据](qa/TASK-103/CANDIDATE9_PARTIAL.json)。最近正常输入验证为[候选8窗口/继续](qa/TASK-103/CANDIDATE8_WINDOW_FOLLOWUP.json)；历史候选5 ZIP只适用于旧源码。

石堡火烟首件已获Owner批准并接入，角色、武器、救援动作、设施与作物的最新实现见各任务交接。锻造27项付费建造/制作/存读档检查通过；完整动作、资源场景和Owner视听验收仍有缺口。不得以旧首件待批准记录覆盖本轮批准。

撤离导航的Development/API验证已推进到[Nav15自然到营检查点](qa/TASK-084/NAV15_EARNED_CAMP_CHECKPOINT.json)。完整Shipping键鼠路线与首次救援尚未通过；Nav12失败仅保留为历史。当前Computer Use可执行点击、短按键与拖动，缺少持续按键接口。

[087模型质量](qa/TASK-087/REPORT.md)与[102联合性能](qa/TASK-102/REPORT.md)仍未过门槛；替代模型对照未选出合格版本。真人样本与第二台实体机器验收未执行，整批不标记Done。UE服务生命周期继续使用UEClient；MCP是否在线取决于实际Editor进程，不把历史驻留状态当作当前连接状态。

2026-10-06 已公开主干历史入口：受测集成基线为 `main@af08e1ab`（PR #61），含 TASK-078—081 弟弟／对话／任务指引及 TASK-082 地图／传送／仓储界面。该版 Windows Demo 见[发行报告](releases/demo-20261006-2/REPORT.md)，当前源码和已知限制见[项目状态](PROJECT_STATE.md)。

先按[目录说明](REPOSITORY_LAYOUT.md)定位文件；开发证据见[QA索引](qa/README.md)，素材见[制作源索引](../art_source/README.md)。

## 先看游戏与制作源

- [README](../README.md)是玩法、构建和运行入口。正常新游戏从夜袭序章开始；自然世界和各模块原始验证见对应任务报告。
- [动物演示操作](qa/TASK-051/使用说明.md)说明14种动物的近景、弟弟接近、致命伤和重置；专用地图使用独立临时档池。
- [建模与动作制作说明](../art_source/TASK-051/制作说明.md)提供Blender／FBX、贴图、连续预览、指导、工具代码与克隆恢复步骤。先运行`git lfs pull`取回二进制。
- [更新兼容报告](qa/update-compatibility-20260930.md)说明旧档保留、新版进度隔离、冲突清理及正式版更新提示。

## 开始一项开发任务

1. 阅读[AGENTS](../AGENTS.md)、[WORKFLOW](../WORKFLOW.md)，确认实际工作目录。Hearthward是游戏仓库，GameFactory是独立工具仓库。
2. 阅读[PROJECT_STATE](PROJECT_STATE.md)、当前`docs/tasks/TASK-xxx.json`和任务分支的交接；运行`python scripts/agent_context.py --task TASK-xxx`。
3. 核对目录、分支、完整HEAD、未提交文件、Owner、授权范围、相关契约和实际LFS锁。[TASK-053—074](planning/TASK-053-074/REVIEW.md)施工成果已合入main，验收仍以各单记录为准；新任务使用独立分支，不把原`.agent-local/task051`视作默认最新主干。
4. 读取最小相关实现、[设计基线](design/CURRENT.md)和[未定规则](design/OPEN_QUESTIONS.md)。实施后执行匹配验证，更新README和本任务交接，记录真实提交与推送状态。

TASK-041及早期AI标签与canonical任务的对应关系见[映射账本](planning/TASK-041-baseline-ledger.md)。原始测试只证明各报告中列出的版本与路径；没有执行的当前UE／发行检查不得写为PASS。
